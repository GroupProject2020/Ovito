////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/Promise.h>
#include "FHIAimsExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(FHIAimsExporter);

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool FHIAimsExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Get particle positions and types.
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* particleTypeProperty = particles->getProperty(ParticlesObject::TypeProperty);

	textStream() << "# FHI-aims file written by " << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << "\n";

	// Output simulation cell.
	Point3 origin = Point3::Origin();
	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();
	if(simulationCell) {
		origin = simulationCell->cellOrigin();
		if(simulationCell->pbcX() || simulationCell->pbcY() || simulationCell->pbcZ()) {
			const AffineTransformation& cell = simulationCell->cellMatrix();
			for(size_t i = 0; i < 3; i++)
				textStream() << "lattice_vector " << cell(0, i) << ' ' << cell(1, i) << ' ' << cell(2, i) << '\n';
		}
	}

	// Output atoms.
	operation.setProgressMaximum(posProperty->size());
	for(size_t i = 0; i < posProperty->size(); i++) {
		const Point3& p = posProperty->getPoint3(i);
		const ElementType* type = particleTypeProperty ? particleTypeProperty->elementType(particleTypeProperty->getInt(i)) : nullptr;

		textStream() << "atom " << (p.x() - origin.x()) << ' ' << (p.y() - origin.y()) << ' ' << (p.z() - origin.z());
		if(type && !type->name().isEmpty()) {
			QString s = type->name();
			textStream() << ' ' << s.replace(QChar(' '), QChar('_')) << '\n';
		}
		else if(particleTypeProperty) {
			textStream() << ' ' << particleTypeProperty->getInt(i) << '\n';
		}
		else {
			textStream() << " 1\n";
		}

		if(!operation.setProgressValueIntermittent(i))
			return false;
	}

	return !operation.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
