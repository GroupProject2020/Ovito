////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2013 Alexander Stukowski
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
#include <ovito/core/utilities/concurrent/AsyncOperation.h>
#include "LAMMPSDumpExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(LAMMPSDumpExporter);

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool LAMMPSDumpExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	// Get particles.
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("No simulation cell available. Cannot write LAMMPS file."));

	const AffineTransformation& simCell = simulationCell->cellMatrix();
	size_t atomsCount = particles->elementCount();

	FloatType xlo = simCell.translation().x();
	FloatType ylo = simCell.translation().y();
	FloatType zlo = simCell.translation().z();
	FloatType xhi = simCell.column(0).x() + xlo;
	FloatType yhi = simCell.column(1).y() + ylo;
	FloatType zhi = simCell.column(2).z() + zlo;
	FloatType xy = simCell.column(1).x();
	FloatType xz = simCell.column(2).x();
	FloatType yz = simCell.column(2).y();

	if(simCell.column(0).y() != 0 || simCell.column(0).z() != 0 || simCell.column(1).z() != 0)
		throwException(tr("Cannot save simulation cell to a LAMMPS dump file. This type of non-orthogonal "
				"cell is not supported by LAMMPS and its file format. See the documentation of LAMMPS for details."));

	xlo += std::min((FloatType)0, std::min(xy, std::min(xz, xy+xz)));
	xhi += std::max((FloatType)0, std::max(xy, std::max(xz, xy+xz)));
	ylo += std::min((FloatType)0, yz);
	yhi += std::max((FloatType)0, yz);

	textStream() << "ITEM: TIMESTEP\n";
	textStream() << state.getAttributeValue(QStringLiteral("Timestep"), frameNumber).toInt() << '\n';
	textStream() << "ITEM: NUMBER OF ATOMS\n";
	textStream() << atomsCount << '\n';
	if(xy != 0 || xz != 0 || yz != 0) {
		textStream() << "ITEM: BOX BOUNDS xy xz yz";
		textStream() << (simulationCell->pbcX() ? " pp" : " ff");
		textStream() << (simulationCell->pbcY() ? " pp" : " ff");
		textStream() << (simulationCell->pbcZ() ? " pp" : " ff");
		textStream() << '\n';
		textStream() << xlo << ' ' << xhi << ' ' << xy << '\n';
		textStream() << ylo << ' ' << yhi << ' ' << xz << '\n';
		textStream() << zlo << ' ' << zhi << ' ' << yz << '\n';
	}
	else {
		textStream() << "ITEM: BOX BOUNDS";
		textStream() << (simulationCell->pbcX() ? " pp" : " ff");
		textStream() << (simulationCell->pbcY() ? " pp" : " ff");
		textStream() << (simulationCell->pbcZ() ? " pp" : " ff");
		textStream() << '\n';
		textStream() << xlo << ' ' << xhi << '\n';
		textStream() << ylo << ' ' << yhi << '\n';
		textStream() << zlo << ' ' << zhi << '\n';
	}
	textStream() << "ITEM: ATOMS";

	const OutputColumnMapping& mapping = columnMapping();
	if(mapping.empty())
		throwException(tr("No particle properties have been selected for export to the LAMMPS dump file. Cannot write dump file with zero columns."));

	// Write column names.
	for(int i = 0; i < (int)mapping.size(); i++) {
		const PropertyReference& pref = mapping[i];
		QString columnName;
		switch(pref.type()) {
		case ParticlesObject::PositionProperty:
			if(pref.vectorComponent() == 0) columnName = QStringLiteral("x");
			else if(pref.vectorComponent() == 1) columnName = QStringLiteral("y");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("z");
			else columnName = QStringLiteral("position");
			break;
		case ParticlesObject::VelocityProperty:
			if(pref.vectorComponent() == 0) columnName = QStringLiteral("vx");
			else if(pref.vectorComponent() == 1) columnName = QStringLiteral("vy");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("vz");
			else columnName = QStringLiteral("velocity");
			break;
		case ParticlesObject::ForceProperty:
			if(pref.vectorComponent() == 0) columnName = QStringLiteral("fx");
			else if(pref.vectorComponent() == 1) columnName = QStringLiteral("fy");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("fz");
			else columnName = QStringLiteral("force");
			break;
		case ParticlesObject::PeriodicImageProperty:
			if(pref.vectorComponent() == 0) columnName = QStringLiteral("ix");
			else if(pref.vectorComponent() == 1) columnName = QStringLiteral("iy");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("iz");
			else columnName = QStringLiteral("pbcimage");
			break;
		case ParticlesObject::IdentifierProperty: columnName = QStringLiteral("id"); break;
		case ParticlesObject::TypeProperty: columnName = QStringLiteral("type"); break;
		case ParticlesObject::MassProperty: columnName = QStringLiteral("mass"); break;
		case ParticlesObject::SelectionProperty: columnName = QStringLiteral("selection"); break;
		case ParticlesObject::RadiusProperty: columnName = QStringLiteral("radius"); break;
		case ParticlesObject::MoleculeProperty: columnName = QStringLiteral("mol"); break;
		case ParticlesObject::ChargeProperty: columnName = QStringLiteral("q"); break;
		case ParticlesObject::PotentialEnergyProperty: columnName = QStringLiteral("c_epot"); break;
		case ParticlesObject::KineticEnergyProperty: columnName = QStringLiteral("c_kpot"); break;
		case ParticlesObject::OrientationProperty:
			if(pref.vectorComponent() == 0) columnName = QStringLiteral("c_orient[1]");
			else if(pref.vectorComponent() == 1) columnName = QStringLiteral("c_orient[2]");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("c_orient[3]");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("c_orient[4]");
			else columnName = QStringLiteral("orientation");
			break;
		case ParticlesObject::AsphericalShapeProperty:
			if(pref.vectorComponent() == 0) columnName = QStringLiteral("c_shape[1]");
			else if(pref.vectorComponent() == 1) columnName = QStringLiteral("c_shape[2]");
			else if(pref.vectorComponent() == 2) columnName = QStringLiteral("c_shape[3]");
			else columnName = QStringLiteral("aspherical_shape");
			break;
		default:
			columnName = pref.nameWithComponent();
			columnName.remove(QRegExp("[^A-Za-z\\d_]"));
		}
		textStream() << ' ' << columnName;
	}
	textStream() << '\n';

	OutputColumnWriter columnWriter(mapping, state);
	operation.setProgressMaximum(atomsCount);
	for(size_t i = 0; i < atomsCount; i++) {
		columnWriter.writeParticle(i, textStream());

		if(!operation.setProgressValueIntermittent(i))
			return false;
	}

	return !operation.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
