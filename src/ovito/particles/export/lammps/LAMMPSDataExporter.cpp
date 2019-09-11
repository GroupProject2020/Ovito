///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2019) Alexander Stukowski
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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/particles/objects/BondsObject.h>
#include <ovito/particles/objects/ParticleType.h>
#include <ovito/stdobj/simcell/SimulationCellObject.h>
#include <ovito/core/utilities/concurrent/Promise.h>
#include "LAMMPSDataExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(LAMMPSDataExporter);
DEFINE_PROPERTY_FIELD(LAMMPSDataExporter, atomStyle);
SET_PROPERTY_FIELD_LABEL(LAMMPSDataExporter, atomStyle, "Atom style");

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool LAMMPSDataExporter::exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation)
{
	const ParticlesObject* particles = state.expectObject<ParticlesObject>();
	const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
	const PropertyObject* velocityProperty = particles->getProperty(ParticlesObject::VelocityProperty);
	const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
	const PropertyObject* periodicImageProperty = particles->getProperty(ParticlesObject::PeriodicImageProperty);
	const PropertyObject* particleTypeProperty = particles->getProperty(ParticlesObject::TypeProperty);
	const PropertyObject* chargeProperty = particles->getProperty(ParticlesObject::ChargeProperty);
	const PropertyObject* radiusProperty = particles->getProperty(ParticlesObject::RadiusProperty);
	const PropertyObject* massProperty = particles->getProperty(ParticlesObject::MassProperty);
	const PropertyObject* moleculeProperty = particles->getProperty(ParticlesObject::MoleculeProperty);
	const PropertyObject* dipoleOrientationProperty = particles->getProperty(ParticlesObject::DipoleOrientationProperty);
	const BondsObject* bonds = particles->bonds();
	const PropertyObject* bondTopologyProperty = bonds ? bonds->getProperty(BondsObject::TopologyProperty) : nullptr;
	const PropertyObject* bondTypeProperty = bonds ? bonds->getProperty(BondsObject::TypeProperty) : nullptr;

	// Get simulation cell info.
	const SimulationCellObject* simulationCell = state.getObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("No simulation cell defined. Cannot write LAMMPS file."));

	const AffineTransformation& simCell = simulationCell->cellMatrix();

	// Transform triclinic cell to LAMMPS canonical format.
	Vector3 a,b,c;
	AffineTransformation transformation;
	bool transformCoordinates;
	if(simCell.column(0).y() != 0 || simCell.column(0).z() != 0 || simCell.column(1).z() != 0) {
		a.x() = simCell.column(0).length();
		a.y() = a.z() = 0;
		b.x() = simCell.column(1).dot(simCell.column(0)) / a.x();
		b.y() = sqrt(simCell.column(1).squaredLength() - b.x()*b.x());
		b.z() = 0;
		c.x() = simCell.column(2).dot(simCell.column(0)) / a.x();
		c.y() = (simCell.column(1).dot(simCell.column(2)) - b.x()*c.x()) / b.y();
		c.z() = sqrt(simCell.column(2).squaredLength() - c.x()*c.x() - c.y()*c.y());
		transformCoordinates = true;
		transformation = AffineTransformation(a,b,c,simCell.translation()) * simCell.inverse();
	}
	else {
		a = simCell.column(0);
		b = simCell.column(1);
		c = simCell.column(2);
		transformCoordinates = false;
	}

	FloatType xlo = simCell.translation().x();
	FloatType ylo = simCell.translation().y();
	FloatType zlo = simCell.translation().z();
	FloatType xhi = a.x() + xlo;
	FloatType yhi = b.y() + ylo;
	FloatType zhi = c.z() + zlo;
	FloatType xy = b.x();
	FloatType xz = c.x();
	FloatType yz = c.y();

	// Decide if we want to export bonds.
	bool writeBonds = (bondTopologyProperty != nullptr) && (atomStyle() != LAMMPSDataImporter::AtomStyle_Atomic);

	textStream() << "# LAMMPS data file written by " << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << "\n";
	textStream() << posProperty->size() << " atoms\n";
	if(writeBonds)
		textStream() << bondTopologyProperty->size() << " bonds\n";

	if(particleTypeProperty && particleTypeProperty->size() > 0) {
		int numParticleTypes = std::max(
				particleTypeProperty->elementTypes().size(),
				*std::max_element(particleTypeProperty->constDataInt(), particleTypeProperty->constDataInt() + particleTypeProperty->size()));
		textStream() << numParticleTypes << " atom types\n";
	}
	else textStream() << "1 atom types\n";
	if(writeBonds) {
		if(bondTypeProperty && bondTypeProperty->size() > 0) {
			int numBondTypes = std::max(
					bondTypeProperty->elementTypes().size(),
					*std::max_element(bondTypeProperty->constDataInt(), bondTypeProperty->constDataInt() + bondTypeProperty->size()));
			textStream() << numBondTypes << " bond types\n";
		}
		else textStream() << "1 bond types\n";
	}

	textStream() << xlo << ' ' << xhi << " xlo xhi\n";
	textStream() << ylo << ' ' << yhi << " ylo yhi\n";
	textStream() << zlo << ' ' << zhi << " zlo zhi\n";
	if(xy != 0 || xz != 0 || yz != 0) {
		textStream() << xy << ' ' << xz << ' ' << yz << " xy xz yz\n";
	}
	textStream() << "\n";

	// Write "Masses" section.
	if(particleTypeProperty && particleTypeProperty->elementTypes().size() > 0) {
		textStream() << "Masses\n\n";
		for(const ElementType* type : particleTypeProperty->elementTypes()) {
			if(const ParticleType* ptype = dynamic_object_cast<ParticleType>(type)) {
				textStream() << ptype->numericId() << " " << ptype->mass();
				if(!ptype->name().isEmpty())
					textStream() << "  # " << ptype->name();
				textStream() << "\n";
			}
		}
		textStream() << "\n";
	}

	qlonglong totalProgressCount = posProperty->size();
	if(velocityProperty) totalProgressCount += posProperty->size();
	if(writeBonds) totalProgressCount += bondTopologyProperty->size();

	// Write "Atoms" section.
	textStream() << "Atoms";
	switch(atomStyle()) {
		case LAMMPSDataImporter::AtomStyle_Atomic: textStream() << "  # atomic"; break;
		case LAMMPSDataImporter::AtomStyle_Angle: textStream() << "  # angle"; break;
		case LAMMPSDataImporter::AtomStyle_Bond: textStream() << "  # bond"; break;
		case LAMMPSDataImporter::AtomStyle_Molecular: textStream() << "  # molecular"; break;
		case LAMMPSDataImporter::AtomStyle_Full: textStream() << "  # full"; break;
		case LAMMPSDataImporter::AtomStyle_Charge: textStream() << "  # charge"; break;
		case LAMMPSDataImporter::AtomStyle_Dipole: textStream() << "  # dipole"; break;
		case LAMMPSDataImporter::AtomStyle_Sphere: textStream() << "  # sphere"; break;
		default: break; // Do nothing
	}
	textStream() << "\n\n";

	operation.setProgressMaximum(totalProgressCount);
	qlonglong currentProgress = 0;
	for(size_t i = 0; i < posProperty->size(); i++) {
		// atom-ID
		textStream() << (identifierProperty ? identifierProperty->getInt64(i) : (i+1));
		if(atomStyle() == LAMMPSDataImporter::AtomStyle_Bond || atomStyle() == LAMMPSDataImporter::AtomStyle_Molecular || atomStyle() == LAMMPSDataImporter::AtomStyle_Full || atomStyle() == LAMMPSDataImporter::AtomStyle_Angle) {
			textStream() << ' ';
			// molecule-ID
			textStream() << (moleculeProperty ? moleculeProperty->getInt64(i) : 1);
		}
		textStream() << ' ';
		// atom-type
		textStream() << (particleTypeProperty ? particleTypeProperty->getInt(i) : 1);
		if(atomStyle() == LAMMPSDataImporter::AtomStyle_Charge || atomStyle() == LAMMPSDataImporter::AtomStyle_Dipole || atomStyle() == LAMMPSDataImporter::AtomStyle_Full) {
			textStream() << ' ';
			// charge
			textStream() << (chargeProperty ? chargeProperty->getFloat(i) : 0);
		}
		else if(atomStyle() == LAMMPSDataImporter::AtomStyle_Sphere) {
			// diameter
			FloatType radius = (radiusProperty ? radiusProperty->getFloat(i) : 0);
			textStream() << ' ' << (radius*2);
			// density
			FloatType density = (massProperty ? massProperty->getFloat(i) : 0);
			if(radius > 0) density /= pow(radius, 3) * (FLOATTYPE_PI * FloatType(4) / FloatType(3));
			textStream() << ' ' << density;
		}
		// x y z
		const Point3& pos = posProperty->getPoint3(i);
		if(!transformCoordinates) {
			for(size_t k = 0; k < 3; k++)
				textStream() << ' ' << pos[k];
		}
		else {
			for(size_t k = 0; k < 3; k++)
				textStream() << ' ' << transformation.prodrow(pos, k);
		}
		if(atomStyle() == LAMMPSDataImporter::AtomStyle_Dipole) {
			// mux muy muz
			textStream() << ' ' << (dipoleOrientationProperty ? dipoleOrientationProperty->getFloatComponent(i, 0) : 0);
			textStream() << ' ' << (dipoleOrientationProperty ? dipoleOrientationProperty->getFloatComponent(i, 1) : 0);
			textStream() << ' ' << (dipoleOrientationProperty ? dipoleOrientationProperty->getFloatComponent(i, 2) : 0);
		}
		if(periodicImageProperty) {
			// pbc images
			const Point3I& pbc = periodicImageProperty->getPoint3I(i);
			for(size_t k = 0; k < 3; k++) {
				textStream() << ' ' << pbc[k];
			}
		}
		textStream() << '\n';

		if(!operation.setProgressValueIntermittent(currentProgress++))
			return false;
	}

	// Write velocities.
	if(velocityProperty) {
		textStream() << "\nVelocities\n\n";
		const Vector3* v = velocityProperty->constDataVector3();
		for(size_t i = 0; i < velocityProperty->size(); i++, ++v) {
			textStream() << (identifierProperty ? identifierProperty->getInt64(i) : (i+1));
			if(!transformCoordinates) {
				for(size_t k = 0; k < 3; k++)
					textStream() << ' ' << (*v)[k];
			}
			else {
				for(size_t k = 0; k < 3; k++)
					textStream() << ' ' << transformation.prodrow(*v, k);
			}
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
	}

	// Write bonds.
	if(writeBonds) {
		textStream() << "\nBonds\n\n";

		size_t bondIndex = 1;
		for(size_t i = 0; i < bondTopologyProperty->size(); i++) {
			size_t atomIndex1 = bondTopologyProperty->getInt64Component(i, 0);
			size_t atomIndex2 = bondTopologyProperty->getInt64Component(i, 1);
			textStream() << bondIndex++;
			textStream() << ' ';
			textStream() << (bondTypeProperty ? bondTypeProperty->getInt(i) : 1);
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty->getInt64(atomIndex1) : (atomIndex1+1));
			textStream() << ' ';
			textStream() << (identifierProperty ? identifierProperty->getInt64(atomIndex2) : (atomIndex2+1));
			textStream() << '\n';

			if(!operation.setProgressValueIntermittent(currentProgress++))
				return false;
		}
		OVITO_ASSERT(bondIndex == bondTopologyProperty->size() + 1);
	}

	return !operation.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
