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
#include "ParticleDisplay.h"
#include "VectorDisplay.h"

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
		OORef<ParticleDisplay> displayObj = new ParticleDisplay(property->dataset());
		displayObj->loadUserDefaults();
		property->addDisplayObject(displayObj);
	}
	else if(property->type() == ParticleProperty::DisplacementProperty) {
		OORef<VectorDisplay> displayObj = new VectorDisplay(property->dataset());
		displayObj->setObjectTitle(tr("Displacements"));
		displayObj->loadUserDefaults();
		displayObj->setEnabled(false);
		property->addDisplayObject(displayObj);
	}
	else if(property->type() == ParticleProperty::ForceProperty) {
		OORef<VectorDisplay> displayObj = new VectorDisplay(property->dataset());
		displayObj->setObjectTitle(tr("Forces"));
		displayObj->loadUserDefaults();
		displayObj->setEnabled(false);
		displayObj->setReverseArrowDirection(false);
		displayObj->setArrowPosition(VectorDisplay::Base);
		property->addDisplayObject(displayObj);
	}
	else if(property->type() == ParticleProperty::DipoleOrientationProperty) {
		OORef<VectorDisplay> displayObj = new VectorDisplay(property->dataset());
		displayObj->setObjectTitle(tr("Dipoles"));
		displayObj->loadUserDefaults();
		displayObj->setEnabled(false);
		displayObj->setReverseArrowDirection(false);
		displayObj->setArrowPosition(VectorDisplay::Center);
		property->addDisplayObject(displayObj);
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
	case ClusterProperty:
	case CoordinationProperty:
	case IdentifierProperty:
	case MoleculeProperty:
	case MoleculeTypeProperty:
		dataType = qMetaTypeId<int>();
		componentCount = 1;
		stride = sizeof(int);
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
		dataType = qMetaTypeId<FloatType>();
		componentCount = 3;
		stride = sizeof(Vector3);
		OVITO_ASSERT(stride == sizeof(Point3));
		break;
	case ColorProperty:
	case VectorColorProperty:
		dataType = qMetaTypeId<FloatType>();
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
		dataType = qMetaTypeId<FloatType>();
		componentCount = 1;
		stride = sizeof(FloatType);
		break;
	case StressTensorProperty:
	case StrainTensorProperty:
	case ElasticStrainTensorProperty:
	case StretchTensorProperty:
		dataType = qMetaTypeId<FloatType>();
		componentCount = 6;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(SymmetricTensor2));
		break;
	case DeformationGradientProperty:
	case ElasticDeformationGradientProperty:
		dataType = qMetaTypeId<FloatType>();
		componentCount = 9;
		stride = componentCount * sizeof(FloatType);
		break;
	case OrientationProperty:
	case RotationProperty:
		dataType = qMetaTypeId<FloatType>();
		componentCount = 4;
		stride = componentCount * sizeof(FloatType);
		OVITO_ASSERT(stride == sizeof(Quaternion));
		break;
	case PeriodicImageProperty:
		dataType = qMetaTypeId<int>();
		componentCount = 3;
		stride = componentCount * sizeof(FloatType);
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
	
	registerStandardProperty(TypeProperty, tr("Particle Type"), qMetaTypeId<int>(), emptyList, tr("Particle types"));
	registerStandardProperty(SelectionProperty, tr("Selection"), qMetaTypeId<int>(), emptyList);
	registerStandardProperty(ClusterProperty, tr("Cluster"), qMetaTypeId<int>(), emptyList);
	registerStandardProperty(CoordinationProperty, tr("Coordination"), qMetaTypeId<int>(), emptyList);
	registerStandardProperty(PositionProperty, tr("Position"), qMetaTypeId<FloatType>(), xyzList, tr("Particle positions"));
	registerStandardProperty(ColorProperty, tr("Color"), qMetaTypeId<FloatType>(), rgbList, tr("Particle colors"));
	registerStandardProperty(DisplacementProperty, tr("Displacement"), qMetaTypeId<FloatType>(), xyzList, tr("Displacements"));
	registerStandardProperty(DisplacementMagnitudeProperty, tr("Displacement Magnitude"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(VelocityProperty, tr("Velocity"), qMetaTypeId<FloatType>(), xyzList, tr("Velocities"));
	registerStandardProperty(PotentialEnergyProperty, tr("Potential Energy"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(KineticEnergyProperty, tr("Kinetic Energy"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(TotalEnergyProperty, tr("Total Energy"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(RadiusProperty, tr("Radius"), qMetaTypeId<FloatType>(), emptyList, tr("Radii"));
	registerStandardProperty(StructureTypeProperty, tr("Structure Type"), qMetaTypeId<int>(), emptyList, tr("Structure types"));
	registerStandardProperty(IdentifierProperty, tr("Particle Identifier"), qMetaTypeId<int>(), emptyList, tr("Particle identifiers"));
	registerStandardProperty(StressTensorProperty, tr("Stress Tensor"), qMetaTypeId<FloatType>(), symmetricTensorList);
	registerStandardProperty(StrainTensorProperty, tr("Strain Tensor"), qMetaTypeId<FloatType>(), symmetricTensorList);
	registerStandardProperty(DeformationGradientProperty, tr("Deformation Gradient"), qMetaTypeId<FloatType>(), tensorList);
	registerStandardProperty(OrientationProperty, tr("Orientation"), qMetaTypeId<FloatType>(), quaternionList);
	registerStandardProperty(ForceProperty, tr("Force"), qMetaTypeId<FloatType>(), xyzList);
	registerStandardProperty(MassProperty, tr("Mass"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(ChargeProperty, tr("Charge"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(PeriodicImageProperty, tr("Periodic Image"), qMetaTypeId<int>(), xyzList);
	registerStandardProperty(TransparencyProperty, tr("Transparency"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(DipoleOrientationProperty, tr("Dipole Orientation"), qMetaTypeId<FloatType>(), xyzList);
	registerStandardProperty(DipoleMagnitudeProperty, tr("Dipole Magnitude"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(AngularVelocityProperty, tr("Angular Velocity"), qMetaTypeId<FloatType>(), xyzList);
	registerStandardProperty(AngularMomentumProperty, tr("Angular Momentum"), qMetaTypeId<FloatType>(), xyzList);
	registerStandardProperty(TorqueProperty, tr("Torque"), qMetaTypeId<FloatType>(), xyzList);
	registerStandardProperty(SpinProperty, tr("Spin"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(CentroSymmetryProperty, tr("Centrosymmetry"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(VelocityMagnitudeProperty, tr("Velocity Magnitude"), qMetaTypeId<FloatType>(), emptyList);
	registerStandardProperty(MoleculeProperty, tr("Molecule Identifier"), qMetaTypeId<int>(), emptyList);
	registerStandardProperty(AsphericalShapeProperty, tr("Aspherical Shape"), qMetaTypeId<FloatType>(), xyzList);
	registerStandardProperty(VectorColorProperty, tr("Vector Color"), qMetaTypeId<FloatType>(), rgbList, tr("Vector colors"));
	registerStandardProperty(ElasticStrainTensorProperty, tr("Elastic Strain"), qMetaTypeId<FloatType>(), symmetricTensorList);
	registerStandardProperty(ElasticDeformationGradientProperty, tr("Elastic Deformation Gradient"), qMetaTypeId<FloatType>(), tensorList);
	registerStandardProperty(RotationProperty, tr("Rotation"), qMetaTypeId<FloatType>(), quaternionList);
	registerStandardProperty(StretchTensorProperty, tr("Stretch Tensor"), qMetaTypeId<FloatType>(), symmetricTensorList);
	registerStandardProperty(MoleculeTypeProperty, tr("Molecule Type"), qMetaTypeId<FloatType>(), emptyList, tr("Molecule types"));
}

}	// End of namespace
}	// End of namespace
