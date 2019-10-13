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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/core/dataset/io/FileSourceImporter.h>

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
	ParticleImporter(DataSet* dataset) : FileSourceImporter(dataset),
		_isMultiTimestepFile(false), _sortParticles(false) {}

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

	/// Request sorting of the input particle with respect to IDs.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, sortParticles, setSortParticles);
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


