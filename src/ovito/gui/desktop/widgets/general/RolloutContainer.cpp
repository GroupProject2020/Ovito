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

#include <ovito/gui/desktop/GUI.h>
#include <ovito/gui/desktop/mainwin/MainWindow.h>
#include "RolloutContainer.h"
#include "RolloutContainerLayout.h"

namespace Ovito {

/******************************************************************************
* Constructs the container.
******************************************************************************/
RolloutContainer::RolloutContainer(QWidget* parent) : QScrollArea(parent)
{
	setFrameStyle(QFrame::Panel | QFrame::Sunken);
	setWidgetResizable(true);
	QWidget* widget = new QWidget();
	RolloutContainerLayout* layout = new RolloutContainerLayout(widget);
	layout->setContentsMargins(QMargins());
	layout->setSpacing(2);
	widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	setWidget(widget);
}

/******************************************************************************
* Inserts a new rollout into the container.
******************************************************************************/
Rollout* RolloutContainer::addRollout(QWidget* content, const QString& title, const RolloutInsertionParameters& params, const char* helpPage)
{
	OVITO_CHECK_POINTER(content);
	Rollout* rollout = new Rollout(widget(), content, title, params, helpPage);
	RolloutContainerLayout* layout = static_cast<RolloutContainerLayout*>(widget()->layout());
	if(params._afterThisRollout) {
		Rollout* otherRollout = qobject_cast<Rollout*>(params._afterThisRollout->parent());
		for(int i = 0; i < layout->count(); i++) {
			if(layout->itemAt(i)->widget() == otherRollout) {
				layout->insertWidget(i + 1, rollout);
				break;
			}
		}
	}
	else if(params._beforeThisRollout) {
		Rollout* otherRollout = qobject_cast<Rollout*>(params._beforeThisRollout->parent());
		for(int i = 0; i < layout->count(); i++) {
			if(layout->itemAt(i)->widget() == otherRollout) {
				layout->insertWidget(i, rollout);
				break;
			}
		}
	}
	else layout->addWidget(rollout);
	return rollout;
}

/******************************************************************************
* Updates the size of all rollouts.
******************************************************************************/
void RolloutContainer::updateRollouts()
{
	for(QObject* child : widget()->children()) {
		if(child->isWidgetType()) {
			static_cast<QWidget*>(child)->updateGeometry();
		}
	}
}

/******************************************************************************
* Returns the Rollout that hosts the given widget.
******************************************************************************/
Rollout* RolloutContainer::findRolloutFromWidget(QWidget* content) const
{
	for(Rollout* rollout : widget()->findChildren<Rollout*>(QString(), Qt::FindDirectChildrenOnly)) {
		if(rollout->content() == content)
			return rollout;
	}
	return nullptr;
}

/******************************************************************************
* Constructs a rollout widget.
******************************************************************************/
Rollout::Rollout(QWidget* parent, QWidget* content, const QString& title, const RolloutInsertionParameters& params, const char* helpPage) :
	QWidget(parent), _content(content), _collapseAnimation(this, "visiblePercentage"), _useAvailableSpace(params._useAvailableSpace), _helpPage(helpPage)
{
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	_collapseAnimation.setDuration(350);
	_collapseAnimation.setEasingCurve(QEasingCurve::InOutCubic);

	// Set initial open/collapsed state.
	if(!params._animateFirstOpening && !params._collapsed)
		_visiblePercentage = 100;
	else
		_visiblePercentage = 0;

	// Insert contents.
	_content->setParent(this);
	_content->setVisible(true);
	connect(_content.data(), &QWidget::destroyed, this, &Rollout::deleteLater);

	// Set up title button.
	_titleButton = new QPushButton(title, this);
	_titleButton->setAutoFillBackground(true);
	_titleButton->setFocusPolicy(Qt::NoFocus);
	_titleButton->setStyleSheet("QPushButton { "
							   "  color: white; "
							   "  border-style: solid; "
							   "  border-width: 1px; "
							   "  border-radius: 0px; "
							   "  border-color: black; "
							   "  background-color: grey; "
							   "  padding: 1px; "
							   "}"
							   "QPushButton:pressed { "
							   "  border-color: white; "
							   "}");
	connect(_titleButton, &QPushButton::clicked, this, &Rollout::toggleCollapsed);

	if(helpPage) {
		_helpButton = new QPushButton(QStringLiteral("?"), this);
		_helpButton->setAutoFillBackground(true);
		_helpButton->setFocusPolicy(Qt::NoFocus);
		_helpButton->setToolTip(tr("Open help topic"));
		_helpButton->setStyleSheet("QPushButton { "
								   "  color: white; "
								   "  border-style: solid; "
								   "  border-width: 1px; "
								   "  border-radius: 0px; "
								   "  border-color: black; "
								   "  background-color: rgb(80,130,80); "
								   "  padding: 1px; "
								   "  min-width: 16px; "
								   "}"
								   "QPushButton:pressed { "
								   "  border-color: white; "
								   "}");
		connect(_helpButton, &QPushButton::clicked, this, &Rollout::onHelpButton);
	}
	else _helpButton = nullptr;

	if(params._animateFirstOpening && !params._collapsed)
		setCollapsed(false);
}

/******************************************************************************
* Collapses or opens the rollout.
******************************************************************************/
void Rollout::setCollapsed(bool collapsed)
{
	_collapseAnimation.stop();
	_collapseAnimation.setStartValue(_visiblePercentage);
	_collapseAnimation.setEndValue(collapsed ? 0 : 100);

	// When expanding rollout, adjust scroll position of container so that it becomes visible.
	if(collapsed == false)
		connect(&_collapseAnimation, &QVariantAnimation::valueChanged, this, &Rollout::ensureVisible);
	else
		disconnect(&_collapseAnimation, &QVariantAnimation::valueChanged, this, &Rollout::ensureVisible);

	_collapseAnimation.start();
}

/******************************************************************************
* Makes sure that the rollout is visible in the rollout container.
******************************************************************************/
void Rollout::ensureVisible()
{
	QWidget* p = parentWidget();
	while(p != nullptr) {
		if(RolloutContainer* container = qobject_cast<RolloutContainer*>(p)) {
			container->ensureWidgetVisible(this, 0, 0);
			break;
		}
		p = p->parentWidget();
	}
}

/******************************************************************************
* Computes the recommended size for the widget.
******************************************************************************/
QSize Rollout::sizeHint() const
{
	QSize titleSize = _titleButton->sizeHint();
	QSize contentSize(0,0);
	if(_content)
		contentSize = _content->sizeHint();
	if(_noticeWidget) {
		contentSize.setHeight(contentSize.height() + _noticeWidget->heightForWidth(width()));
	}
	if(_useAvailableSpace) {
		int occupiedSpace = 0;
		for(Rollout* rollout : parentWidget()->findChildren<Rollout*>()) {
			if(rollout->_useAvailableSpace) continue;
			occupiedSpace += rollout->sizeHint().height();
		}
		occupiedSpace += parentWidget()->layout()->spacing() * (parentWidget()->findChildren<Rollout*>().size() - 1);
		int totalSpace = parentWidget()->parentWidget()->height();
		int availSpace = totalSpace - occupiedSpace;
		availSpace -= titleSize.height();
		if(availSpace > contentSize.height())
			contentSize.setHeight(availSpace);
	}
	contentSize.setHeight(contentSize.height() * visiblePercentage() / 100);
	return QSize(std::max(titleSize.width(), contentSize.width()), titleSize.height() + contentSize.height());
}

/******************************************************************************
* Returns the preferred height for this widget, given a width.
******************************************************************************/
int Rollout::heightForWidth(int w) const
{
	if(!_noticeWidget) return -1;

	int titleSize = _titleButton->sizeHint().height();
	int contentSize = 0;
	if(_content)
		contentSize = _content->sizeHint().height();
	if(_noticeWidget)
		contentSize += _noticeWidget->heightForWidth(w);
	if(_useAvailableSpace) {
		int occupiedSpace = 0;
		for(Rollout* rollout : parentWidget()->findChildren<Rollout*>()) {
			if(rollout->_useAvailableSpace) continue;
			occupiedSpace += rollout->sizeHint().height();
		}
		occupiedSpace += parentWidget()->layout()->spacing() * (parentWidget()->findChildren<Rollout*>().size() - 1);
		int totalSpace = parentWidget()->parentWidget()->height();
		int availSpace = totalSpace - occupiedSpace;
		availSpace -= titleSize;
		if(availSpace > contentSize)
			contentSize = availSpace;
	}
	contentSize = contentSize * visiblePercentage() / 100;
	return titleSize + contentSize;
}

/******************************************************************************
* Handles the resize events of the rollout widget.
******************************************************************************/
void Rollout::resizeEvent(QResizeEvent* event)
{
	int titleHeight = _titleButton->sizeHint().height();
	int contentHeight = 0;
	if(_content)
		contentHeight = _content->sizeHint().height();
	int noticeWidgetHeight = 0;
	if(_noticeWidget) {
		noticeWidgetHeight = _noticeWidget->heightForWidth(width());
		contentHeight += noticeWidgetHeight;
	}
	if(_useAvailableSpace) {
		int occupiedSpace = 0;
		for(Rollout* rollout : parentWidget()->findChildren<Rollout*>()) {
			if(rollout->_useAvailableSpace) continue;
			occupiedSpace += rollout->sizeHint().height();
		}
		occupiedSpace += parentWidget()->layout()->spacing() * (parentWidget()->findChildren<Rollout*>().size() - 1);
		int totalSpace = parentWidget()->parentWidget()->height();
		int availSpace = totalSpace - occupiedSpace;
		availSpace -= titleHeight;
		if(availSpace > contentHeight)
			contentHeight = availSpace;
	}
	if(_helpButton) {
		int helpButtonWidth = titleHeight;
		_titleButton->setGeometry(0, 0, width() - helpButtonWidth + 1, titleHeight);
		_helpButton->setGeometry(width() - helpButtonWidth, 0, helpButtonWidth, titleHeight);
	}
	else {
		_titleButton->setGeometry(0, 0, width(), titleHeight);
	}
	int contentY = 0;
	if(_noticeWidget) {
		contentY = noticeWidgetHeight;
		_noticeWidget->setGeometry(0, height() - contentHeight, width(), noticeWidgetHeight);
	}
	if(_content) {
		_content->setGeometry(0, height() - contentHeight + contentY, width(), contentHeight - contentY);
	}
}

/******************************************************************************
* Paints the border around the rollout contents.
******************************************************************************/
void Rollout::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	int y = _titleButton->height() / 2;
	if(height()-y+1 > 0)
		qDrawShadeRect(&painter, 0, y, width()+1, height()-y+1, palette(), true);
}

/******************************************************************************
* Is called when the user presses the help button.
******************************************************************************/
void Rollout::onHelpButton()
{
	MainWindow* mainWindow = qobject_cast<MainWindow*>(window());
	if(mainWindow)
		mainWindow->openHelpTopic(_helpPage);
}

/******************************************************************************
* Displays a notice text at the top of the rollout window.
******************************************************************************/
void Rollout::setNotice(const QString& noticeText)
{
	if(!noticeText.isEmpty()) {
		if(!_noticeWidget) {
			_noticeWidget = new QLabel(noticeText, this);
			_noticeWidget->setMargin(4);
			_noticeWidget->setTextFormat(Qt::RichText);
			_noticeWidget->setTextInteractionFlags(Qt::TextBrowserInteraction);
			_noticeWidget->setOpenExternalLinks(true);
			_noticeWidget->setWordWrap(true);
			_noticeWidget->setAutoFillBackground(true);
			_noticeWidget->lower();
			_noticeWidget->setStyleSheet("QLabel { "
									"  background-color: rgb(230,180,180); "
									"}");
		}
		else
			_noticeWidget->setText(noticeText);
	}
	else {
		delete _noticeWidget;
	}
}

}	// End of namespace
