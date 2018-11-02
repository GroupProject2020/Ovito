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

#include <plugins/particles/Particles.h>
#include <plugins/particles/import/ParticleFrameData.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "GALAMOSTImporter.h"

#include <QXmlDefaultHandler> 
#include <QXmlInputSource> 
#include <QXmlSimpleReader> 

#include <boost/algorithm/string.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_OVITO_CLASS(GALAMOSTImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GALAMOSTImporter::OOMetaClass::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) const
{
	// Open input file and test whether it's an XML file.
	{
		CompressedTextReader stream(input, sourceLocation.path());
		const char* line = stream.readLineTrimLeft(1024);
		if(!boost::algorithm::istarts_with(line, "<?xml "))
			return false;
		input.seek(0);
	}

	// Now use a full XML parser to check the schema of the XML file. First XML element must be <galamost_xml>.

	// A minimal XML content handler class that just checks the name of the first XML element:
	class ContentHandler : public QXmlDefaultHandler
	{
	public:
		virtual bool startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts) override {
			if(localName == "galamost_xml")
				isGalamostFile = true;
			return false; // Always stop after the first XML element. We are not interested in any further data. 
		}
		bool isGalamostFile = false;
	};

	// Set up XML data source and reader.
	QXmlInputSource source(&input);
	QXmlSimpleReader reader;
	ContentHandler contentHandler;
	reader.setContentHandler(&contentHandler);
	reader.parse(&source, false);

	return contentHandler.isGalamostFile;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr GALAMOSTImporter::FrameLoader::loadFile(QFile& file)
{
	setProgressText(tr("Reading GALAMOST file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Create the container for the particle data to be loaded.
	_frameData = std::make_shared<ParticleFrameData>();

	// Set up XML data source and reader, then parse the file.
	QXmlInputSource source(&file);
	QXmlSimpleReader reader;
	reader.setContentHandler(this);
	reader.setErrorHandler(this);
	reader.parse(&source, false);

	// Make sure bonds that cross a periodic cell boundary are correctly wrapped around.
	_frameData->generateBondPeriodicImageProperty();

	// Report number of particles and bonds to the user.
	QString statusString = tr("Number of particles: %1").arg(_natoms);
	if(PropertyPtr topologyProperty = _frameData->findStandardBondProperty(BondsObject::TopologyProperty))
		statusString += tr("\nNumber of bonds: %1").arg(topologyProperty->size());
	_frameData->setStatus(statusString);

	return std::move(_frameData);
}

/******************************************************************************
* Is called by the XML parser whenever a new XML element is read.
******************************************************************************/
bool GALAMOSTImporter::FrameLoader::fatalError(const QXmlParseException& exception)
{
	if(!isCanceled()) {
		setException(std::make_exception_ptr(
			Exception(tr("GALAMOST file parsing error on line %1, column %2: %3")
			.arg(exception.lineNumber()).arg(exception.columnNumber()).arg(exception.message()))));
	}
	return false;
}

/******************************************************************************
* Is called by the XML parser whenever a new XML element is read.
******************************************************************************/
bool GALAMOSTImporter::FrameLoader::startElement(const QString& namespaceURI, const QString& localName, const QString& qName, const QXmlAttributes& atts)
{
	// This parser only reads the first <configuration> element in a GALAMOST file. 
	// Additional <configuration> elements will be skipped.
	if(_numConfigurationsRead == 0) {
		if(localName == "configuration") {

			// Parse simulation timestep.
			QString timeStepStr = atts.value(QStringLiteral("time_step"));
			if(!timeStepStr.isEmpty()) {
				bool ok;
				_frameData->attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(timeStepStr.toLongLong(&ok)));
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'time_step' attribute value in <%1> element: %2").arg(qName).arg(timeStepStr));
			}

			// Parse dimensionality.
			QString dimensionsStr = atts.value(QStringLiteral("dimensions"));
			if(!dimensionsStr.isEmpty()) {
				int dimensions = dimensionsStr.toInt();
				if(dimensions == 2)
					_frameData->simulationCell().set2D(true);
				else if(dimensions != 3)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'dimensions' attribute value in <%1> element: %2").arg(qName).arg(dimensionsStr));
			}

			// Parse number of atoms.
			QString natomsStr = atts.value(QStringLiteral("natoms"));
			if(!natomsStr.isEmpty()) {
				bool ok;
				_natoms = natomsStr.toULongLong(&ok);
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'natoms' attribute value in <%1> element: %2").arg(qName).arg(natomsStr));
			}
			else {
				throw Exception(tr("GALAMOST file parsing error. Expected 'natoms' attribute in <%1> element.").arg(qName));
			}
		}
		else if(localName == "box") {
			// Parse box dimensions.
			AffineTransformation cellMatrix = _frameData->simulationCell().matrix();
			QString lxStr = atts.value(QStringLiteral("lx"));
			if(!lxStr.isEmpty()) {
				bool ok;
				cellMatrix(0,0) = (FloatType)lxStr.toDouble(&ok);
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'lx' attribute value in <%1> element: %2").arg(qName).arg(lxStr));
			}
			QString lyStr = atts.value(QStringLiteral("ly"));
			if(!lyStr.isEmpty()) {
				bool ok;
				cellMatrix(1,1) = (FloatType)lyStr.toDouble(&ok);
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'ly' attribute value in <%1> element: %2").arg(qName).arg(lyStr));
			}
			QString lzStr = atts.value(QStringLiteral("lz"));
			if(!lzStr.isEmpty()) {
				bool ok;
				cellMatrix(2,2) = (FloatType)lzStr.toDouble(&ok);
				if(!ok)
					throw Exception(tr("GALAMOST file parsing error. Invalid 'lz' attribute value in <%1> element: %2").arg(qName).arg(lzStr));
			}
			cellMatrix.translation() = cellMatrix * Vector3(-0.5, -0.5, -0.5);
			_frameData->simulationCell().setMatrix(cellMatrix);
		}
		else if(localName == "position") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::PositionProperty, false);
		}
		else if(localName == "velocity") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::VelocityProperty, false);
		}
		else if(localName == "image") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::PeriodicImageProperty, false);
		}
		else if(localName == "mass") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::MassProperty, false);
		}
		else if(localName == "diameter") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::RadiusProperty, false);
		}
		else if(localName == "charge") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::ChargeProperty, false);
		}
		else if(localName == "quaternion") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::OrientationProperty, false);
		}
		else if(localName == "orientation") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::OrientationProperty, false);
		}
		else if(localName == "type") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::TypeProperty, false);
		}
		else if(localName == "molecule") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::MoleculeProperty, false);
		}
		else if(localName == "body") {
			_currentProperty = std::make_shared<PropertyStorage>(_natoms, PropertyStorage::Int64, 1, 0, QStringLiteral("Body"), false);
		}
		else if(localName == "Aspheres") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::AsphericalShapeProperty, false);
		}
		else if(localName == "rotation") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::AngularVelocityProperty, false);
		}
		else if(localName == "inert") {
			_currentProperty = ParticlesObject::OOClass().createStandardStorage(_natoms, ParticlesObject::AngularMomentumProperty, false);
		}
		else if(localName == "bond") {
			_currentProperty = BondsObject::OOClass().createStandardStorage(0, BondsObject::TopologyProperty, false);
		}
	}

	return !isCanceled();
}

/******************************************************************************
* Is called by the XML parser whenever it has parsed a chunk of character data.
******************************************************************************/
bool GALAMOSTImporter::FrameLoader::characters(const QString& ch)
{
	if(_currentProperty) {
		_characterData += ch;
	}
	return !isCanceled();
}

/******************************************************************************
* Is called by the XML parser whenever it has parsed an end element tag.
******************************************************************************/
bool GALAMOSTImporter::FrameLoader::endElement(const QString& namespaceURI, const QString& localName, const QString& qName)
{
	if(_currentProperty) {
		QTextStream stream(&_characterData, QIODevice::ReadOnly | QIODevice::Text);
		if(localName == "position") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::PositionProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Point3 pos;
				stream >> pos.x() >> pos.y() >> pos.z();
				_currentProperty->setPoint3(i, pos);
			}
		}
		else if(localName == "velocity") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::VelocityProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Vector3 vel;
				stream >> vel.x() >> vel.y() >> vel.z();
				_currentProperty->setVector3(i, vel);
			}
		}
		else if(localName == "image") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::PeriodicImageProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Point3I image;
				stream >> image.x() >> image.y() >> image.z();
				_currentProperty->setPoint3I(i, image);
			}
		}
		else if(localName == "mass") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::MassProperty);
			for(size_t i = 0; i < _natoms; i++) {
				FloatType mass;
				stream >> mass;
				_currentProperty->setFloat(i, mass);
			}
		}
		else if(localName == "diameter") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::RadiusProperty);
			for(size_t i = 0; i < _natoms; i++) {
				FloatType diameter;
				stream >> diameter;
				_currentProperty->setFloat(i, diameter / 2);
			}
		}
		else if(localName == "charge") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::ChargeProperty);
			for(size_t i = 0; i < _natoms; i++) {
				FloatType charge;
				stream >> charge;
				_currentProperty->setFloat(i, charge);
			}
		}
		else if(localName == "quaternion") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::OrientationProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Quaternion q;
				stream >> q.w() >> q.x() >> q.y() >> q.z();
				_currentProperty->setQuaternion(i, q);
			}
		}
		else if(localName == "orientation") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::OrientationProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Vector3 dir;
				stream >> dir.x() >> dir.y() >> dir.z();
				Rotation r(Vector3(0,0,1), dir);
				_currentProperty->setQuaternion(i, Quaternion(r));
			}
		}
		else if(localName == "molecule") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::MoleculeProperty);
			for(size_t i = 0; i < _natoms; i++) {
				qlonglong molecule;
				stream >> molecule;
				_currentProperty->setInt64(i, molecule);
			}
		}
		else if(localName == "body") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::UserProperty);
			for(size_t i = 0; i < _natoms; i++) {
				qlonglong body;
				stream >> body;
				_currentProperty->setInt64(i, body);
			}
		}
		else if(localName == "rotation") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::AngularVelocityProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Vector3 rot_vel;
				stream >> rot_vel.x() >> rot_vel.y() >> rot_vel.z();
				_currentProperty->setVector3(i, rot_vel);
			}
		}
		else if(localName == "inert") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::AngularMomentumProperty);
			for(size_t i = 0; i < _natoms; i++) {
				Vector3 ang_moment;
				stream >> ang_moment.x() >> ang_moment.y() >> ang_moment.z();
				_currentProperty->setVector3(i, ang_moment);
			}
		}		
		else if(localName == "type") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::TypeProperty);
			QString typeName;
			std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>();
			for(size_t i = 0; i < _natoms; i++) {
				stream >> typeName;
				_currentProperty->setInt(i, typeList->addTypeName(typeName));
			}
			typeList->sortTypesByName(_currentProperty);
			_frameData->setPropertyTypesList(_currentProperty, std::move(typeList));
		}
		else if(localName == "Aspheres") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::AsphericalShapeProperty);
			PropertyPtr typeProperty = _frameData->findStandardParticleProperty(ParticlesObject::TypeProperty);
			if(!typeProperty)
				throw Exception(tr("GALAMOST file parsing error. <%1> element must appear after <type> element.").arg(qName));
			ParticleFrameData::TypeList* typeList = _frameData->propertyTypesList(typeProperty);
			OVITO_ASSERT(typeList != nullptr);
			std::vector<Vector3> typesAsphericalShape;
			while(!stream.atEnd()) {
				QString typeName;
				FloatType a,b,c;
				FloatType eps_a, eps_b, eps_c;
				stream >> typeName >> a >> b >> c >> eps_a >> eps_b >> eps_c;
				stream.skipWhiteSpace();
				for(const ParticleFrameData::TypeDefinition& type : typeList->types()) {
					if(type.name == typeName) {
						if(typesAsphericalShape.size() <= type.id) typesAsphericalShape.resize(type.id+1, Vector3::Zero());
						typesAsphericalShape[type.id] = Vector3(a/2,b/2,c/2);
						break;
					}
				}
				for(size_t i = 0; i < _natoms; i++) {
					int typeIndex = typeProperty->getInt(i);
					if(typeIndex < typesAsphericalShape.size())
						_currentProperty->setVector3(i, typesAsphericalShape[typeIndex]);
				}
			}
		}
		else if(localName == "bond") {
			OVITO_ASSERT(_currentProperty->type() == BondsObject::TopologyProperty);
			QString typeName;
			std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>();
			std::vector<qlonglong> topology;
			std::vector<int> bondTypes;
			while(!stream.atEnd()) {
				qlonglong a,b;
				stream >> typeName >> a >> b;
				bondTypes.push_back(typeList->addTypeName(typeName));
				topology.push_back(a);
				topology.push_back(b);
				stream.skipWhiteSpace();
			}
			_currentProperty = BondsObject::OOClass().createStandardStorage(topology.size() / 2, BondsObject::TopologyProperty, false);
			std::copy(topology.begin(), topology.end(), _currentProperty->dataInt64());
			_frameData->addBondProperty(std::move(_currentProperty));
			_currentProperty = BondsObject::OOClass().createStandardStorage(bondTypes.size(), BondsObject::TypeProperty, false);
			std::copy(bondTypes.begin(), bondTypes.end(), _currentProperty->dataInt());
			typeList->sortTypesByName(_currentProperty);
			_frameData->setPropertyTypesList(_currentProperty, std::move(typeList));
			_frameData->addBondProperty(std::move(_currentProperty));
			_currentProperty.reset();
		}
		if(stream.status() == QTextStream::ReadPastEnd)
			throw Exception(tr("GALAMOST file parsing error. Unexpected end of data in <%1> element").arg(qName));
		if(_currentProperty) {
			_frameData->addParticleProperty(std::move(_currentProperty));
			_currentProperty.reset();
		}
		_characterData.clear();
	}
	else {
		if(localName == "configuration")
			_numConfigurationsRead++;
	}
	return !isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
