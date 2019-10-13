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
#include "IMDExporter.h"
#include "../OutputColumnMapping.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(IMDExporter);

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool IMDExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* typeProperty = nullptr;
	const PropertyObject* identifierProperty = nullptr;
	const PropertyObject* velocityProperty = nullptr;
	const PropertyObject* massProperty = nullptr;

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.expectObject<SimulationCellObject>();

	const AffineTransformation& simCell = simulationCell->cellMatrix();
	size_t atomsCount = posProperty->size();

	OutputColumnMapping colMapping;
	OutputColumnMapping filteredMapping;
	posProperty = nullptr;
	bool exportIdentifiers = false;
	for(const ParticlePropertyReference& pref : columnMapping()) {
		if(pref.type() == ParticlesObject::PositionProperty) {
			posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
		}
		else if(pref.type() == ParticlesObject::TypeProperty) {
			typeProperty = particles->expectProperty(ParticlesObject::TypeProperty);
		}
		else if(pref.type() == ParticlesObject::IdentifierProperty) {
			identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
			exportIdentifiers = true;
		}
		else if(pref.type() == ParticlesObject::VelocityProperty) {
			velocityProperty = particles->expectProperty(ParticlesObject::VelocityProperty);
		}
		else if(pref.type() == ParticlesObject::MassProperty) {
			massProperty = particles->expectProperty(ParticlesObject::MassProperty);
		}
		else filteredMapping.push_back(pref);
	}

	QVector<QString> columnNames;
	textStream() << "#F A ";
	if(exportIdentifiers) {
		if(identifierProperty) {
			textStream() << "1 ";
			colMapping.emplace_back(identifierProperty);
			columnNames.push_back("number");
		}
		else {
			textStream() << "1 ";
			colMapping.emplace_back(ParticlesObject::IdentifierProperty);
			columnNames.push_back("number");
		}
	}
	else textStream() << "0 ";
	if(typeProperty) {
		textStream() << "1 ";
		colMapping.emplace_back(typeProperty);
		columnNames.push_back("type");
	}
	else textStream() << "0 ";
	if(massProperty) {
		textStream() << "1 ";
		colMapping.emplace_back(massProperty);
		columnNames.push_back("mass");
	}
	else textStream() << "0 ";
	if(posProperty) {
		textStream() << "3 ";
		colMapping.emplace_back(posProperty, 0);
		colMapping.emplace_back(posProperty, 1);
		colMapping.emplace_back(posProperty, 2);
		columnNames.push_back("x");
		columnNames.push_back("y");
		columnNames.push_back("z");
	}
	else textStream() << "0 ";
	if(velocityProperty) {
		textStream() << "3 ";
		colMapping.emplace_back(velocityProperty, 0);
		colMapping.emplace_back(velocityProperty, 1);
		colMapping.emplace_back(velocityProperty, 2);
		columnNames.push_back("vx");
		columnNames.push_back("vy");
		columnNames.push_back("vz");
	}
	else textStream() << "0 ";

	for(int i = 0; i < (int)filteredMapping.size(); i++) {
		const PropertyReference& pref = filteredMapping[i];
		QString columnName = pref.nameWithComponent();
		columnName.remove(QRegExp("[^A-Za-z\\d_.]"));
		columnNames.push_back(columnName);
		colMapping.push_back(pref);
	}
	textStream() << filteredMapping.size() << "\n";

	textStream() << "#C";
	for(const QString& cname : columnNames)
		textStream() << " " << cname;
	textStream() << "\n";

	textStream() << "#X " << simCell.column(0)[0] << " " << simCell.column(0)[1] << " " << simCell.column(0)[2] << "\n";
	textStream() << "#Y " << simCell.column(1)[0] << " " << simCell.column(1)[1] << " " << simCell.column(1)[2] << "\n";
	textStream() << "#Z " << simCell.column(2)[0] << " " << simCell.column(2)[1] << " " << simCell.column(2)[2] << "\n";

	textStream() << "## Generated on " << QDateTime::currentDateTime().toString() << "\n";
	textStream() << "## IMD file written by " << QCoreApplication::applicationName() << "\n";
	textStream() << "#E\n";

	operation.setProgressMaximum(atomsCount);
	OutputColumnWriter columnWriter(colMapping, state);
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
