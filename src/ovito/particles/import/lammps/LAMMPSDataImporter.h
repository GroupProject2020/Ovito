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

#pragma once


#include <ovito/particles/Particles.h>
#include <ovito/particles/import/ParticleImporter.h>
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito { namespace Particles {

/**
 * \brief File parser for LAMMPS data files.
 */
class OVITO_PARTICLES_EXPORT LAMMPSDataImporter : public ParticleImporter
{
	/// Defines a metaclass specialization for this importer type.
	class OOMetaClass : public ParticleImporter::OOMetaClass
	{
	public:
		/// Inherit standard constructor from base meta class.
		using ParticleImporter::OOMetaClass ::OOMetaClass;

		/// Returns the file filter that specifies the files that can be imported by this service.
		virtual QString fileFilter() const override { return QStringLiteral("*"); }

		/// Returns the filter description that is displayed in the drop-down box of the file dialog.
		virtual QString fileFilterDescription() const override { return tr("LAMMPS Data Files"); }

		/// Checks if the given file has format that can be read by this importer.
		virtual bool checkFileFormat(const FileHandle& file) const override;
	};

	OVITO_CLASS_META(LAMMPSDataImporter, OOMetaClass)
	Q_OBJECT

public:

	/// \brief The LAMMPS atom_style used by the data file.
	enum LAMMPSAtomStyle {
		AtomStyle_Unknown,	//< Special value indicating that the atom_style could not be automatically detected and needs to be specified by the user.
		AtomStyle_Angle,
		AtomStyle_Atomic,
		AtomStyle_Body,
		AtomStyle_Bond,
		AtomStyle_Charge,
		AtomStyle_Dipole,
		AtomStyle_DPD,
		AtomStyle_EDPD,
		AtomStyle_MDPD,
		AtomStyle_Electron,
		AtomStyle_Ellipsoid,
		AtomStyle_Full,
		AtomStyle_Line,
		AtomStyle_Meso,
		AtomStyle_Molecular,
		AtomStyle_Peri,
		AtomStyle_SMD,
		AtomStyle_Sphere,
		AtomStyle_Template,
		AtomStyle_Tri,
		AtomStyle_Wavepacket,
		AtomStyle_Hybrid
	};
	Q_ENUMS(LAMMPSAtomStyle);

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LAMMPSDataImporter(DataSet* dataset) : ParticleImporter(dataset), _atomStyle(AtomStyle_Unknown) {}

	/// Returns the title of this object.
	virtual QString objectTitle() const override { return tr("LAMMPS Data"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FileSourceImporter::FrameLoader> createFrameLoader(const Frame& frame, const FileHandle& file) override {
		activateCLocale();
		return std::make_shared<FrameLoader>(frame, file, sortParticles(), atomStyle());
	}

	/// Inspects the header of the given file and returns the detected LAMMPS atom style.
	Future<LAMMPSAtomStyle> inspectFileHeader(const Frame& frame);

private:

	class LAMMPSFrameData : public ParticleFrameData
	{
	public:

		/// Inherit constructor from base class.
		using ParticleFrameData::ParticleFrameData;

		/// Returns the LAMMPS atom style used in the data file.
		LAMMPSAtomStyle detectedAtomStyle() const { return _detectedAtomStyle; }

		/// Returns the LAMMPS atom sub-styles used in the data file if the main style is "hybrid".
		const std::vector<LAMMPSAtomStyle>& detectedAtomSubStyles() const { return _detectedAtomSubStyles; }

		/// Sets the LAMMPS atom style used in the data file.
		void setDetectedAtomStyle(LAMMPSAtomStyle style, std::vector<LAMMPSAtomStyle> subStyles) {
			_detectedAtomStyle = style;
			_detectedAtomSubStyles = std::move(subStyles);
		}

	private:

		/// The LAMMPS atom style used in the data file.
		LAMMPSAtomStyle _detectedAtomStyle;

		/// The LAMMPS atom sub-styles if the atom style is "hybrid".
		std::vector<LAMMPSAtomStyle> _detectedAtomSubStyles;
	};

	/// The format-specific task object that is responsible for reading an input file in the background.
	class FrameLoader : public FileSourceImporter::FrameLoader
	{
	public:

		/// Constructor.
		FrameLoader(const FileSourceImporter::Frame& frame, const FileHandle& file,
				bool sortParticles,
				LAMMPSAtomStyle atomStyle = AtomStyle_Unknown,
				bool detectAtomStyle = false)
			: FileSourceImporter::FrameLoader(frame, file),
				_atomStyle(atomStyle),
				_detectAtomStyle(detectAtomStyle),
				_sortParticles(sortParticles) {}

		/// Detects or verifies the LAMMPS atom style used by the data file.
		bool detectAtomStyle(const char* firstLine, const QByteArray& keywordLine);

	protected:

		/// Reads the frame data from the external file.
		virtual FrameDataPtr loadFile() override;

		/// Sets up the mapping of data file columns to internal particle properties based on the selected LAMMPS atom style.
		static InputColumnMapping createColumnMapping(LAMMPSAtomStyle atomStyle, bool includeImageFlags);

		/// Parses a hint string for the LAMMPS atom style.
		static LAMMPSAtomStyle parseAtomStyleHint(const QString& atomStyleHint);

		/// The LAMMPS atom style to assume.
		LAMMPSAtomStyle _atomStyle;

		/// The LAMMPS atom sub-styles if the atom style is "hybrid".
		std::vector<LAMMPSAtomStyle> _atomSubStyles;

		bool _detectAtomStyle;
		bool _sortParticles;
	};

	/// The LAMMPS atom style used by the data format.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(LAMMPSAtomStyle, atomStyle, setAtomStyle);
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::LAMMPSDataImporter::LAMMPSAtomStyle);
Q_DECLARE_TYPEINFO(Ovito::Particles::LAMMPSDataImporter::LAMMPSAtomStyle, Q_PRIMITIVE_TYPE);
