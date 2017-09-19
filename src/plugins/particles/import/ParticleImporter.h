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

#pragma once


#include <plugins/particles/Particles.h>
#include <core/dataset/io/FileSourceImporter.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/**
 * \brief Base class for file parsers that read particle-position data.
 */
class OVITO_PARTICLES_EXPORT ParticleImporter : public FileSourceImporter
{
	Q_OBJECT
	OVITO_CLASS(ParticleImporter)
	
public:

	/// \brief Constructs a new instance of this class.
	ParticleImporter(DataSet* dataset) : FileSourceImporter(dataset), _isMultiTimestepFile(false) {}

	/// This method indicates whether a wildcard pattern should be automatically generated
	/// when the user picks a new input filename.
	virtual bool autoGenerateWildcardPattern() override { return !isMultiTimestepFile(); }

protected:

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// \brief Determines whether the input file should be scanned to discover all contained frames.
	virtual bool shouldScanFileForFrames(const QUrl& sourceUrl) override {
		return isMultiTimestepFile();
	}

private:

	/// Indicates that the input file contains multiple timesteps.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isMultiTimestepFile, setMultiTimestepFile);
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


