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
#include <ovito/particles/objects/ParticlesObject.h>
#include <ovito/core/dataset/pipeline/PipelineFlowState.h>
#include <ovito/core/dataset/io/FileExporter.h>
#include <ovito/core/utilities/io/CompressedTextWriter.h>

namespace Ovito { namespace Particles {

/**
 * \brief Abstract base class for export services that write particle datasets to an output file.
 */
class OVITO_PARTICLES_EXPORT ParticleExporter : public FileExporter
{
	Q_OBJECT
	OVITO_CLASS(ParticleExporter)

public:

	/// \brief Evaluates the pipeline of an PipelineSceneNode and makes sure that the data to be
	///        exported contains particles and throws an exception if not.
	PipelineFlowState getParticleData(TimePoint time, AsyncOperation& operation) const;

	/// \brief Returns the type(s) of data objects that this exporter service can export.
	virtual std::vector<DataObjectClassPtr> exportableDataObjectClass() const override {
		return { &ParticlesObject::OOClass() };
	}

protected:

	/// \brief Constructs a new instance of this class.
	ParticleExporter(DataSet* dataset);

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation) override;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

	/// Returns the text stream used to write into the current output file.
	CompressedTextWriter& textStream() { return *_outputStream; }

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;

	/// \brief Writes the particle data of one animation frame to the current output file.
	/// \param state The data to be exported.
	/// \param frameNumber The animation frame to be written to the output file.
	/// \param time The animation time to be written to the output file.
	/// \param filePath The path of the output file.
	/// \throws Exception on error.
	/// \return \a false when the operation has been canceled by the user; \a true on success.
	virtual bool exportData(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) = 0;

private:

	/// The output file stream.
	QFile _outputFile;

	/// The stream object used to write into the output file.
	std::unique_ptr<CompressedTextWriter> _outputStream;
};

}	// End of namespace
}	// End of namespace


