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


#include <ovito/gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

class RolloutContainer;		// defined below

/**
 * This data structure specifies how and where a new rollout is inserted into a RolloutContainer.
 */
class RolloutInsertionParameters
{
public:

	RolloutInsertionParameters after(QWidget* afterThisRollout) const {
		OVITO_ASSERT(afterThisRollout);
		RolloutInsertionParameters p;
		p._collapsed = this->_collapsed;
		p._useAvailableSpace = this->_useAvailableSpace;
		p._intoThisContainer = this->_intoThisContainer;
		p._afterThisRollout = afterThisRollout;
		p._title = this->_title;
		return p;
	}

	RolloutInsertionParameters before(QWidget* beforeThisRollout) const {
		OVITO_ASSERT(beforeThisRollout);
		RolloutInsertionParameters p;
		p._collapsed = this->_collapsed;
		p._useAvailableSpace = this->_useAvailableSpace;
		p._intoThisContainer = this->_intoThisContainer;
		p._beforeThisRollout = beforeThisRollout;
		p._title = this->_title;
		return p;
	}

	RolloutInsertionParameters collapse() const {
		RolloutInsertionParameters p(*this);
		p._collapsed = true;
		return p;
	}

	RolloutInsertionParameters useAvailableSpace() const {
		RolloutInsertionParameters p(*this);
		p._useAvailableSpace = true;
		return p;
	}

	RolloutInsertionParameters animate() const {
		RolloutInsertionParameters p(*this);
		p._animateFirstOpening = true;
		return p;
	}

	RolloutInsertionParameters insertInto(QWidget* intoThisContainer) const {
		OVITO_ASSERT(intoThisContainer);
		RolloutInsertionParameters p;
		p._intoThisContainer = intoThisContainer;
		return p;
	}

	RolloutInsertionParameters setTitle(const QString& title) const {
		RolloutInsertionParameters p(*this);
		p._title = title;
		return p;
	}

	/// Returns the container set by insertInto() into which the properties editor should inserted.
	QWidget* container() const {
		return _intoThisContainer;
	}

	const QString& title() const { return _title; }

private:

	bool _collapsed = false;
	bool _animateFirstOpening = false;
	bool _useAvailableSpace = false;
	QPointer<QWidget> _afterThisRollout;
	QPointer<QWidget> _beforeThisRollout;
	QPointer<QWidget> _intoThisContainer;
	QString _title;

	friend class Rollout;
	friend class RolloutContainer;
};

/******************************************************************************
* A rollout widget in a RolloutContainer.
* This is part of the implementation of the RolloutContainer and
* should not be used outside of a RolloutContainer.
******************************************************************************/
class OVITO_GUI_EXPORT Rollout : public QWidget
{
	Q_OBJECT

public:

	/// Constructor.
	Rollout(QWidget* parent, QWidget* content, const QString& title, const RolloutInsertionParameters& params, const char* helpPage = nullptr);

	/// Returns true if this rollout is currently in the collapsed state.
	bool isCollapsed() const { return visiblePercentage() != 100; }

	/// Returns the child widget that is contained in the rollout.
	QWidget* content() const { return _content; }

	/// Returns how much of rollout contents is visible.
	int visiblePercentage() const { return _visiblePercentage; }

	/// Sets how much of rollout contents is visible.
	void setVisiblePercentage(int p) {
		_visiblePercentage = p;
		updateGeometry();
	}

	/// Computes the recommended size for the widget.
	virtual QSize sizeHint() const override;

	/// Returns true if the widget's preferred height depends on its width; otherwise returns false.
	virtual bool hasHeightForWidth() const override { return _noticeWidget != nullptr; }

	/// Returns the preferred height for this widget, given a width.
	virtual int heightForWidth(int w) const override;

	Q_PROPERTY(int visiblePercentage READ visiblePercentage WRITE setVisiblePercentage);

public Q_SLOTS:

	/// Opens the rollout if it is collapsed; or collapses it if it is open.
	void toggleCollapsed() { setCollapsed(!isCollapsed()); }

	/// Collapses or opens the rollout.
	void setCollapsed(bool collapsed);

	/// Changes the title of the rollout.
	void setTitle(const QString& title) {
		_titleButton->setText(title);
	}
	
	/// Displays a notice text at the top of the rollout window.
	void setNotice(const QString& noticeText);

	/// Is called when the user presses the help button.
	void onHelpButton();

	/// Makes sure that the rollout is visible in the rollout container.
	void ensureVisible();

protected:

	/// Handles the resize events of the rollout widget.
	virtual void resizeEvent(QResizeEvent* event) override;

	/// Paints the border around the contents widget.
	virtual void paintEvent(QPaintEvent* event) override;

private:

	/// The button that allows to collapse the rollout.
	QPushButton* _titleButton;

	/// The button that opens the help page.
	QPushButton* _helpButton;

	/// The widget that is inside the rollout.
	QPointer<QWidget> _content;

	/// The label widget displaying the user notice.
	QLabel* _noticeWidget = nullptr;

	/// Internal property that controls how much of rollout contents is visible.
	int _visiblePercentage;

	/// The object that animates the collapse/opening of the rollout.
	QPropertyAnimation _collapseAnimation;

	/// Indicates that this rollout should automatically expand to use all available space in the container.
	bool _useAvailableSpace;

	/// The help page in the user manual for this rollout.
	const char* _helpPage;
};

/******************************************************************************
* This container manages multiple rollouts.
******************************************************************************/
class OVITO_GUI_EXPORT RolloutContainer : public QScrollArea
{
	Q_OBJECT

public:

	/// Constructs the rollout container.
	RolloutContainer(QWidget* parent = 0);

	/// Adds a new rollout to the container.
	Rollout* addRollout(QWidget* content, const QString& title, const RolloutInsertionParameters& param = RolloutInsertionParameters(), const char* helpPage = nullptr);

	virtual QSize minimumSizeHint() const override {
		return QSize(QFrame::minimumSizeHint().width(), 10);
	}

	/// Returns the Rollout that hosts the given widget.
	Rollout* findRolloutFromWidget(QWidget* content) const;

protected:

	/// Handles the resize events of the rollout container widget.
	virtual void resizeEvent(QResizeEvent* event) override {
		QScrollArea::resizeEvent(event);
		updateRollouts();
	}

public Q_SLOTS:

	/// Updates the size of all rollouts.
	void updateRollouts();

	/// Updates the size of all rollouts soon.
	void updateRolloutsLater() {
		QTimer::singleShot(0, this, &RolloutContainer::updateRollouts);
	}

};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
