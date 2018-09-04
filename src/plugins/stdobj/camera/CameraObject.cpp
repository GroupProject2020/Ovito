///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/stdobj/StdObj.h>
#include <plugins/stdobj/camera/TargetObject.h>
#include <core/viewport/Viewport.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/scene/PipelineSceneNode.h>
#include <core/rendering/RenderSettings.h>
#include <core/rendering/SceneRenderer.h>
#include <core/dataset/pipeline/StaticSource.h>
#include <core/utilities/units/UnitsManager.h>
#include "CameraObject.h"

namespace Ovito { namespace StdObj {

IMPLEMENT_OVITO_CLASS(CameraObject);
DEFINE_PROPERTY_FIELD(CameraObject, isPerspective);
DEFINE_REFERENCE_FIELD(CameraObject, fovController);
DEFINE_REFERENCE_FIELD(CameraObject, zoomController);
SET_PROPERTY_FIELD_LABEL(CameraObject, isPerspective, "Perspective projection");
SET_PROPERTY_FIELD_LABEL(CameraObject, fovController, "FOV angle");
SET_PROPERTY_FIELD_LABEL(CameraObject, zoomController, "FOV size");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CameraObject, fovController, AngleParameterUnit, FloatType(1e-3), FLOATTYPE_PI - FloatType(1e-2));
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CameraObject, zoomController, WorldParameterUnit, 0);

IMPLEMENT_OVITO_CLASS(CameraVis);

/******************************************************************************
* Constructs a camera object.
******************************************************************************/
CameraObject::CameraObject(DataSet* dataset) : AbstractCameraObject(dataset), _isPerspective(true)
{
	setFovController(ControllerManager::createFloatController(dataset));
	fovController()->setFloatValue(0, FLOATTYPE_PI/4);
	setZoomController(ControllerManager::createFloatController(dataset));
	zoomController()->setFloatValue(0, 200);

	addVisElement(new CameraVis(dataset));
}

/******************************************************************************
* Asks the object for its validity interval at the given time.
******************************************************************************/
TimeInterval CameraObject::objectValidity(TimePoint time)
{
	TimeInterval interval = DataObject::objectValidity(time);
	if(isPerspective() && fovController()) interval.intersect(fovController()->validityInterval(time));
	if(!isPerspective() && zoomController()) interval.intersect(zoomController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* Fills in the missing fields of the camera view descriptor structure.
******************************************************************************/
void CameraObject::projectionParameters(TimePoint time, ViewProjectionParameters& params) const
{
	// Transform scene bounding box to camera space.
	Box3 bb = params.boundingBox.transformed(params.viewMatrix).centerScale(FloatType(1.01));

	// Compute projection matrix.
	params.isPerspective = isPerspective();
	if(params.isPerspective) {
		if(bb.minc.z() < -FLOATTYPE_EPSILON) {
			params.zfar = -bb.minc.z();
			params.znear = std::max(-bb.maxc.z(), params.zfar * FloatType(1e-4));
		}
		else {
			params.zfar = std::max(params.boundingBox.size().length(), FloatType(1));
			params.znear = params.zfar * FloatType(1e-4);
		}
		params.zfar = std::max(params.zfar, params.znear * FloatType(1.01));

		// Get the camera angle.
		params.fieldOfView = fovController() ? fovController()->getFloatValue(time, params.validityInterval) : 0;
		if(params.fieldOfView < FLOATTYPE_EPSILON) params.fieldOfView = FLOATTYPE_EPSILON;
		if(params.fieldOfView > FLOATTYPE_PI - FLOATTYPE_EPSILON) params.fieldOfView = FLOATTYPE_PI - FLOATTYPE_EPSILON;

		params.projectionMatrix = Matrix4::perspective(params.fieldOfView, FloatType(1) / params.aspectRatio, params.znear, params.zfar);
	}
	else {
		if(!bb.isEmpty()) {
			params.znear = -bb.maxc.z();
			params.zfar  = std::max(-bb.minc.z(), params.znear + FloatType(1));
		}
		else {
			params.znear = 1;
			params.zfar = 100;
		}

		// Get the camera zoom.
		params.fieldOfView = zoomController() ? zoomController()->getFloatValue(time, params.validityInterval) : 0;
		if(params.fieldOfView < FLOATTYPE_EPSILON) params.fieldOfView = FLOATTYPE_EPSILON;

		params.projectionMatrix = Matrix4::ortho(-params.fieldOfView / params.aspectRatio, params.fieldOfView / params.aspectRatio,
							-params.fieldOfView, params.fieldOfView, params.znear, params.zfar);
	}
	params.inverseProjectionMatrix = params.projectionMatrix.inverse();
}

/******************************************************************************
* Returns the field of view of the camera.
******************************************************************************/
FloatType CameraObject::fieldOfView(TimePoint time, TimeInterval& validityInterval) const
{
	if(isPerspective())
		return fovController() ? fovController()->getFloatValue(time, validityInterval) : 0;
	else
		return zoomController() ? zoomController()->getFloatValue(time, validityInterval) : 0;
}

/******************************************************************************
* Changes the field of view of the camera.
******************************************************************************/
void CameraObject::setFieldOfView(TimePoint time, FloatType newFOV)
{
	if(isPerspective()) {
		if(fovController()) fovController()->setFloatValue(time, newFOV);
	}
	else {
		if(zoomController()) zoomController()->setFloatValue(time, newFOV);
	}
}

/******************************************************************************
* Returns whether this camera is a target camera directed at a target object.
******************************************************************************/
bool CameraObject::isTargetCamera() const
{
	for(RefMaker* dependent : dependents()) {
		if(StaticSource* staticSource = dynamic_object_cast<StaticSource>(dependent)) {
			if(staticSource->dataObjects().contains(const_cast<CameraObject*>(this))) {
				for(PipelineSceneNode* node : staticSource->pipelines(true)) {
					if(node->lookatTargetNode() != nullptr)
						return true;
				}
			}
		}
	}
	return false;
}

/******************************************************************************
* Changes the type of the camera to a target camera or a free camera.
******************************************************************************/
void CameraObject::setIsTargetCamera(bool enable)
{
	dataset()->undoStack().pushIfRecording<TargetChangedUndoOperation>(this);

	for(RefMaker* dependent : dependents()) {
		if(StaticSource* staticSource = dynamic_object_cast<StaticSource>(dependent)) {
			if(staticSource->dataObjects().contains(this)) {
				for(PipelineSceneNode* node : staticSource->pipelines(true)) {
					if(node->lookatTargetNode() == nullptr && enable) {
						if(SceneNode* parentNode = node->parentNode()) {
							AnimationSuspender noAnim(this);
							OORef<TargetObject> targetObj = new TargetObject(dataset());
							OORef<StaticSource> targetSource = new StaticSource(dataset(), targetObj);
							OORef<PipelineSceneNode> targetNode = new PipelineSceneNode(dataset());
							targetNode->setDataProvider(targetSource);
							targetNode->setNodeName(tr("%1.target").arg(node->nodeName()));
							parentNode->addChildNode(targetNode);
							// Position the new target to match the current orientation of the camera.
							TimeInterval iv;
							const AffineTransformation& cameraTM = node->getWorldTransform(dataset()->animationSettings()->time(), iv);
							Vector3 cameraPos = cameraTM.translation();
							Vector3 cameraDir = cameraTM.column(2).normalized();
							Vector3 targetPos = cameraPos - targetDistance() * cameraDir;
							targetNode->transformationController()->translate(0, targetPos, AffineTransformation::Identity());
							node->setLookatTargetNode(targetNode);
						}
					}
					else if(node->lookatTargetNode() != nullptr && !enable) {
						OORef<SceneNode> targetNode = node->lookatTargetNode();
						node->setLookatTargetNode(nullptr);
						targetNode->deleteNode();
					}
				}
			}
		}
	}

	dataset()->undoStack().pushIfRecording<TargetChangedRedoOperation>(this);
	notifyTargetChanged();
}

/******************************************************************************
* With a target camera, indicates the distance between the camera and its target.
******************************************************************************/
FloatType CameraObject::targetDistance() const
{
	for(RefMaker* dependent : dependents()) {
		if(StaticSource* staticSource = dynamic_object_cast<StaticSource>(dependent)) {
			if(staticSource->dataObjects().contains(const_cast<CameraObject*>(this))) {
				for(PipelineSceneNode* node : staticSource->pipelines(true)) {
					if(node->lookatTargetNode() != nullptr) {
						TimeInterval iv;
						Vector3 cameraPos = node->getWorldTransform(dataset()->animationSettings()->time(), iv).translation();
						Vector3 targetPos = node->lookatTargetNode()->getWorldTransform(dataset()->animationSettings()->time(), iv).translation();
						return (cameraPos - targetPos).length();
					}
				}
			}
		}
	}

	// That's the fixed target distance of a free camera:
	return FloatType(50);
}

/******************************************************************************
* Lets the vis element render a camera object.
******************************************************************************/
void CameraVis::render(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineFlowState& flowState, SceneRenderer* renderer, const PipelineSceneNode* contextNode)
{
	// Camera objects are only visible in the interactive viewports.
	if(renderer->isInteractive() == false || renderer->viewport() == nullptr)
		return;

	TimeInterval iv;

	std::shared_ptr<LinePrimitive> cameraIcon;
	std::shared_ptr<LinePrimitive> cameraPickIcon;
	if(!renderer->isBoundingBoxPass()) {

		// The key type used for caching the geometry primitive:
		using CacheKey = std::tuple<
			CompatibleRendererGroup,	// The scene renderer
			VersionedDataObjectRef,		// Camera object + revision number
			Color						// Display color
		>;

		// The values stored in the vis cache.
		struct CacheValue {
			std::shared_ptr<LinePrimitive> icon;
			std::shared_ptr<LinePrimitive> pickIcon;
		};

		// Determine icon color depending on selection state.
		Color color = ViewportSettings::getSettings().viewportColor(contextNode->isSelected() ? ViewportSettings::COLOR_SELECTION : ViewportSettings::COLOR_CAMERAS);

		// Lookup the rendering primitive in the vis cache.
		auto& cameraPrimitives = dataset()->visCache().get<CacheValue>(CacheKey(renderer, objectStack.back(), color));

		// Check if we already have a valid rendering primitive that is up to date.
		if(!cameraPrimitives.icon || !cameraPrimitives.pickIcon 
				|| !cameraPrimitives.icon->isValid(renderer)
				|| !cameraPrimitives.pickIcon->isValid(renderer)) {

			cameraPrimitives.icon = renderer->createLinePrimitive();
			cameraPrimitives.pickIcon = renderer->createLinePrimitive();

			// Initialize lines.
			static std::vector<Point3> iconVertices;
			if(iconVertices.empty()) {
				// Load and parse PLY file that contains the camera icon.
				QFile meshFile(QStringLiteral(":/core/3dicons/camera.ply"));
				meshFile.open(QIODevice::ReadOnly | QIODevice::Text);
				QTextStream stream(&meshFile);
				for(int i = 0; i < 3; i++) stream.readLine();
				int numVertices = stream.readLine().section(' ', 2, 2).toInt();
				OVITO_ASSERT(numVertices > 0);
				for(int i = 0; i < 3; i++) stream.readLine();
				int numFaces = stream.readLine().section(' ', 2, 2).toInt();
				for(int i = 0; i < 2; i++) stream.readLine();
				std::vector<Point3> vertices(numVertices);
				for(int i = 0; i < numVertices; i++)
					stream >> vertices[i].x() >> vertices[i].y() >> vertices[i].z();
				for(int i = 0; i < numFaces; i++) {
					int numEdges, vindex, lastvindex, firstvindex;
					stream >> numEdges;
					for(int j = 0; j < numEdges; j++) {
						stream >> vindex;
						if(j != 0) {
							iconVertices.push_back(vertices[lastvindex]);
							iconVertices.push_back(vertices[vindex]);
						}
						else firstvindex = vindex;
						lastvindex = vindex;
					}
					iconVertices.push_back(vertices[lastvindex]);
					iconVertices.push_back(vertices[firstvindex]);
				}
			}

			cameraPrimitives.icon->setVertexCount(iconVertices.size());
			cameraPrimitives.icon->setVertexPositions(iconVertices.data());
			cameraPrimitives.icon->setLineColor(ColorA(color));

			cameraPrimitives.pickIcon->setVertexCount(iconVertices.size(), renderer->defaultLinePickingWidth());
			cameraPrimitives.pickIcon->setVertexPositions(iconVertices.data());
			cameraPrimitives.pickIcon->setLineColor(ColorA(color));
		}
		cameraIcon = cameraPrimitives.icon;
		cameraPickIcon = cameraPrimitives.pickIcon;
	}

	// Determine the camera and target positions when rendering a target camera.
	FloatType targetDistance = 0;
	bool showTargetLine = false;
	if(contextNode->lookatTargetNode()) {
		Vector3 cameraPos = contextNode->getWorldTransform(time, iv).translation();
		Vector3 targetPos = contextNode->lookatTargetNode()->getWorldTransform(time, iv).translation();
		targetDistance = (cameraPos - targetPos).length();
		showTargetLine = true;
	}

	// Determine the aspect ratio and angle of the camera cone.
	FloatType aspectRatio = 0;
	FloatType coneAngle = 0;
	if(contextNode->isSelected()) {
		if(RenderSettings* renderSettings = dataset()->renderSettings())
			aspectRatio = renderSettings->outputImageAspectRatio();
		if(const CameraObject* camera = dynamic_object_cast<CameraObject>(objectStack.back())) {
			if(camera->isPerspective()) {
				coneAngle = camera->fieldOfView(time, iv);
				if(targetDistance == 0)
					targetDistance = camera->targetDistance();
			}
		}
	}

	if(!renderer->isBoundingBoxPass()) {
		if(!renderer->isPicking()) {
			// The key type used for caching the geometry primitive:
			using CacheKey = std::tuple<
				CompatibleRendererGroup,	// The scene renderer
				Color,						// Display color
				FloatType,					// Camera target distance
				bool,						// Target line visible
				FloatType,					// Cone aspect ratio
				FloatType					// Cone angle
			>;

			Color color = ViewportSettings::getSettings().viewportColor(ViewportSettings::COLOR_CAMERAS);
			
			// Lookup the rendering primitive in the vis cache.
			auto& conePrimitive = dataset()->visCache().get<std::shared_ptr<LinePrimitive>>(CacheKey(
					renderer,
					color, 
					targetDistance, 
					showTargetLine, 
					aspectRatio, 
					coneAngle));

			// Check if we already have a valid rendering primitive that is up to date.
			if(!conePrimitive || !conePrimitive->isValid(renderer)) {
				conePrimitive = renderer->createLinePrimitive();
				std::vector<Point3> targetLineVertices;
				if(targetDistance != 0) {
					if(showTargetLine) {
						targetLineVertices.push_back(Point3::Origin());
						targetLineVertices.push_back(Point3(0,0,-targetDistance));
					}
					if(aspectRatio != 0 && coneAngle != 0) {
						FloatType sizeY = tan(FloatType(0.5) * coneAngle) * targetDistance;
						FloatType sizeX = sizeY / aspectRatio;
						targetLineVertices.push_back(Point3::Origin());
						targetLineVertices.push_back(Point3(sizeX, sizeY, -targetDistance));
						targetLineVertices.push_back(Point3::Origin());
						targetLineVertices.push_back(Point3(-sizeX, sizeY, -targetDistance));
						targetLineVertices.push_back(Point3::Origin());
						targetLineVertices.push_back(Point3(-sizeX, -sizeY, -targetDistance));
						targetLineVertices.push_back(Point3::Origin());
						targetLineVertices.push_back(Point3(sizeX, -sizeY, -targetDistance));

						targetLineVertices.push_back(Point3(sizeX, sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(-sizeX, sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(-sizeX, sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(-sizeX, -sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(-sizeX, -sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(sizeX, -sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(sizeX, -sizeY, -targetDistance));
						targetLineVertices.push_back(Point3(sizeX, sizeY, -targetDistance));
					}
				}
				conePrimitive->setVertexCount(targetLineVertices.size());
				conePrimitive->setVertexPositions(targetLineVertices.data());
				conePrimitive->setLineColor(ColorA(color));
			}
			conePrimitive->render(renderer);
		}
	}
	else {
		// Add camera view cone to bounding box.
		if(showTargetLine) {
			renderer->addToLocalBoundingBox(Point3::Origin());
			renderer->addToLocalBoundingBox(Point3(0,0,-targetDistance));
		}
		if(aspectRatio != 0 && coneAngle != 0) { 
			FloatType sizeY = tan(FloatType(0.5) * coneAngle) * targetDistance;
			FloatType sizeX = sizeY / aspectRatio;
			renderer->addToLocalBoundingBox(Point3(sizeX, sizeY, -targetDistance));
			renderer->addToLocalBoundingBox(Point3(-sizeX, sizeY, -targetDistance));
			renderer->addToLocalBoundingBox(Point3(-sizeX, -sizeY, -targetDistance));
			renderer->addToLocalBoundingBox(Point3(sizeX, -sizeY, -targetDistance));
		}
	}

	// Setup transformation matrix to always show the camera icon at the same size.
	Point3 cameraPos = Point3::Origin() + renderer->worldTransform().translation();
	FloatType scaling = FloatType(0.3) * renderer->viewport()->nonScalingSize(cameraPos);
	renderer->setWorldTransform(renderer->worldTransform() * AffineTransformation::scaling(scaling));

	if(!renderer->isBoundingBoxPass()) {
		renderer->beginPickObject(contextNode);
		if(!renderer->isPicking())
			cameraIcon->render(renderer);
		else
			cameraPickIcon->render(renderer);
		renderer->endPickObject();
	}
	else {
		// Add camera symbol to bounding box.
		renderer->addToLocalBoundingBox(Box3(Point3::Origin(), scaling * 2));
	}
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 CameraVis::boundingBox(TimePoint time, const std::vector<const DataObject*>& objectStack, const PipelineSceneNode* contextNode, const PipelineFlowState& flowState, TimeInterval& validityInterval)
{
	// This is not a physical object. It doesn't have a size.
	return Box3(Point3::Origin(), Point3::Origin());
}

}	// End of namespace
}	// End of namespace
