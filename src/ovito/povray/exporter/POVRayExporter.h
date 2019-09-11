///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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


#include <ovito/core/Core.h>
#include <ovito/core/dataset/io/FileExporter.h>
#include "../renderer/POVRayRenderer.h"

namespace Ovito { namespace POVRay {

/**
 * \brief Export services that write the scene to a POV-Ray file.
 */
class OVITO_POVRAY_EXPORT POVRayExporter : public FileExporter
{
	/// Defines a metaclass specialization for this exporter type.
	class OOMetaClass : public FileExporter::OOMetaClass
	{
	public:

		/// Inherit standard constructor from base meta class.
		using FileExporter::OOMetaClass::OOMetaClass;

		/// Returns the file filter that specifies the files that can be exported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*.pov"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("POV-Ray scene"); }
	};

	Q_OBJECT
	OVITO_CLASS_META(POVRayExporter, OOMetaClass)

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE POVRayExporter(DataSet* dataset);

	/// \brief Determines whether the given scene node is suitable for exporting with this exporter service.
	virtual bool isSuitableNode(SceneNode* node) const override {
		return node->isRootNode();
	}

	/// \brief This is called once for every output file to be written and before exportData() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames, AsyncOperation& operation) override;

	/// \brief This is called once for every output file written after exportData() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

protected:

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, AsyncOperation&& operation) override;

private:

	/// The output file stream.
	QFile _outputFile;

	/// The internal renderer, which is responsible for streaming the scene to a POVRay scene file.
	OORef<POVRayRenderer> _renderer;
};

}	// End of namespace
}	// End of namespace
