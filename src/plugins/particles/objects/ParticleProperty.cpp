///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include "ParticleProperty.h"
#include "ParticlesVis.h"
#include "VectorVis.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticleProperty);

/******************************************************************************
* Constructor.
******************************************************************************/
ParticleProperty::ParticleProperty(DataSet* dataset) : PropertyObject(dataset)
{
}

/******************************************************************************
* Gives the property class the opportunity to set up a newly created 
* property object.
******************************************************************************/
void ParticleProperty::OOMetaClass::prepareNewProperty(PropertyObject* property) const
{
	if(property->type() == ParticleProperty::PositionProperty) {
		OORef<ParticlesVis> vis = new ParticlesVis(property->dataset());
		vis->loadUserDefaults();
		property->addVisElement(vis);
	}
	else if(property->type() == ParticleProperty::DisplacementProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Displacements"));
		vis->loadUserDefaults();
		vis->setEnabled(false);
		property->addVisElement(vis);
	}
	else if(property->type() == ParticleProperty::ForceProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Forces"));
		vis->loadUserDefaults();
		vis->setEnabled(false);
		vis->setReverseArrowDirection(false);
		vis->setArrowPosition(VectorVis::Base);
		property->addVisElement(vis);
	}
	else if(property->type() == ParticleProperty::DipoleOrientationProperty) {
		OORef<VectorVis> vis = new VectorVis(property->dataset());
		vis->setObjectTitle(tr("Dipoles"));
		vis->loadUserDefaults();
		vis->setEnabled(false);
		vis->setReverseArrowDirection(false);
		vis->setArrowPosition(VectorVis::Center);
		property->addVisElement(vis);
	}
}

/******************************************************************************
* Creates a storage object for standard particle properties.
******************************************************************************/
PropertyPtr ParticleProperty::OOMetaClass::createStandardStorage(size_t particleCount, int type, bool initializeMemory) const
{
	int dataType;
	size_t componentCount;
	size_t stride;

	switch(type) {
	case TypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case CoordinationProperty:
	case MoleculeTypeProperty:
		dataType = PropertyStorage::Int;
		componentCount = 1;
		stride = sizeof(int);
		break;
	case IdentifierProperty:
	case ClusterProperty:
	case MoleculeProperty:
		dataType = PropertyStorage::Int64;
		componentCount = 1;
		stride = sizeof(qlonglong);
		break;
	case PositionProperty:
	case DisplacementProperty:
	case VelocityProperty:
	case ForceProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
	case AsphericalShapeProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = sizeof(Vector3);
		OVITO_ASSERT(stride == sizeof(Point3));
		break;
	case ColorProperty:
	case VectorColorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Color));
		break;
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case SpinProperty:
	case DipoleMagnitudeProperty:
	case CentroSymmetryProperty:
	case DisplacementMagnitudeProperty:
	case VelocityMagnitudeProperty:
		dataType = PropertyStorage::Float;
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case StressTensorProperty:
	case StrainTensorProperty:
	case ElasticStrainTensorProperty:
	case StretchTensorProperty:
		dataType = PropertyStorage::Float;
		componentCount = 6;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(SymmetricTensor2));
		break;
	case DeformationGradientProperty:
	case ElasticDeformationGradientProperty:
		dataType = PropertyStorage::Float;
		componentCount = 9;
		stride = componentCount * sizeof(FloatType);
		break;
	case OrientationProperty:
	case RotationProperty:
		dataType = PropertyStorage::Float;
		componentCount = 4;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Quaternion));
		break;
	case PeriodicImageProperty:
		dataType = PropertyStorage::Int;
		componentCount = 3;
		stride = componentCount * sizeof(int);
		break;
	default:
		OVITO_ASSERT_MSG(false, "ParticleProperty::createStandardStorage()", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard property type: %1").arg(type));
	}

	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));
	
	return std::make_shared<PropertyStorage>(particleCount, dataType, componentCount, stride, 
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Returns the number of particles in the given data state.
******************************************************************************/
size_t ParticleProperty::OOMetaClass::elementCount(const PipelineFlowState& state) const
{
	for(DataObject* obj : state.objects()) {
		if(ParticleProperty* property = dynamic_object_cast<ParticleProperty>(obj)) {
			return property->size();
		}
	}
	return 0;
}

/******************************************************************************
* Determines if the data elements which this property class applies to are 
* present for the given data state.
******************************************************************************/
bool ParticleProperty::OOMetaClass::isDataPresent(const PipelineFlowState& state) const
{
	return state.findObject<ParticleProperty>() != nullptr;
}
	
/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void ParticleProperty::OOMetaClass::initialize()
{
	PropertyClass::initialize();

	// Enable automatic conversion of a ParticlePropertyReference to a generic PropertyReference and vice versa.
	QMetaType::registerConverter<ParticlePropertyReference, PropertyReference>();
	QMetaType::registerConverter<PropertyReference, ParticlePropertyReference>();
	
	setPropertyClassDisplayName(tr("Particles"));
	setElementDescriptionName(QStringLiteral("particles"));
	setPythonName(QStringLiteral("particles"));

	const QStringList emptyList;
	const QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const QStringList rgbList = QStringList() << "R" << "G" << "B";
	const QStringList symmetricTensorList = QStringList() << "XX" << "YY" << "ZZ" << "XY" << "XZ" << "YZ";
	const QStringList tensorList = QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ";
	const QStringList quaternionList = QStringList() << "X" << "Y" << "Z" << "W";
	
	registerStandardProperty(TypeProperty, tr("Particle Type"), PropertyStorage::Int, emptyList, tr("Particle types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), PropertyStorage::Int, emptyList);
	registerStandardProperty(ClusterProperty, tr("Cluster"), PropertyStorage::Int64, emptyList);
	registerStandardProperty(CoordinationProperty, tr("Coordination"), PropertyStorage::Int, emptyList);
	registerStandardProperty(PositionProperty, tr("Position"), PropertyStorage::Float, xyzList, tr("Particle positions"));
	registerStandardProperty(ColorProperty, tr("Color"), PropertyStorage::Float, rgbList, tr("Particle colors"));
	registerStandardProperty(DisplacementProperty, tr("Displacement"), PropertyStorage::Float, xyzList, tr("Displacements"));
	registerStandardProperty(DisplacementMagnitudeProperty, tr("Displacement Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(VelocityProperty, tr("Velocity"), PropertyStorage::Float, xyzList, tr("Velocities"));
	registerStandardProperty(PotentialEnergyProperty, tr("Potential Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(KineticEnergyProperty, tr("Kinetic Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(TotalEnergyProperty, tr("Total Energy"), PropertyStorage::Float, emptyList);
	registerStandardProperty(RadiusProperty, tr("Radius"), PropertyStorage::Float, emptyList, tr("Radii"));
	registerStandardProperty(StructureTypeProperty, tr("Structure Type"), PropertyStorage::Int, emptyList, tr("Structure types"));
	registerStandardProperty(IdentifierProperty, tr("Particle Identifier"), PropertyStorage::Int64, emptyList, tr("Particle identifiers"));
	registerStandardProperty(StressTensorProperty, tr("Stress Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(StrainTensorProperty, tr("Strain Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(DeformationGradientProperty, tr("Deformation Gradient"), PropertyStorage::Float, tensorList);
	registerStandardProperty(OrientationProperty, tr("Orientation"), PropertyStorage::Float, quaternionList);
	registerStandardProperty(ForceProperty, tr("Force"), PropertyStorage::Float, xyzList);
	registerStandardProperty(MassProperty, tr("Mass"), PropertyStorage::Float, emptyList);
	registerStandardProperty(ChargeProperty, tr("Charge"), PropertyStorage::Float, emptyList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), PropertyStorage::Int, xyzList);
	registerStandardProperty(TransparencyProperty, tr("Transparency"), PropertyStorage::Float, emptyList);
	registerStandardProperty(DipoleOrientationProperty, tr("Dipole Orientation"), PropertyStorage::Float, xyzList);
	registerStandardProperty(DipoleMagnitudeProperty, tr("Dipole Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(AngularVelocityProperty, tr("Angular Velocity"), PropertyStorage::Float, xyzList);
	registerStandardProperty(AngularMomentumProperty, tr("Angular Momentum"), PropertyStorage::Float, xyzList);
	registerStandardProperty(TorqueProperty, tr("Torque"), PropertyStorage::Float, xyzList);
	registerStandardProperty(SpinProperty, tr("Spin"), PropertyStorage::Float, emptyList);
	registerStandardProperty(CentroSymmetryProperty, tr("Centrosymmetry"), PropertyStorage::Float, emptyList);
	registerStandardProperty(VelocityMagnitudeProperty, tr("Velocity Magnitude"), PropertyStorage::Float, emptyList);
	registerStandardProperty(MoleculeProperty, tr("Molecule Identifier"), PropertyStorage::Int64, emptyList);
	registerStandardProperty(AsphericalShapeProperty, tr("Aspherical Shape"), PropertyStorage::Float, xyzList);
	registerStandardProperty(VectorColorProperty, tr("Vector Color"), PropertyStorage::Float, rgbList, tr("Vector colors"));
	registerStandardProperty(ElasticStrainTensorProperty, tr("Elastic Strain"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(ElasticDeformationGradientProperty, tr("Elastic Deformation Gradient"), PropertyStorage::Float, tensorList);
	registerStandardProperty(RotationProperty, tr("Rotation"), PropertyStorage::Float, quaternionList);
	registerStandardProperty(StretchTensorProperty, tr("Stretch Tensor"), PropertyStorage::Float, symmetricTensorList);
	registerStandardProperty(MoleculeTypeProperty, tr("Molecule Type"), PropertyStorage::Float, emptyList, tr("Molecule types"));
}

}	// End of namespace
}	// End of namespace
