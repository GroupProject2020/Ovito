///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <core/Core.h>

#include <QAbstractListModel>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Manages the application-wide list of modifier templates.
 */
class OVITO_CORE_EXPORT ModifierTemplates : public QAbstractListModel
{
	Q_OBJECT

public:

	/// \brief Constructor.
	ModifierTemplates(QObject* parent = nullptr);

	/// \brief Returns the names of the stored modifier templates.
	const QStringList& templateList() const { return _templateNames; }

	/// \brief Returns the number of rows in this list model.
	virtual int rowCount(const QModelIndex& parent) const override { return _templateNames.size(); }

	/// \brief Returns the data stored in this list model under the given role.
	virtual QVariant data(const QModelIndex& index, int role) const override {
		if(role == Qt::DisplayRole)
			return _templateNames[index.row()];
		else
			return {};
	}

	/// \brief Creates a new modifier template on the basis of the given modifier(s).
	/// \param templateName The name of the new template. If a temnplate with the same name exists, it is overwritten.
	/// \param modifiers The list of one or more modifiers from which the template should be created. 
	/// \return The index of the created template.
	int createTemplate(const QString& templateName, const QVector<OORef<Modifier>>& modifiers);

	/// \brief Creates a new modifier template given a serialized version of the modifier.
	/// \param templateName The name of the new template. If a temnplate with the same name exists, it is overwritten.
	/// \param data The serialized modifier data, which was originally obtained by a call to templateData(). 
	/// \return The index of the created template.
	int createTemplate(const QString& templateName, QByteArray data);

	/// \brief Deletes the given modifier template from the store.
	void removeTemplate(const QString& templateName);

	/// \brief Renames an existing modifier template.
	void renameTemplate(const QString& oldTemplateName, const QString& newTemplateName);

	/// \brief Instantiates the modifiers that are stored under the given template name.
	QVector<OORef<Modifier>> instantiateTemplate(const QString& templateName, DataSet* dataset);

	/// \brief Returns the serialized modifier data for the given template.
	QByteArray templateData(const QString& templateName);

	/// \brief Writes in-memory template list to the given settings store.
	void commit() {
		QSettings settings;
		commit(settings);
	}

	/// \brief Writes in-memory template list to the given settings store.
	void commit(QSettings& settings);

	/// \brief Loads a template list from the given settings store.
	int load(QSettings& settings);

private:

	/// Holds the names of the templates.
	QStringList _templateNames;

	/// Holds the serialized modifier data for the templates.
	std::map<QString, QByteArray> _templateData;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
