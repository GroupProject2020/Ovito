////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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
#include <ovito/particles/import/ParticleFrameData.h>
#include <ovito/particles/objects/BondType.h>
#include <ovito/core/utilities/io/CompressedTextReader.h>
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
bool GALAMOSTImporter::OOMetaClass::checkFileFormat(const FileHandle& file) const
{
	// Open input file and test whether it's an XML file.
	{
		CompressedTextReader stream(file);
		const char* line = stream.readLineTrimLeft(1024);
		if(!boost::algorithm::istarts_with(line, "<?xml "))
			return false;
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
	std::unique_ptr<QIODevice> device = file.createIODevice();
	QXmlInputSource source(device.get());
	QXmlSimpleReader reader;
	ContentHandler contentHandler;
	reader.setContentHandler(&contentHandler);
	reader.parse(&source, false);

	return contentHandler.isGalamostFile;
}

/******************************************************************************
* Parses the given input file.
******************************************************************************/
FileSourceImporter::FrameDataPtr GALAMOSTImporter::FrameLoader::loadFile()
{
	setProgressText(tr("Reading GALAMOST file %1").arg(fileHandle().toString()));

	// Create the container for the particle data to be loaded.
	_frameData = std::make_shared<ParticleFrameData>();

	// Set up XML data source and reader, then parse the file.
	std::unique_ptr<QIODevice> device = fileHandle().createIODevice();
	QXmlInputSource source(device.get());
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
			for(Point3& p : PropertyAccess<Point3>(_currentProperty)) {
				stream >> p.x() >> p.y() >> p.z();
			}
		}
		else if(localName == "velocity") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::VelocityProperty);
			for(Vector3& v : PropertyAccess<Vector3>(_currentProperty)) {
				stream >> v.x() >> v.y() >> v.z();
			}
		}
		else if(localName == "image") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::PeriodicImageProperty);
			for(Point3I& p : PropertyAccess<Point3I>(_currentProperty)) {
				stream >> p.x() >> p.y() >> p.z();
			}
		}
		else if(localName == "mass") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::MassProperty);
			for(FloatType& mass : PropertyAccess<FloatType>(_currentProperty)) {
				stream >> mass;
			}
		}
		else if(localName == "diameter") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::RadiusProperty);
			for(FloatType& radius : PropertyAccess<FloatType>(_currentProperty)) {
				stream >> radius;
				radius /= 2;
			}
		}
		else if(localName == "charge") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::ChargeProperty);
			for(FloatType& charge : PropertyAccess<FloatType>(_currentProperty)) {
				stream >> charge;
			}
		}
		else if(localName == "quaternion") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::OrientationProperty);
			for(Quaternion& q : PropertyAccess<Quaternion>(_currentProperty)) {
				stream >> q.w() >> q.x() >> q.y() >> q.z();
			}
		}
		else if(localName == "orientation") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::OrientationProperty);
			for(Quaternion& q : PropertyAccess<Quaternion>(_currentProperty)) {
				Vector3 dir;
				stream >> dir.x() >> dir.y() >> dir.z();
				if(!dir.isZero()) {
					Rotation r(Vector3(0,0,1), dir);
					q = Quaternion(r);
				}
				else {
					q = Quaternion::Identity();
				}
			}
		}
		else if(localName == "molecule") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::MoleculeProperty);
			for(qlonglong& molecule : PropertyAccess<qlonglong>(_currentProperty)) {
				stream >> molecule;
			}
		}
		else if(localName == "body") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::UserProperty);
			for(qlonglong& body : PropertyAccess<qlonglong>(_currentProperty)) {
				stream >> body;
			}
		}
		else if(localName == "rotation") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::AngularVelocityProperty);
			for(Vector3& rot_vel : PropertyAccess<Vector3>(_currentProperty)) {
				stream >> rot_vel.x() >> rot_vel.y() >> rot_vel.z();
			}
		}
		else if(localName == "inert") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::AngularMomentumProperty);
			for(Vector3& ang_moment : PropertyAccess<Vector3>(_currentProperty)) {
				stream >> ang_moment.x() >> ang_moment.y() >> ang_moment.z();
			}
		}
		else if(localName == "type") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::TypeProperty);
			QString typeName;
			std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>();
			PropertyAccess<int> typeArray(_currentProperty);
			for(int& type : typeArray) {
				stream >> typeName;
				type = typeList->addTypeName(typeName);
			}
			typeList->sortTypesByName(typeArray);
			_frameData->setPropertyTypesList(typeArray, std::move(typeList));
		}
		else if(localName == "Aspheres") {
			OVITO_ASSERT(_currentProperty->type() == ParticlesObject::AsphericalShapeProperty);
			ConstPropertyAccess<int> typeProperty = _frameData->findStandardParticleProperty(ParticlesObject::TypeProperty);
			if(!typeProperty)
				throw Exception(tr("GALAMOST file parsing error. <%1> element must appear after <type> element.").arg(qName));
			ParticleFrameData::TypeList* typeList = _frameData->createPropertyTypesList(typeProperty.storage());
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
				const int* typeIndex = typeProperty.cbegin();
				for(Vector3& shape : PropertyAccess<Vector3>(_currentProperty)) {
					if(*typeIndex < typesAsphericalShape.size())
						shape = typesAsphericalShape[*typeIndex];
					++typeIndex;
				}
			}
		}
		else if(localName == "bond") {
			OVITO_ASSERT(_currentProperty->type() == BondsObject::TopologyProperty);
			QString typeName;
			std::unique_ptr<ParticleFrameData::TypeList> typeList = std::make_unique<ParticleFrameData::TypeList>(BondType::OOClass());
			std::vector<ParticleIndexPair> topology;
			std::vector<int> bondTypes;
			while(!stream.atEnd()) {
				qlonglong a,b;
				stream >> typeName >> a >> b;
				bondTypes.push_back(typeList->addTypeName(typeName));
				topology.push_back({a,b});
				stream.skipWhiteSpace();
			}
			PropertyAccess<ParticleIndexPair> topologyProperty = _frameData->addBondProperty(BondsObject::OOClass().createStandardStorage(topology.size(), BondsObject::TopologyProperty, false));
			boost::copy(topology, topologyProperty.begin());
			PropertyAccess<int> bondTypeProperty = _frameData->addBondProperty(BondsObject::OOClass().createStandardStorage(bondTypes.size(), BondsObject::TypeProperty, false));
			boost::copy(bondTypes, bondTypeProperty.begin());
			typeList->sortTypesByName(bondTypeProperty);
			_frameData->setPropertyTypesList(bondTypeProperty, std::move(typeList));
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
