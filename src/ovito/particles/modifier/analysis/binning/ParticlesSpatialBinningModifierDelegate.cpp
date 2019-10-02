///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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

#include <ovito/particles/Particles.h>
#include <ovito/stdobj/simcell/SimulationCell.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "ParticlesSpatialBinningModifierDelegate.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(ParticlesSpatialBinningModifierDelegate);

/******************************************************************************
* Indicates which data objects in the given input data collection the modifier
* delegate is able to operate on.
******************************************************************************/
QVector<DataObjectReference> ParticlesSpatialBinningModifierDelegate::OOMetaClass::getApplicableObjects(const DataCollection& input) const
{
	if(input.containsObject<ParticlesObject>())
		return { DataObjectReference(&ParticlesObject::OOClass()) };
	return {};
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the
* modifier's results.
******************************************************************************/
std::shared_ptr<SpatialBinningModifierDelegate::SpatialBinningEngine> ParticlesSpatialBinningModifierDelegate::createEngine(
				TimePoint time,
				const PipelineFlowState& input,
                const SimulationCell& cell,
				int binningDirection,
				ConstPropertyPtr sourceProperty,
                size_t sourceComponent,
				ConstPropertyPtr selectionProperty,
                PropertyPtr binData,
				const Vector3I& binCount,
				const Vector3I& binDir,
				int reductionOperation,
                bool computeFirstDerivative)
{
	// Get the particle positions.
    const ParticlesObject* particles = input.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeEngine>(
			input.stateValidity(),
            cell,
            binningDirection,
			std::move(sourceProperty),
            sourceComponent,
			std::move(selectionProperty),
			posProperty->storage(),
            std::move(binData),
            binCount,
            binDir,
            reductionOperation,
            computeFirstDerivative);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ParticlesSpatialBinningModifierDelegate::ComputeEngine::perform()
{
	task()->setProgressText(tr("Spatial binning '%1'").arg(sourceProperty()->name()));

	task()->setProgressValue(0);
	task()->setProgressMaximum(positions()->size());

	if(sourceProperty()->size() == 0)
        return;

    std::vector<size_t> numberOfParticlesPerBin(binData()->size(), 0);
    auto binOneParticle = [this,&numberOfParticlesPerBin](FloatType value, const Point3& pos) {
        Point3I binPos;
        for(size_t dim = 0; dim < 3; dim++) {
            if(binDir(dim) != 3) {
                binPos[dim] = (int)floor(cell().inverseMatrix().prodrow(pos, binDir(dim)) * binCount(dim));
                if(cell().pbcFlags()[binDir(dim)]) binPos[dim] = SimulationCell::modulo(binPos[dim], binCount(dim));
                else if(binPos[dim] < 0 || binPos[dim] >= binCount(dim)) return;
            }
            else binPos[dim] = 0;
        }
        size_t binIndex = (size_t)binPos[2] * (size_t)binCount(0)*(size_t)binCount(1) + (size_t)binPos[1] * (size_t)binCount(0) + (size_t)binPos[0];
        FloatType& binValue = binData()->dataFloat()[binIndex];
        if(reductionOperation() == SpatialBinningModifier::RED_MEAN || reductionOperation() == SpatialBinningModifier::RED_SUM || reductionOperation() == SpatialBinningModifier::RED_SUM_VOL) {
            binValue += value;
        }
        else {
            if(numberOfParticlesPerBin[binIndex] == 0) {
                binValue = value;
            }
            else {
                if(reductionOperation() == SpatialBinningModifier::RED_MAX) {
                    binValue = std::max(binValue, value);
                }
                else if(reductionOperation() == SpatialBinningModifier::RED_MIN) {
                    binValue = std::min(binValue, value);
                }
            }
        }
        numberOfParticlesPerBin[binIndex]++;
    };

    const Point3* pos = positions()->constDataPoint3();
    const int* sel = selectionProperty() ? selectionProperty()->constDataInt() : nullptr;
    size_t vecComponentCount = sourceProperty()->componentCount();
    if(sourceProperty()->dataType() == PropertyStorage::Float) {
        const FloatType* v = sourceProperty()->constDataFloat() + sourceComponent();
        const FloatType* v_end = v + (sourceProperty()->size() * vecComponentCount);
        for(; v != v_end; v += vecComponentCount, ++pos) {
            if(sel && !*sel++) continue;
            if(!std::isnan(*v))
                binOneParticle(*v, *pos);
        }
    }
    else if(sourceProperty()->dataType() == PropertyStorage::Int) {
        const int* v = sourceProperty()->constDataInt() + sourceComponent();
        const int* v_end = v + (sourceProperty()->size() * vecComponentCount);
        for(; v != v_end; v += vecComponentCount, ++pos) {
            if(sel && !*sel++) continue;
            binOneParticle(*v, *pos);
        }
    }
    else if(sourceProperty()->dataType() == PropertyStorage::Int64) {
        auto v = sourceProperty()->constDataInt64() + sourceComponent();
        auto v_end = v + (sourceProperty()->size() * vecComponentCount);
        for(; v != v_end; v += vecComponentCount, ++pos) {
            if(sel && !*sel++) continue;
            binOneParticle(*v, *pos);
        }
    }
    else {
        throw Exception(tr("The input property '%1' has a data type that is not supported by the modifier.").arg(sourceProperty()->name()));
    }

    if(reductionOperation() == SpatialBinningModifier::RED_MEAN) {
        // Normalize.
        auto a = binData()->dataFloat();
        for(auto n : numberOfParticlesPerBin) {
            if(n != 0) *a /= n;
            ++a;
        }
    }
    else if(reductionOperation() == SpatialBinningModifier::RED_SUM_VOL) {
        // Divide by bin volume.
        FloatType cellVolume = cell().is2D() ? cell().volume2D() : cell().volume3D();
        FloatType binVolume = cellVolume / ((FloatType)binCount(0) * (FloatType)binCount(1) * (FloatType)binCount(2));
        for(FloatType& v : binData()->floatRange())
            v /= binVolume;
	}

    // Let the base class compute the first derivative (if requested).
    computeGradient();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
