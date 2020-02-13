////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2016 Alexander Stukowski
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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/core/dataset/DataSetContainer.h>

namespace Ovito {

/**
 * \brief Manages the DataSet being edited.
 */
class OVITO_GUI_EXPORT GuiDataSetContainer : public DataSetContainer
{
	Q_OBJECT
	OVITO_CLASS(GuiDataSetContainer)

public:

	/// \brief Constructor.
	GuiDataSetContainer(MainWindow* mainWindow = nullptr);

	/// \brief Returns the window this dataset container is linked to (may be NULL).
	MainWindow* mainWindow() const { return _mainWindow; }

	/// \brief Imports a given file into the current dataset.
	/// \param url The location of the file to import.
	/// \param importerType The FileImporter type to use. If NULL, the file format will be automatically detected.
	/// \return true if the file was successfully imported; false if operation has been canceled by the user.
	/// \throw Exception on error.
	bool importFile(const QUrl& url, const FileImporterClass* importerType = nullptr);

	/// \brief Creates an empty dataset and makes it the current dataset.
	/// \return \c true if the operation was completed; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool fileNew();

	/// \brief Loads the given file and makes it the current dataset.
	/// \return \c true if the file has been successfully loaded; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool fileLoad(const QString& filename);

	/// \brief Save the current dataset.
	/// \return \c true, if the dataset has been saved; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	///
	/// If the current dataset has not been assigned a file path, then this method
	/// displays a file selector dialog by calling fileSaveAs() to let the user select a file path.
	bool fileSave();

	/// \brief Lets the user select a new destination filename for the current dataset. Then saves the dataset by calling fileSave().
	/// \param filename If \a filename is an empty string that this method asks the user for a filename. Otherwise
	///                 the provided filename is used.
	/// \return \c true, if the dataset has been saved; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool fileSaveAs(const QString& filename = QString());

	/// \brief Asks the user if changes made to the dataset should be saved.
	/// \return \c false if the operation has been canceled by the user; \c true on success.
	/// \throw Exception on error.
	///
	/// If the current dataset has been changed, this method asks the user if changes should be saved.
	/// If yes, then the dataset is saved by calling fileSave().
	bool askForSaveChanges();

Q_SIGNALS:

	/// Is emitted whenever the scene of the current dataset has been changed and is being made ready for rendering.
	void scenePreparationBegin();

	/// Is emitted whenever the scene of the current dataset became ready for rendering.
	void scenePreparationEnd();

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// Is called when scene of the current dataset is ready to be displayed.
	void sceneBecameReady();

	/// The window this dataset container is linked to (may be NULL).
	MainWindow* _mainWindow;

	/// Indicates whether we are already waiting for the scene to become ready.
	bool _sceneReadyScheduled = false;

	/// The task that makes the scene ready for interactive rendering in the viewports.
	SharedFuture<> _sceneReadyFuture;
};

}	// End of namespace
