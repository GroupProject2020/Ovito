////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/stdobj/properties/PropertyAccess.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include <ovito/core/app/Application.h>
#include "POSCARExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(POSCARExporter);
DEFINE_PROPERTY_FIELD(POSCARExporter, writeReducedCoordinates);
SET_PROPERTY_FIELD_LABEL(POSCARExporter, writeReducedCoordinates, "Output reduced coordinates");

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool POSCARExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Get particle positions and velocities.
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	particles->verifyIntegrity();
	ConstPropertyAccess<Point3> posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	ConstPropertyAccess<Vector3> velocityProperty = particles->getProperty(ParticlesObject::VelocityProperty);
	size_t particleCount = particles->elementCount();

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("No simulation cell available. Cannot write POSCAR file."));

	// Write POSCAR header including the simulation cell geometry.
	textStream() << "POSCAR file written by " << Application::applicationName() << " " << Application::applicationVersionString() << "\n";
	textStream() << "1\n";
	const SimulationCell& cell = simulationCell->data();
	for(size_t i = 0; i < 3; i++)
		textStream() << cell.matrix()(0, i) << ' ' << cell.matrix()(1, i) << ' ' << cell.matrix()(2, i) << '\n';
	const Vector3& origin = cell.matrix().translation();

	// Count number of particles per particle type.
	QMap<int,int> particleCounts;
	const PropertyObject* particleTypeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	ConstPropertyAccess<int> particleTypeArray(particleTypeProperty);
	if(particleTypeProperty) {
		for(int ptype : particleTypeArray)
			particleCounts[ptype]++;

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
		textStream() << particleCount << '\n';
		particleCounts[0] = particleCount;
	}

	qlonglong totalProgressCount = particleCount;
	if(velocityProperty) totalProgressCount += particleCount;
	qlonglong currentProgress = 0;
	operation.setProgressMaximum(totalProgressCount);

	// Write atomic positions.
	textStream() << (writeReducedCoordinates() ? "Direct\n" : "Cartesian\n");
	for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
		int ptype = c.key();
		const Point3* p = posProperty.cbegin();
		for(size_t i = 0; i < particleCount; i++, ++p) {
			if(particleTypeArray && particleTypeArray[i] != ptype)
				continue;
			if(writeReducedCoordinates()) {
				Point3 rp = cell.absoluteToReduced(*p);
				textStream() << rp.x() << ' ' << rp.y() << ' ' << rp.z() << '\n';
			}
			else {
				textStream() << (p->x() - origin.x()) << ' ' << (p->y() - origin.y()) << ' ' << (p->z() - origin.z()) << '\n';
			}

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
	}

	// Write atomic velocities.
	if(velocityProperty) {
		textStream() << (writeReducedCoordinates() ? "Direct\n" : "Cartesian\n");
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			int ptype = c.key();
			const Vector3* v = velocityProperty.cbegin();
			for(size_t i = 0; i < particleCount; i++, ++v) {
				if(particleTypeArray && particleTypeArray[i] != ptype)
					continue;

				if(writeReducedCoordinates()) {
					Vector3 rv = cell.absoluteToReduced(*v);
					textStream() << rv.x() << ' ' << rv.y() << ' ' << rv.z() << '\n';
				}
				else {
					textStream() << v->x() << ' ' << v->y() << ' ' << v->z() << '\n';
				}

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
