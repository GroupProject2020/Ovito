////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/particles/Particles.h>
#include <ovito/particles/objects/BondsVis.h>
#include <ovito/core/app/Application.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/utilities/concurrent/ParallelFor.h>
#include "BondsObject.h"
#include "ParticlesObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondsObject);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsObject::BondsObject(DataSet* dataset) : PropertyContainer(dataset)
{
	// Attach a visualization element for rendering the bonds.
	addVisElement(new BondsVis(dataset));
}

/******************************************************************************
* Gives the property class the opportunity to set up a newly created
* property object.
******************************************************************************/
void BondsObject::OOMetaClass::prepareNewProperty(PropertyObject* property) const
{
}

/******************************************************************************
* Creates a storage object for standard bond properties.
******************************************************************************/
PropertyPtr BondsObject::OOMetaClass::createStandardStorage(size_t bondsCount, int type, bool initializeMemory, const ConstDataObjectPath& containerPath) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case TypeProperty:
	case SelectionProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case LengthProperty:
	case TransparencyProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case ColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case TopologyProperty:
		dataType = PropertyStorage::Int64;
		componentCount = 2;
		stride = componentCount * sizeof(qlonglong);
		break;
	case PeriodicImageProperty:
		dataType = PropertyStorage::Int;
		componentCount = 3;
		stride = componentCount * sizeof(int);
		break;
	default:
		OVITO_ASSERT_MSG(false, "BondsObject::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard bond property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));

	PropertyPtr property = std::make_shared<PropertyStorage>(bondsCount, dataType, componentCount, stride,
								propertyName, false, type, componentNames);

	// Initialize memory if requested.
	if(initializeMemory && containerPath.size() >= 2) {
		// Certain standard properties need to be initialized with default values determined by the attached visual elements.
		if(type == ColorProperty) {
			if(const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(containerPath[containerPath.size()-2])) {
				const std::vector<ColorA>& colors = particles->inputBondColors();
				OVITO_ASSERT(colors.size() == property->size());
				boost::transform(colors, property->data<Color>(), [](const ColorA& c) { return Color(c.r(), c.g(), c.b()); });
				initializeMemory = false;
			}
		}
	}

	if(initializeMemory) {
		// Default-initialize property values with zeros.
		std::memset(property->data<void>(), 0, property->size() * property->stride());
	}

	return property;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void BondsObject::OOMetaClass::initialize()
{
	PropertyContainerClass::initialize();

	// Enable automatic conversion of a BondPropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<BondPropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, BondPropertyReference>();

	setPropertyClassDisplayName(tr("Bonds"));
	setElementDescriptionName(QStringLiteral("bonds"));
	setPythonName(QStringLiteral("bonds"));

	const QStringList emptyList;
	const QStringList abList = QStringList() << "A" << "B";
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";

	registerStandardProperty(TypeProperty, tr("Bond Type"), PropertyStorage::Int, emptyList, tr("Bond types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Bond colors"));
	registerStandardProperty(LengthProperty, tr("Length"), PropertyStorage::Float, emptyList);
	registerStandardProperty(TopologyProperty, tr("Topology"), PropertyStorage::Int64, abList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), PropertyStorage::Int, xyzList);
	registerStandardProperty(TransparencyProperty, tr("Transparency"), PropertyStorage::Float, emptyList);
}

/******************************************************************************
* Returns the index of the element that was picked in a viewport.
******************************************************************************/
std::pair<size_t, ConstDataObjectPath> BondsObject::OOMetaClass::elementFromPickResult(const ViewportPickResult& pickResult) const
{
	// Check if a bond was picked.
	if(BondPickInfo* pickInfo = dynamic_object_cast<BondPickInfo>(pickResult.pickInfo())) {
		if(const ParticlesObject* particles = pickInfo->pipelineState().getObject<ParticlesObject>()) {
			size_t bondIndex = pickResult.subobjectId() / 2;
			if(particles->bonds() && bondIndex < particles->bonds()->elementCount()) {
				return std::make_pair(bondIndex, ConstDataObjectPath{{particles, particles->bonds()}});
			}
		}
	}

	return std::pair<size_t, DataObjectPath>(std::numeric_limits<size_t>::max(), {});
}

/******************************************************************************
* Tries to remap an index from one property container to another, considering the
* possibility that elements may have been added or removed.
******************************************************************************/
size_t BondsObject::OOMetaClass::remapElementIndex(const ConstDataObjectPath& source, size_t elementIndex, const ConstDataObjectPath& dest) const
{
	const BondsObject* sourceBonds = static_object_cast<BondsObject>(source.back());
	const BondsObject* destBonds = static_object_cast<BondsObject>(dest.back());
	const ParticlesObject* sourceParticles = dynamic_object_cast<ParticlesObject>(source.size() >= 2 ? source[source.size()-2] : nullptr);
	const ParticlesObject* destParticles = dynamic_object_cast<ParticlesObject>(dest.size() >= 2 ? dest[dest.size()-2] : nullptr);
	if(sourceParticles && destParticles) {

		// Make sure the topology information is present.
		if(const PropertyObject* sourceTopology = sourceBonds->getProperty(TopologyProperty)) {
			if(const PropertyObject* destTopology = destBonds->getProperty(TopologyProperty)) {

				// If unique IDs are available try to use them to look up the bond in the other data collection.
				if(const PropertyObject* sourceIdentifiers = sourceParticles->getProperty(ParticlesObject::IdentifierProperty)) {
					if(const PropertyObject* destIdentifiers = destParticles->getProperty(ParticlesObject::IdentifierProperty)) {
						size_t index_a = sourceTopology->get<qlonglong>(elementIndex, 0);
						size_t index_b = sourceTopology->get<qlonglong>(elementIndex, 1);
						if(index_a < sourceIdentifiers->size() && index_b < sourceIdentifiers->size()) {
							qlonglong id_a = sourceIdentifiers->get<qlonglong>(index_a);
							qlonglong id_b = sourceIdentifiers->get<qlonglong>(index_b);

							// Quick test if the bond storage order is the same.
							if(elementIndex < destTopology->size()) {
								size_t index2_a = destTopology->get<qlonglong>(elementIndex, 0);
								size_t index2_b = destTopology->get<qlonglong>(elementIndex, 1);
								if(index2_a < destIdentifiers->size() && index2_b < destIdentifiers->size()) {
									if(destIdentifiers->get<qlonglong>(index2_a) == id_a && destIdentifiers->get<qlonglong>(index2_b) == id_b) {
										return elementIndex;
									}
								}
							}

							// Determine the indices of the two particles connected by the bond.
							size_t index2_a = boost::find(destIdentifiers->crange<qlonglong>(), id_a) - destIdentifiers->cdata<qlonglong>();
							size_t index2_b = boost::find(destIdentifiers->crange<qlonglong>(), id_b) - destIdentifiers->cdata<qlonglong>();
							if(index2_a < destIdentifiers->size() && index2_b < destIdentifiers->size()) {
								// Go through the whole bonds list to see if there is a bond connecting the particles with
								// the same IDs.
								for(auto bond = destTopology->cdata<qlonglong>(), bond_end = bond + destTopology->size()*2; bond != bond_end; bond += 2) {
									if((bond[0] == index2_a && bond[1] == index2_b) || (bond[0] == index2_b && bond[1] == index2_a)) {
										return (bond - destTopology->cdata<qlonglong>()) / 2;
									}
								}
							}
						}

						// Give up.
						return PropertyContainerClass::remapElementIndex(source, elementIndex, dest);
					}
				}

				// Try to find matching bond based on particle indices alone.
				if(const PropertyObject* sourcePos = sourceParticles->getProperty(ParticlesObject::PositionProperty)) {
					if(const PropertyObject* destPos = destParticles->getProperty(ParticlesObject::PositionProperty)) {
						size_t index_a = sourceTopology->get<qlonglong>(elementIndex, 0);
						size_t index_b = sourceTopology->get<qlonglong>(elementIndex, 1);
						if(index_a < sourcePos->size() && index_b < sourcePos->size()) {

							// Quick check if number of particles and bonds didn't change.
							if(sourcePos->size() == destPos->size() && sourceTopology->size() == destTopology->size()) {
								size_t index2_a = destTopology->get<qlonglong>(elementIndex, 0);
								size_t index2_b = destTopology->get<qlonglong>(elementIndex, 1);
								if(index_a == index2_a && index_b == index2_b) {
									return elementIndex;
								}
							}

							// Find matching bond by means of particle positions.
							const Point3& pos_a = sourcePos->get<Point3>(index_a);
							const Point3& pos_b = sourcePos->get<Point3>(index_b);
							size_t index2_a = boost::find(destPos->crange<Point3>(), pos_a) - destPos->cdata<Point3>();
							size_t index2_b = boost::find(destPos->crange<Point3>(), pos_b) - destPos->cdata<Point3>();
							if(index2_a < destPos->size() && index2_b < destPos->size()) {
								// Go through the whole bonds list to see if there is a bond connecting the particles with
								// the same positions.
								for(auto bond = destTopology->cdata<qlonglong>(), bond_end = bond + destTopology->size()*2; bond != bond_end; bond += 2) {
									if((bond[0] == index2_a && bond[1] == index2_b) || (bond[0] == index2_b && bond[1] == index2_a)) {
										return (bond - destTopology->cdata<qlonglong>()) / 2;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Give up.
	return PropertyContainerClass::remapElementIndex(source, elementIndex, dest);
}

/******************************************************************************
* Determines which elements are located within the given
* viewport fence region (=2D polygon).
******************************************************************************/
boost::dynamic_bitset<> BondsObject::OOMetaClass::viewportFenceSelection(const QVector<Point2>& fence, const ConstDataObjectPath& objectPath, PipelineSceneNode* node, const Matrix4& projectionTM) const
{
	const BondsObject* bonds = static_object_cast<BondsObject>(objectPath.back());
	const ParticlesObject* particles = dynamic_object_cast<ParticlesObject>(objectPath.size() >= 2 ? objectPath[objectPath.size()-2] : nullptr);

	if(particles) {
		if(const PropertyObject* topologyProperty = bonds->getProperty(BondsObject::TopologyProperty)) {
			if(const PropertyObject* posProperty = particles->getProperty(ParticlesObject::PositionProperty)) {

				if(!bonds->visElement() || bonds->visElement()->isEnabled() == false)
					node->throwException(tr("Cannot select bonds while the corresponding visual element is disabled. Please enable the display of bonds first."));

				boost::dynamic_bitset<> fullSelection(topologyProperty->size());
				QMutex mutex;
				parallelForChunks(topologyProperty->size(), [topo = topologyProperty->cdata<qlonglong>(), posProperty, &projectionTM, &fence, &mutex, &fullSelection](size_t startIndex, size_t chunkSize) {
					boost::dynamic_bitset<> selection(fullSelection.size());
					auto t = topo + startIndex * 2;
					for(size_t index = startIndex; chunkSize != 0; chunkSize--, index++, t += 2) {
						int insideCount = 0;
						for(int i = 0; i < 2; i++) {
							if(t[i] >= posProperty->size()) continue;
							const Point3& p = posProperty->get<Point3>(t[i]);

							// Project particle center to screen coordinates.
							Point3 projPos = projectionTM * p;

							// Perform z-clipping.
							if(std::abs(projPos.z()) >= FloatType(1))
								break;

							// Perform point-in-polygon test.
							int intersectionsLeft = 0;
							int intersectionsRight = 0;
							for(auto p2 = fence.constBegin(), p1 = p2 + (fence.size()-1); p2 != fence.constEnd(); p1 = p2++) {
								if(p1->y() == p2->y()) continue;
								if(projPos.y() >= p1->y() && projPos.y() >= p2->y()) continue;
								if(projPos.y() < p1->y() && projPos.y() < p2->y()) continue;
								FloatType xint = (projPos.y() - p2->y()) / (p1->y() - p2->y()) * (p1->x() - p2->x()) + p2->x();
								if(xint >= projPos.x())
									intersectionsRight++;
								else
									intersectionsLeft++;
							}
							if(intersectionsRight & 1)
								insideCount++;
						}
						if(insideCount == 2)
							selection.set(index);
					}
					// Transfer thread-local results to output bit array.
					QMutexLocker locker(&mutex);
					fullSelection |= selection;
				});

				return fullSelection;
			}
		}
	}

	// Give up.
	return PropertyContainerClass::viewportFenceSelection(fence, objectPath, node, projectionTM);
}

}	// End of namespace
}	// End of namespace
