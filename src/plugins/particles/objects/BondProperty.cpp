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
#include <plugins/particles/objects/BondsVis.h>
#include <core/app/Application.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/PipelineFlowState.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "BondProperty.h"
#include "ParticleProperty.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(BondProperty);

/******************************************************************************
* Constructor.
******************************************************************************/
BondProperty::BondProperty(DataSet* dataset) : PropertyObject(dataset)
{
}

/******************************************************************************
* Gives the property class the opportunity to set up a newly created 
* property object.
******************************************************************************/
void BondProperty::OOMetaClass::prepareNewProperty(PropertyObject* property) const
{
	if(property->type() == BondProperty::TopologyProperty) {
		OORef<BondsVis> vis = new BondsVis(property->dataset());
		if(Application::instance()->guiMode())
			vis->loadUserDefaults();
		property->addVisElement(vis);
	}
}

/******************************************************************************
* Creates a storage object for standard bond properties.
******************************************************************************/
PropertyPtr BondProperty::OOMetaClass::createStandardStorage(size_t bondsCount, int type, bool initializeMemory) const
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
		OVITO_ASSERT_MSG(false, "BondProperty::createStandardStorage", "Invalid standard property type");
		throw Exception(tr("This is not a valid standard bond property type: %1").arg(type));
	}
	const QStringList& componentNames = standardPropertyComponentNames(type);
	const QString& propertyName = standardPropertyName(type);

	OVITO_ASSERT(componentCount == standardPropertyComponentCount(type));
	
	return std::make_shared<PropertyStorage>(bondsCount, dataType, componentCount, stride, 
								propertyName, initializeMemory, type, componentNames);
}

/******************************************************************************
* Returns the number of particles in the given data state.
******************************************************************************/
size_t BondProperty::OOMetaClass::elementCount(const PipelineFlowState& state) const
{
	for(DataObject* obj : state.objects()) {
		if(BondProperty* property = dynamic_object_cast<BondProperty>(obj)) {
			return property->size();
		}
	}
	return 0;
}

/******************************************************************************
* Determines if the data elements which this property class applies to are 
* present for the given data state.
******************************************************************************/
bool BondProperty::OOMetaClass::isDataPresent(const PipelineFlowState& state) const
{
	return state.findObject<BondProperty>() != nullptr;
}

/******************************************************************************
* Registers all standard properties with the property traits class.
******************************************************************************/
void BondProperty::OOMetaClass::initialize()
{
	PropertyClass::initialize();

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
std::pair<size_t, PipelineFlowState> BondProperty::OOMetaClass::elementFromPickResult(const ViewportPickResult& pickResult) const
{
	// Check if a bond was picked.
	if(BondPickInfo* pickInfo = dynamic_object_cast<BondPickInfo>(pickResult.pickInfo())) {

		size_t bondIndex = pickResult.subobjectId() / 2;
		BondProperty* topologyProperty = BondProperty::findInState(pickInfo->pipelineState(), BondProperty::TopologyProperty);
		if(topologyProperty && topologyProperty->size() > bondIndex) {
			return std::make_pair(bondIndex, pickInfo->pipelineState());
		}
	}

	return std::pair<size_t, PipelineFlowState>(std::numeric_limits<size_t>::max(), {});
}

/******************************************************************************
* Tries to remap an index from one data collection to another, considering the 
* possibility that elements may have been added or removed. 
******************************************************************************/
size_t BondProperty::OOMetaClass::remapElementIndex(const PipelineFlowState& sourceState, size_t elementIndex, const PipelineFlowState& destState) const
{
	// Make sure the topology information is present.
	if(PropertyObject* sourceTopology = findInState(sourceState, BondProperty::TopologyProperty)) {
		if(PropertyObject* destTopology = findInState(destState, BondProperty::TopologyProperty)) {

			// If unique IDs are available try to use them to look up the bond in the other data collection. 
			if(PropertyObject* sourceIdentifiers = ParticleProperty::findInState(sourceState, ParticleProperty::IdentifierProperty)) {
				if(PropertyObject* destIdentifiers = ParticleProperty::findInState(destState, ParticleProperty::IdentifierProperty)) {
					size_t index_a = sourceTopology->getInt64Component(elementIndex, 0);
					size_t index_b = sourceTopology->getInt64Component(elementIndex, 1);
					if(index_a < sourceIdentifiers->size() && index_b < sourceIdentifiers->size()) {
						qlonglong id_a = sourceIdentifiers->getInt64(index_a);
						qlonglong id_b = sourceIdentifiers->getInt64(index_b);

						// Quick test if the bond storage order is the same.
						if(elementIndex < destTopology->size()) {
							size_t index2_a = destTopology->getInt64Component(elementIndex, 0);
							size_t index2_b = destTopology->getInt64Component(elementIndex, 1);
							if(index2_a < destIdentifiers->size() && index2_b < destIdentifiers->size()) {
								if(destIdentifiers->getInt64(index2_a) == id_a && destIdentifiers->getInt64(index2_b) == id_b) {
									return elementIndex;
								}
							}
						}

						// Determine the indices of the two particles connected by the bond.
						size_t index2_a = std::find(destIdentifiers->constDataInt64(), destIdentifiers->constDataInt64() + destIdentifiers->size(), id_a) - destIdentifiers->constDataInt64();
						size_t index2_b = std::find(destIdentifiers->constDataInt64(), destIdentifiers->constDataInt64() + destIdentifiers->size(), id_b) - destIdentifiers->constDataInt64();
						if(index2_a < destIdentifiers->size() && index2_b < destIdentifiers->size()) {
							// Go through the whole bonds list to see if there is a bond connecting the particles with 
							// the same IDs.
							for(auto bond = destTopology->constDataInt64(), bond_end = bond + destTopology->size()*2; bond != bond_end; bond += 2) {
								if((bond[0] == index2_a && bond[1] == index2_b) || (bond[0] == index2_b && bond[1] == index2_a)) {
									return (bond - destTopology->constDataInt64()) / 2;
								}
							}
						}
					}

					// Give up.
					return PropertyClass::remapElementIndex(sourceState, elementIndex, destState);
				}
			}

			// Try to find matching bond based on particle indices alone.
			if(PropertyObject* sourcePos = ParticleProperty::findInState(sourceState, ParticleProperty::PositionProperty)) {
				if(PropertyObject* destPos = ParticleProperty::findInState(destState, ParticleProperty::PositionProperty)) {
					size_t index_a = sourceTopology->getInt64Component(elementIndex, 0);
					size_t index_b = sourceTopology->getInt64Component(elementIndex, 1);
					if(index_a < sourcePos->size() && index_b < sourcePos->size()) {

						// Quick check if number of particles and bonds didn't change.
						if(sourcePos->size() == destPos->size() && sourceTopology->size() == destTopology->size()) {
							size_t index2_a = destTopology->getInt64Component(elementIndex, 0);
							size_t index2_b = destTopology->getInt64Component(elementIndex, 1);
							if(index_a == index2_a && index_b == index2_b) {
								return elementIndex;
							}
						}

						// Find matching bond by means of particle positions.
						const Point3& pos_a = sourcePos->getPoint3(index_a);
						const Point3& pos_b = sourcePos->getPoint3(index_b);
						size_t index2_a = std::find(destPos->constDataPoint3(), destPos->constDataPoint3() + destPos->size(), pos_a) - destPos->constDataPoint3();
						size_t index2_b = std::find(destPos->constDataPoint3(), destPos->constDataPoint3() + destPos->size(), pos_b) - destPos->constDataPoint3();
						if(index2_a < destPos->size() && index2_b < destPos->size()) {
							// Go through the whole bonds list to see if there is a bond connecting the particles with 
							// the same positions.
							for(auto bond = destTopology->constDataInt64(), bond_end = bond + destTopology->size()*2; bond != bond_end; bond += 2) {
								if((bond[0] == index2_a && bond[1] == index2_b) || (bond[0] == index2_b && bond[1] == index2_a)) {
									return (bond - destTopology->constDataInt64()) / 2;
								}
							}
						}
					}
				}
			}
		}
	}

	// Give up.
	return PropertyClass::remapElementIndex(sourceState, elementIndex, destState);
}

/******************************************************************************
* Determines which elements are located within the given 
* viewport fence region (=2D polygon).
******************************************************************************/
boost::dynamic_bitset<> BondProperty::OOMetaClass::viewportFenceSelection(const QVector<Point2>& fence, const PipelineFlowState& state, PipelineSceneNode* node, const Matrix4& projectionTM) const
{
	if(PropertyObject* topologyProperty = findInState(state, BondProperty::TopologyProperty)) {
		if(PropertyObject* posProperty = ParticleProperty::findInState(state, ParticleProperty::PositionProperty)) {

			if(!topologyProperty->visElement() || topologyProperty->visElement()->isEnabled() == false)
				node->throwException(tr("Cannot select bonds while the corresponding visual element is disabled. Please enable the display of bonds first."));

			boost::dynamic_bitset<> fullSelection(topologyProperty->size());
			QMutex mutex;
			parallelForChunks(topologyProperty->size(), [topo = topologyProperty->constDataInt64(), posProperty, &projectionTM, &fence, &mutex, &fullSelection](size_t startIndex, size_t chunkSize) {
				boost::dynamic_bitset<> selection(fullSelection.size());
				auto t = topo + startIndex * 2;
				for(size_t index = startIndex; chunkSize != 0; chunkSize--, index++, t += 2) {
					int insideCount = 0;
					for(int i = 0; i < 2; i++) { 
						if(t[i] >= posProperty->size()) continue;
						const Point3& p = posProperty->getPoint3(t[i]);

						// Project particle center to screen coordinates.
						Point3 projPos = projectionTM * p;

						// Perform z-clipping.
						if(std::fabs(projPos.z()) >= FloatType(1))
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

	// Give up.
	return PropertyClass::viewportFenceSelection(fence, state, node, projectionTM);
}

}	// End of namespace
}	// End of namespace
