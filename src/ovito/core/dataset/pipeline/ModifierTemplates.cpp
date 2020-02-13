////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2017 Alexander Stukowski
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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/pipeline/Modifier.h>
#include <ovito/core/dataset/DataSet.h>
#include "ModifierTemplates.h"

namespace Ovito {

static const QString modTemplateStoreGroup = QStringLiteral("core/modifier/templates/");

/******************************************************************************
* Constructor.
******************************************************************************/
ModifierTemplates::ModifierTemplates(QObject* parent) : QAbstractListModel(parent)
{
	QSettings settings;
	settings.beginGroup(modTemplateStoreGroup);
	_templateNames = settings.childKeys();
}

/******************************************************************************
* Creates a new modifier template on the basis of the given modifier(s).
******************************************************************************/
int ModifierTemplates::createTemplate(const QString& templateName, const QVector<OORef<Modifier>>& modifiers)
{
	if(modifiers.empty())
		throw Exception(tr("Expected non-empty modifier list for creating a new modifier template."));

	QByteArray buffer;
	QDataStream dstream(&buffer, QIODevice::WriteOnly);
	ObjectSaveStream stream(dstream);

	// Serialize modifiers.
	for(Modifier* modifier : modifiers) {
		stream.beginChunk(0x01);
		stream.saveObject(modifier);
		stream.endChunk();
	}

	// Append EOF marker.
	stream.beginChunk(0x00);
	stream.endChunk();
	stream.close();

	return createTemplate(templateName, std::move(buffer));
}

/******************************************************************************
* Creates a new modifier template given a serialized version of the modifier.
******************************************************************************/
int ModifierTemplates::createTemplate(const QString& templateName, QByteArray data)
{
	if(templateName.trimmed().isEmpty())
		throw Exception(tr("Invalid modifier template name."));

	_templateData[templateName] = std::move(data);
	int idx = _templateNames.indexOf(templateName);
	if(idx >= 0) {
		Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
		return idx;
	}
	else {
		beginInsertRows(QModelIndex(), _templateNames.size(), _templateNames.size());
		_templateNames.push_back(templateName);
		endInsertRows();
		return _templateNames.size() - 1;
	}
}

/******************************************************************************
* Deletes the given modifier template from the store.
******************************************************************************/
void ModifierTemplates::removeTemplate(const QString& templateName)
{
	int idx = _templateNames.indexOf(templateName);
	if(idx < 0)
		throw Exception(tr("Modifier template with the name '%1' does not exist.").arg(templateName));

	_templateData.erase(templateName);
	beginRemoveRows(QModelIndex(), idx, idx);
	_templateNames.removeAt(idx);
	endRemoveRows();
}

/******************************************************************************
* Renames an existing modifier template.
******************************************************************************/
void ModifierTemplates::renameTemplate(const QString& oldTemplateName, const QString& newTemplateName)
{
	int idx = _templateNames.indexOf(oldTemplateName);
	if(idx < 0)
		throw Exception(tr("Modifier template with the name '%1' does not exist.").arg(oldTemplateName));
	if(_templateNames.contains(newTemplateName))
		throw Exception(tr("Modifier template with the name '%1' does already exist.").arg(newTemplateName));
	if(newTemplateName.trimmed().isEmpty())
		throw Exception(tr("Invalid new modifier template name."));

	_templateData[newTemplateName] = templateData(oldTemplateName);
	_templateData.erase(oldTemplateName);
	_templateNames[idx] = newTemplateName;
	Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
}

/******************************************************************************
* Returns the serialized modifier data for the given template.
******************************************************************************/
QByteArray ModifierTemplates::templateData(const QString& templateName)
{
	int idx = _templateNames.indexOf(templateName);
	if(idx < 0)
		throw Exception(tr("Modifier template with the name '%1' does not exist.").arg(templateName));
	auto iter = _templateData.find(templateName);
	if(iter != _templateData.end())
		return iter->second;
	QSettings settings;
	settings.beginGroup(modTemplateStoreGroup);
	QByteArray buffer = settings.value(templateName).toByteArray();
	if(buffer.isEmpty())
		throw Exception(tr("Modifier template with the name '%1' does not exist in the settings store.").arg(templateName));
	_templateData.insert(std::make_pair(templateName, buffer));
	return buffer;
}

/******************************************************************************
* Instantiates the modifiers that are stored under the given temnplate name.
******************************************************************************/
QVector<OORef<Modifier>> ModifierTemplates::instantiateTemplate(const QString& templateName, DataSet* dataset)
{
	OVITO_ASSERT(dataset != nullptr);

	QVector<OORef<Modifier>> modifierSet;
	try {
		UndoSuspender noUndo(dataset->undoStack());
		QSettings settings;
		settings.beginGroup(modTemplateStoreGroup);
		QByteArray buffer = settings.value(templateName).toByteArray();
		if(buffer.isEmpty())
			throw Exception(tr("Modifier template with the name '%1' does not exist.").arg(templateName));
		QDataStream dstream(buffer);
		ObjectLoadStream stream(dstream);
		stream.setDataset(dataset);
		for(int chunkId = stream.expectChunkRange(0,1); chunkId == 1; chunkId = stream.expectChunkRange(0,1)) {
			modifierSet.push_back(stream.loadObject<Modifier>());
			stream.closeChunk();
		}
		stream.closeChunk();
		stream.close();
	}
	catch(Exception& ex) {
		if(ex.context() == nullptr) ex.setContext(dataset);
		ex.prependGeneralMessage(tr("Failed to load stored modifier template."));
		throw;
	}
	return modifierSet;
}

/******************************************************************************
* Writes in-memory template list to the given settings store.
******************************************************************************/
void ModifierTemplates::commit(QSettings& settings)
{
	for(const QString& templateName : _templateNames)
		templateData(templateName);

	settings.beginGroup(modTemplateStoreGroup);
	settings.remove(QString());
	for(const auto& item : _templateData) {
		settings.setValue(item.first, item.second);
	}
	settings.endGroup();
}

/******************************************************************************
* Loads a template list from the given settings store.
******************************************************************************/
int ModifierTemplates::load(QSettings& settings)
{
	settings.beginGroup(modTemplateStoreGroup);
	int count = 0;
	for(const QString& templateName : settings.childKeys()) {
		QByteArray buffer = settings.value(templateName).toByteArray();
		if(buffer.isEmpty())
			throw Exception(tr("The stored modifier template with the name '%1' is invalid.").arg(templateName));
		createTemplate(templateName, std::move(buffer));
		count++;
	}
	settings.endGroup();
	return count;
}

}	// End of namespace
