////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include <ovito/core/dataset/io/FileSource.h>
#include <ovito/core/utilities/units/UnitsManager.h>
#include "ParticleType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticleType);
DEFINE_PROPERTY_FIELD(ParticleType, radius);
DEFINE_REFERENCE_FIELD(ParticleType, shapeMesh);
DEFINE_PROPERTY_FIELD(ParticleType, highlightShapeEdges);
DEFINE_PROPERTY_FIELD(ParticleType, shapeBackfaceCullingEnabled);
DEFINE_PROPERTY_FIELD(ParticleType, shapeUseMeshColor);
DEFINE_PROPERTY_FIELD(ParticleType, mass);
//Begin of modification
DEFINE_PROPERTY_FIELD(ParticleType, transparency);
DEFINE_PROPERTY_FIELD(ParticleType, particleSmoothing);
//End of modification
SET_PROPERTY_FIELD_LABEL(ParticleType, radius, "Radius");
SET_PROPERTY_FIELD_LABEL(ParticleType, shapeMesh, "Shape");
SET_PROPERTY_FIELD_LABEL(ParticleType, highlightShapeEdges, "Highlight edges");
SET_PROPERTY_FIELD_LABEL(ParticleType, shapeBackfaceCullingEnabled, "Back-face culling");
SET_PROPERTY_FIELD_LABEL(ParticleType, shapeUseMeshColor, "Use mesh color");
SET_PROPERTY_FIELD_LABEL(ParticleType, mass, "Mass");
//Begin of modification
SET_PROPERTY_FIELD_LABEL(ParticleType, transparency, "Transparency");
SET_PROPERTY_FIELD_LABEL(ParticleType, particleSmoothing, "Particle smoothing");
//End of modification
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticleType, radius, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new particle type.
******************************************************************************/
ParticleType::ParticleType(DataSet* dataset) : ElementType(dataset),
	_radius(0),
	_highlightShapeEdges(false),
	_shapeBackfaceCullingEnabled(true),
	_shapeUseMeshColor(false),
	_mass(0),
	_transparency(1.0),
	_particleSmoothing(0)
{
}

/******************************************************************************
 * Loads a user-defined display shape from a geometry file and assigns it to this particle type.
 ******************************************************************************/
bool ParticleType::loadShapeMesh(const QUrl& sourceUrl, Promise<>&& operation, const FileImporterClass* importerType)
{
    operation.setProgressText(tr("Loading mesh geometry file %1").arg(sourceUrl.fileName()));

	// Temporarily disable undo recording while loading the geometry data.
	UndoSuspender noUndo(this);

	OORef<FileSourceImporter> importer;
	if(!importerType) {

		// Inspect input file to detect its format.
		Future<OORef<FileImporter>> importerFuture = FileImporter::autodetectFileFormat(dataset(), sourceUrl);
		if(!operation.waitForFuture(importerFuture))
			return false;

		importer = dynamic_object_cast<FileSourceImporter>(importerFuture.result());
	}
	else {
		importer = dynamic_object_cast<FileSourceImporter>(importerType->createInstance(dataset()));
	}
	if(!importer)
		throwException(tr("Could not detect the format of the geometry file. The format might not be supported."));

	// Create a temporary FileSource for loading the geometry data from the file.
	OORef<FileSource> fileSource = new FileSource(dataset());
	fileSource->setSource({sourceUrl}, importer, false);
	SharedFuture<PipelineFlowState> stateFuture = fileSource->evaluate(PipelineEvaluationRequest(0));
	if(!operation.waitForFuture(stateFuture))
		return false;

	// Check if the FileSource has provided some useful data.
	const PipelineFlowState& state = stateFuture.result();
	if(state.status().type() == PipelineStatus::Error) {
		operation.cancel();
		return false;
	}
	if(!state)
		throwException(tr("The loaded geometry file does not provide any valid mesh data."));
	const TriMeshObject* meshObj = state.expectObject<TriMeshObject>();
	if(!meshObj->mesh())
		throwException(tr("The loaded geometry file does not contain a valid mesh."));

	// Turn on undo recording again. The final shape assignment should be recorded on the undo stack.
	noUndo.reset();
	setShapeMesh(meshObj);

	// Show sharp edges of the mesh.
	shapeMesh()->modifiableMesh()->determineEdgeVisibility();

    return !operation.isCanceled();
}

// Define default names, colors, and radii for some predefined particle types.
// MODIFICATIONS : ADD TRANSPARENCY OF 1 (FULL OPACITY)
std::array<ParticleType::PredefinedTypeInfo, ParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES> ParticleType::_predefinedParticleTypes{{
	ParticleType::PredefinedTypeInfo{ QString("H"), Color(255.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 0.46f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("He"), Color(217.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f), 1.22f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Li"), Color(204.0f/255.0f, 128.0f/255.0f, 255.0f/255.0f), 1.57f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("C"), Color(144.0f/255.0f, 144.0f/255.0f, 144.0f/255.0f), 0.77f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("N"), Color(48.0f/255.0f, 80.0f/255.0f, 248.0f/255.0f), 0.74f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("O"), Color(255.0f/255.0f, 13.0f/255.0f, 13.0f/255.0f), 0.74f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Na"), Color(171.0f/255.0f, 92.0f/255.0f, 242.0f/255.0f), 1.91f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Mg"), Color(138.0f/255.0f, 255.0f/255.0f, 0.0f/255.0f), 1.60f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Al"), Color(191.0f/255.0f, 166.0f/255.0f, 166.0f/255.0f), 1.43f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Si"), Color(240.0f/255.0f, 200.0f/255.0f, 160.0f/255.0f), 1.18f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("K"), Color(143.0f/255.0f, 64.0f/255.0f, 212.0f/255.0f), 2.35f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Ca"), Color(61.0f/255.0f, 255.0f/255.0f, 0.0f/255.0f), 1.97f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Ti"), Color(191.0f/255.0f, 194.0f/255.0f, 199.0f/255.0f), 1.47f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Cr"), Color(138.0f/255.0f, 153.0f/255.0f, 199.0f/255.0f), 1.29f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Fe"), Color(224.0f/255.0f, 102.0f/255.0f, 51.0f/255.0f), 1.26f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Co"), Color(240.0f/255.0f, 144.0f/255.0f, 160.0f/255.0f), 1.25f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Ni"), Color(80.0f/255.0f, 208.0f/255.0f, 80.0f/255.0f), 1.25f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Cu"), Color(200.0f/255.0f, 128.0f/255.0f, 51.0f/255.0f), 1.28f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Zn"), Color(125.0f/255.0f, 128.0f/255.0f, 176.0f/255.0f), 1.37f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Ga"), Color(194.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f), 1.53f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Ge"), Color(102.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f), 1.22f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Kr"), Color(92.0f/255.0f, 184.0f/255.0f, 209.0f/255.0f), 1.98f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Sr"), Color(0.0f, 1.0f, 0.15259f), 2.15f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Y"), Color(0.40259f, 0.59739f, 0.55813f), 1.82f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Zr"), Color(0.0f, 1.0f, 0.0f), 1.60f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Nb"), Color(0.29992f, 0.7f, 0.46459f), 1.47f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Pd"), Color(0.0f/255.0f, 105.0f/255.0f, 133.0f/255.0f), 1.37f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Pt"), Color(0.79997f, 0.77511f, 0.75068f), 1.39f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("W"), Color(0.55616f, 0.54257f, 0.50178f), 1.41f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Au"), Color(255.0f/255.0f, 209.0f/255.0f, 35.0f/255.0f), 1.44f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Pb"), Color(87.0f/255.0f, 89.0f/255.0f, 97.0f/255.0f), 1.47f, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Bi"), Color(158.0f/255.0f, 79.0f/255.0f, 181.0f/255.0f), 1.46f, 0.5 }
}};

// Define default names, colors, and radii for predefined structure types.
// SAME HERE
std::array<ParticleType::PredefinedTypeInfo, ParticleType::NUMBER_OF_PREDEFINED_STRUCTURE_TYPES> ParticleType::_predefinedStructureTypes{{
	ParticleType::PredefinedTypeInfo{ QString("Other"), Color(0.95f, 0.95f, 0.95f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("FCC"), Color(0.4f, 1.0f, 0.4f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("HCP"), Color(1.0f, 0.4f, 0.4f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("BCC"), Color(0.4f, 0.4f, 1.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("ICO"), Color(0.95f, 0.8f, 0.2f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Cubic diamond"), Color(19.0f/255.0f, 160.0f/255.0f, 254.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Cubic diamond (1st neighbor)"), Color(0.0f/255.0f, 254.0f/255.0f, 245.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Cubic diamond (2nd neighbor)"), Color(126.0f/255.0f, 254.0f/255.0f, 181.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Hexagonal diamond"), Color(254.0f/255.0f, 137.0f/255.0f, 0.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Hexagonal diamond (1st neighbor)"), Color(254.0f/255.0f, 220.0f/255.0f, 0.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Hexagonal diamond (2nd neighbor)"), Color(204.0f/255.0f, 229.0f/255.0f, 81.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Simple cubic"), Color(160.0f/255.0f, 20.0f/255.0f, 254.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Graphene"), Color(160.0f/255.0f, 120.0f/255.0f, 254.0f/255.0f), 0, 0.5 },	//todo: pick a different colour
	ParticleType::PredefinedTypeInfo{ QString("Hexagonal ice"), Color(0.0f, 0.9f, 0.9f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Cubic ice"), Color(1.0f, 193.0f/255.0f, 5.0f/255.0f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Interfacial ice"), Color(0.5f, 0.12f, 0.4f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Hydrate"), Color(1.0f, 0.3f, 0.1f), 0, 0.5 },
	ParticleType::PredefinedTypeInfo{ QString("Interfacial hydrate"), Color(0.1f, 1.0f, 0.1f), 0, 0.5 },
}};

/******************************************************************************
* Returns the default color for a particle type name.
******************************************************************************/
Color ParticleType::getDefaultParticleColor(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults)
{
	if(userDefaults) {
		QSettings settings;
		settings.beginGroup("particles/defaults/color");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(particleTypeName);
		if(v.isValid() && v.type() == QVariant::Color)
			return v.value<Color>();
	}

	if(typeClass == ParticlesObject::StructureTypeProperty) {
		for(const PredefinedTypeInfo& predefType : _predefinedStructureTypes) {
			if(std::get<0>(predefType) == particleTypeName)
				return std::get<1>(predefType);
		}
		return Color(1,1,1);
	}
	else if(typeClass == ParticlesObject::TypeProperty) {
		for(const PredefinedTypeInfo& predefType : _predefinedParticleTypes) {
			if(std::get<0>(predefType) == particleTypeName)
				return std::get<1>(predefType);
		}

		// Sometimes atom type names have additional letters/numbers appended.
		if(particleTypeName.length() > 1 && particleTypeName.length() <= 3) {
			return getDefaultParticleColor(typeClass, particleTypeName.left(particleTypeName.length() - 1), particleTypeId, userDefaults);
		}
	}

	return getDefaultColorForId(typeClass, particleTypeId);
}

/******************************************************************************
* Changes the default color for a particle type name.
******************************************************************************/
void ParticleType::setDefaultParticleColor(ParticlesObject::Type typeClass, const QString& particleTypeName, const Color& color)
{
	QSettings settings;
	settings.beginGroup("particles/defaults/color");
	settings.beginGroup(QString::number((int)typeClass));

	if(getDefaultParticleColor(typeClass, particleTypeName, 0, false) != color)
		settings.setValue(particleTypeName, QVariant::fromValue((QColor)color));
	else
		settings.remove(particleTypeName);
}

/******************************************************************************
* Returns the default radius for a particle type name.
******************************************************************************/
FloatType ParticleType::getDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults)
{
	if(userDefaults) {
		QSettings settings;
		settings.beginGroup("particles/defaults/radius");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(particleTypeName);
		if(v.isValid() && v.canConvert<FloatType>())
			return v.value<FloatType>();
	}

	if(typeClass == ParticlesObject::TypeProperty) {
		for(const PredefinedTypeInfo& predefType : _predefinedParticleTypes) {
			if(std::get<0>(predefType) == particleTypeName)
				return std::get<2>(predefType);
		}

		// Sometimes atom type names have additional letters/numbers appended.
		if(particleTypeName.length() > 1 && particleTypeName.length() <= 3) {
			return getDefaultParticleRadius(typeClass, particleTypeName.left(particleTypeName.length() - 1), particleTypeId, userDefaults);
		}
	}

	return 0;
}

/******************************************************************************
* Changes the default radius for a particle type name.
******************************************************************************/
void ParticleType::setDefaultParticleRadius(ParticlesObject::Type typeClass, const QString& particleTypeName, FloatType radius)
{
	QSettings settings;
	settings.beginGroup("particles/defaults/radius");
	settings.beginGroup(QString::number((int)typeClass));

	if(getDefaultParticleRadius(typeClass, particleTypeName, 0, false) != radius)
		settings.setValue(particleTypeName, QVariant::fromValue(radius));
	else
		settings.remove(particleTypeName);
}


//Begin of modifications
/*******************************************************************************
* Returns the default transparency for a particle type name.
*******************************************************************************/

FloatType ParticleType::getDefaultParticleTransparency(ParticlesObject::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults){

	if(userDefaults) {
        QSettings settings;
        settings.beginGroup("particles/defaults/transparency");
        settings.beginGroup(QString::number((int)typeClass));
        QVariant v = settings.value(particleTypeName);
        if(v.isValid() && v.canConvert<FloatType>())
            return v.value<FloatType>();
    }

    if(typeClass == ParticlesObject::TypeProperty) {
        for(const PredefinedTypeInfo& predefType : _predefinedParticleTypes) {
            if(std::get<0>(predefType) == particleTypeName){
				return std::get<2>(predefType);
                //return std::get<3>(predefType);
            }
			else{
				return FloatType(1.0);
			}
        }

	// Sometimes atom type names have additional letters/numbers appended.
		if(particleTypeName.length() > 1 && particleTypeName.length() <= 3) {
			return getDefaultParticleTransparency(typeClass, particleTypeName.left(particleTypeName.length() - 1), particleTypeId, userDefaults);
		}
    }
    return 0;

}
/******************************************************************************
* Changes the default transparency for a particle type name.
*******************************************************************************/
void ParticleType::setDefaultParticleTransparency(ParticlesObject::Type typeClass, const QString& particleTypeName, FloatType transparency)
{
    QSettings settings;
    settings.beginGroup("particles/defaults/transparency");
    settings.beginGroup(QString::number((int)typeClass));

    if(getDefaultParticleTransparency(typeClass, particleTypeName, 0, false) != transparency)
        settings.setValue(particleTypeName, QVariant::fromValue(transparency));
    else
        settings.remove(particleTypeName);
}
}	// End of namespace
}	// End of namespace
