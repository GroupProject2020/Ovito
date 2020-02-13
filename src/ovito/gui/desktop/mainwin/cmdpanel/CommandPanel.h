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


#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>

namespace Ovito {

/**
 * The command panel in the main window.
 */
class OVITO_GUI_EXPORT CommandPanel : public QWidget
{
	Q_OBJECT

public:

	/// \brief Creates the command panel.
	CommandPanel(MainWindow* mainWindow, QWidget* parent);

	/// \brief Activate one of the command pages.
	/// \param newPage The identifier of the page to activate.
	void setCurrentPage(MainWindow::CommandPanelPage newPage) {
		OVITO_ASSERT(newPage < _tabWidget->count());
		_tabWidget->setCurrentIndex((int)newPage);
	}

	/// \brief Returns the active command page.
	/// \return The identifier of the page that is currently active.
	MainWindow::CommandPanelPage currentPage() const { return (MainWindow::CommandPanelPage)_tabWidget->currentIndex(); }

	/// \brief Returns the modification page contained in the command panel.
	ModifyCommandPage* modifyPage() const { return _modifyPage; }

	/// \brief Returns the rendering page contained in the command panel.
	RenderCommandPage* renderPage() const { return _renderPage; }

	/// \brief Returns the viewport overlay page contained in the command panel.
	OverlayCommandPage* overlayPage() const { return _overlayPage; }

	/// \brief Returns the default size for the command panel.
	virtual QSize sizeHint() const { return QSize(336, 300); }

private:

	QTabWidget* _tabWidget;
	ModifyCommandPage* _modifyPage;
	RenderCommandPage* _renderPage;
	OverlayCommandPage* _overlayPage;
};

}	// End of namespace


