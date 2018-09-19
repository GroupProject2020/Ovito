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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/utilities/concurrent/Promise.h>
#include "POSCARExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(POSCARExporter);

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool POSCARExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{	
	// Get particle positions and velocities.
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* velocityProperty = particles->getProperty(ParticlesObject::VelocityProperty);

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("No simulation cell available. Cannot write POSCAR file."));

	// Write POSCAR header including the simulation cell geometry.
	textStream() << "POSCAR file written by OVITO\n";
	textStream() << "1\n";
	const AffineTransformation& cell = simulationCell->cellMatrix();
	for(size_t i = 0; i < 3; i++)
		textStream() << cell(0, i) << ' ' << cell(1, i) << ' ' << cell(2, i) << '\n';
	Vector3 origin = cell.translation();

	// Count number of particles per particle type.
	QMap<int,int> particleCounts;
	const PropertyObject* particleTypeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	if(particleTypeProperty) {
		const int* ptype = particleTypeProperty->constDataInt();
		const int* ptype_end = ptype + particleTypeProperty->size();
		for(; ptype != ptype_end; ++ptype) {
			particleCounts[*ptype]++;
		}

		// Write line with particle type names.
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			const ElementType* particleType = particleTypeProperty->elementType(c.key());
			if(particleType) {
				QString typeName = particleType->nameOrNumericId();
				typeName.replace(' ', '_');
				textStream() << typeName << ' ';
			}
			else textStream() << "Type" << c.key() << ' ';
		}
		textStream() << '\n';

		// Write line with particle counts per type.
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			textStream() << c.value() << ' ';
		}
		textStream() << '\n';
	}
	else {
		// Write line with particle type name.
		textStream() << "A\n";
		// Write line with particle count.
		textStream() << posProperty->size() << '\n';
		particleCounts[0] = posProperty->size();
	}

	qlonglong totalProgressCount = posProperty->size();
	if(velocityProperty) totalProgressCount += posProperty->size();
	qlonglong currentProgress = 0;
	operation.setProgressMaximum(totalProgressCount);

	// Write atomic positions.
	textStream() << "Cartesian\n";
	for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
		int ptype = c.key();
		const Point3* p = posProperty->constDataPoint3();
		for(size_t i = 0; i < posProperty->size(); i++, ++p) {
			if(particleTypeProperty && particleTypeProperty->getInt(i) != ptype)
				continue;
			textStream() << (p->x() - origin.x()) << ' ' << (p->y() - origin.y()) << ' ' << (p->z() - origin.z()) << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
	}

	// Write atomic velocities.
	if(velocityProperty) {
		textStream() << "Cartesian\n";
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			int ptype = c.key();
			const Vector3* v = velocityProperty->constDataVector3();
			for(size_t i = 0; i < velocityProperty->size(); i++, ++v) {
				if(particleTypeProperty && particleTypeProperty->getInt(i) != ptype)
					continue;
				textStream() << v->x() << ' ' << v->y() << ' ' << v->z() << '\n';

				if(!operation.setProgressValueIntermittent(currentProgress++))
					return false;
			}
		}
	}

	return !operation.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
