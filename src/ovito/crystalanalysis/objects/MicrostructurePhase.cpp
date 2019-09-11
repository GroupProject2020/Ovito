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

#include <ovito/crystalanalysis/CrystalAnalysis.h>
#include "MicrostructurePhase.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_CLASS(MicrostructurePhase);
DEFINE_PROPERTY_FIELD(MicrostructurePhase, shortName);
DEFINE_PROPERTY_FIELD(MicrostructurePhase, dimensionality);
DEFINE_PROPERTY_FIELD(MicrostructurePhase, crystalSymmetryClass);
DEFINE_REFERENCE_FIELD(MicrostructurePhase, burgersVectorFamilies);
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, shortName, "Short name");
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, dimensionality, "Dimensionality");
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, crystalSymmetryClass, "Symmetry class");
SET_PROPERTY_FIELD_LABEL(MicrostructurePhase, burgersVectorFamilies, "Burgers vector families");

/******************************************************************************
* Constructs the MicrostructurePhase object.
******************************************************************************/
MicrostructurePhase::MicrostructurePhase(DataSet* dataset) : ElementType(dataset),
	_dimensionality(Dimensionality::None),
	_crystalSymmetryClass(CrystalSymmetryClass::NoSymmetry)
{
}

/******************************************************************************
* Returns the display color to be used for a given Burgers vector.
******************************************************************************/
Color MicrostructurePhase::getBurgersVectorColor(const QString& latticeName, const Vector3& b)
{
	if(latticeName == ParticleType::getPredefinedStructureTypeName(ParticleType::PredefinedStructureType::BCC)) {
		return getBurgersVectorColor(ParticleType::PredefinedStructureType::BCC, b);
	}
	else if(latticeName == ParticleType::getPredefinedStructureTypeName(ParticleType::PredefinedStructureType::FCC)) {
		return getBurgersVectorColor(ParticleType::PredefinedStructureType::FCC, b);
	}
	return getBurgersVectorColor(ParticleType::PredefinedStructureType::OTHER, b);
}

/******************************************************************************
* Returns the display color to be used for a given Burgers vector.
******************************************************************************/
Color MicrostructurePhase::getBurgersVectorColor(ParticleType::PredefinedStructureType structureType, const Vector3& b)
{
	if(structureType == ParticleType::PredefinedStructureType::BCC) {
		static const Color predefinedLineColors[] = {
				Color(0.4f,1.0f,0.4f),
				Color(1.0f,0.2f,0.2f),
				Color(0.4f,0.4f,1.0f),
				Color(0.9f,0.5f,0.0f),
				Color(1.0f,1.0f,0.0f),
				Color(1.0f,0.4f,1.0f),
				Color(0.7f,0.0f,1.0f)
		};
		static const Vector3 burgersVectors[] = {
				{ FloatType(0.5), FloatType(0.5), FloatType(0.5) },
				{ FloatType(-0.5), FloatType(0.5), FloatType(0.5) },
				{ FloatType(0.5), FloatType(-0.5), FloatType(0.5) },
				{ FloatType(0.5), FloatType(0.5), FloatType(-0.5) },
				{ FloatType(1.0), FloatType(0.0), FloatType(0.0) },
				{ FloatType(0.0), FloatType(1.0), FloatType(0.0) },
				{ FloatType(0.0), FloatType(0.0), FloatType(1.0) }
		};
		OVITO_STATIC_ASSERT(sizeof(burgersVectors)/sizeof(burgersVectors[0]) == sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(burgersVectors)/sizeof(burgersVectors[0]); i++) {
			if(b.equals(burgersVectors[i], FloatType(1e-6)) || b.equals(-burgersVectors[i], FloatType(1e-6)))
				return predefinedLineColors[i];
		}
	}
	else if(structureType == ParticleType::PredefinedStructureType::FCC) {
		static const Color predefinedLineColors[] = {
				Color(230.0/255.0, 25.0/255.0, 75.0/255.0),
				Color(245.0/255.0, 130.0/255.0, 48.0/255.0),
				Color(255.0/255.0, 225.0/255.0, 25.0/255.0),
				Color(210.0/255.0, 245.0/255.0, 60.0/255.0),
				Color(60.0/255.0, 180.0/255.0, 75.0/255.0),
				Color(70.0/255.0, 240.0/255.0, 240.0/255.0),
				Color(0.0/255.0, 130.0/255.0, 200.0/255.0),
				Color(145.0/255.0, 30.0/255.0, 180.0/255.0),
				Color(240.0/255.0, 50.0/255.0, 230.0/255.0),
				Color(0.0/255.0, 128.0/255.0, 128.0/255.0),
				Color(170.0/255.0, 110.0/255.0, 40.0/255.0),
				Color(128.0/255.0, 128.0/255.0, 0.0/255.0),

				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
				Color(0.5f,0.5f,0.5f),
		};
		static const Vector3 burgersVectors[] = {
				{ FloatType(1.0/6.0), FloatType(-2.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-2.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), FloatType(2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), FloatType(-2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), FloatType(2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), FloatType(-2.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(2.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(2.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(-1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(-1.0/6.0), FloatType(1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(2.0/6.0), FloatType(1.0/6.0), FloatType(1.0/6.0) },

				{ 0, FloatType(1.0/6.0), FloatType(1.0/6.0) },
				{ 0, FloatType(1.0/6.0), FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), 0, FloatType(1.0/6.0) },
				{ FloatType(1.0/6.0), 0, FloatType(-1.0/6.0) },
				{ FloatType(1.0/6.0), FloatType(1.0/6.0), 0 },
				{ FloatType(1.0/6.0), FloatType(-1.0/6.0), 0 },
		};
		OVITO_STATIC_ASSERT(sizeof(burgersVectors)/sizeof(burgersVectors[0]) == sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(burgersVectors)/sizeof(burgersVectors[0]); i++) {
			if(b.equals(burgersVectors[i], FloatType(1e-6)) || b.equals(-burgersVectors[i], FloatType(1e-6)))
				return predefinedLineColors[i];
		}
	}
	return Color(0.9f, 0.9f, 0.9f);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
