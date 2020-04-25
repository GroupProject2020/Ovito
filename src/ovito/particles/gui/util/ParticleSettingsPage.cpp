////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Alexander Stukowski
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

#include <ovito/particles/gui/ParticlesGui.h>
#include <ovito/particles/objects/ParticleType.h>
#include "ParticleSettingsPage.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(ParticleSettingsPage);

class NameColumnDelegate : public QStyledItemDelegate
{
public:
	NameColumnDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override { return nullptr; }
};

class RadiusColumnDelegate : public QStyledItemDelegate
{
public:
	RadiusColumnDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    	if(!index.model()->data(index, Qt::EditRole).isValid())
    		return nullptr;
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setFrame(false);
		editor->setMinimum(0);
		editor->setSingleStep(0.1);
		return editor;
	}

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
    	double value = index.model()->data(index, Qt::EditRole).toDouble();
    	QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
    	QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
    	spinBox->interpretText();
    	double value = spinBox->value();
    	model->setData(index, value, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    	editor->setGeometry(option.rect);
    }

    QString displayText(const QVariant& value, const QLocale& locale) const override {
    	if(value.isValid())
    		return QString::number(value.toDouble());
    	else
    		return QString();
    }
};

class TransparencyColumnDelegate : public QStyledItemDelegate
{
public:
    TransparencyColumnDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if(!index.model()->data(index, Qt::EditRole).isValid())
            return nullptr;
        QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
        editor->setFrame(false);
        editor->setMinimum(0);
        editor->setSingleStep(0.05);
        return editor;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        double value = index.model()->data(index, Qt::EditRole).toDouble();
        QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
        spinBox->setValue(value);
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
        spinBox->interpretText();
        double value = spinBox->value();
        model->setData(index, value, Qt::EditRole);
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        editor->setGeometry(option.rect);
    }

    QString displayText(const QVariant& value, const QLocale& locale) const override {
        if(value.isValid())
            return QString::number(value.toDouble());
        else
            return QString();
    }
};


class ColorColumnDelegate : public QStyledItemDelegate
{
public:
	ColorColumnDelegate(QObject* parent = 0) : QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    	QColor oldColor = index.model()->data(index, Qt::EditRole).value<QColor>();
    	QString ptypeName = index.sibling(index.row(), 0).data().toString();
    	QColor newColor = QColorDialog::getColor(oldColor, parent->window(), tr("Select color for '%1'").arg(ptypeName));
    	if(newColor.isValid()) {
    		const_cast<QAbstractItemModel*>(index.model())->setData(index, QVariant::fromValue(newColor), Qt::EditRole);
    	}
		return nullptr;
	}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    	QBrush brush(index.model()->data(index, Qt::EditRole).value<QColor>());
    	painter->fillRect(option.rect, brush);
    }
};

/******************************************************************************
* Creates the widget that contains the plugin specific setting controls.
******************************************************************************/
void ParticleSettingsPage::insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget)
{
	QWidget* page = new QWidget();
	tabWidget->addTab(page, tr("Particles"));
	QVBoxLayout* layout1 = new QVBoxLayout(page);
	layout1->setSpacing(0);

	_particleTypesItem = new QTreeWidgetItem(QStringList() << tr("Particle types") << QString() << QString());
	_particleTypesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
	_structureTypesItem = new QTreeWidgetItem(QStringList() << tr("Structure types") << QString() << QString());
	_structureTypesItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

	QStringList typeNames;
	for(int i = 0; i < ParticleType::PredefinedParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES; i++)
		typeNames << ParticleType::getPredefinedParticleTypeName((ParticleType::PredefinedParticleType)i);
	QSettings settings;
	settings.beginGroup("particles/defaults/color");
	settings.beginGroup(QString::number((int)ParticlesObject::TypeProperty));
	typeNames.append(settings.childKeys());
	settings.endGroup();
	settings.endGroup();
	settings.beginGroup("particles/defaults/radius");
	settings.beginGroup(QString::number((int)ParticlesObject::TypeProperty));
	typeNames.append(settings.childKeys());
	settings.endGroup();
	settings.endGroup();
	//Begin of modifications
	settings.beginGroup("particles/defaults/transparency"); //Create a new group that will contain the spinner for transparency
	settings.beginGroup(QString::number((int)ParticlesObject::TypeProperty));
	typeNames.append(settings.childKeys());
	settings.endGroup();
	settings.endGroup();
	//End of modifications
	typeNames.removeDuplicates();

	for(const QString& tname : typeNames) {
		QTreeWidgetItem* childItem = new QTreeWidgetItem();
		childItem->setText(0, tname);
		Color color = ParticleType::getDefaultParticleColor(ParticlesObject::TypeProperty, tname, 0);
		FloatType radius = ParticleType::getDefaultParticleRadius(ParticlesObject::TypeProperty, tname, 0);
		//Modif
		FloatType transparency = ParticleType::getDefaultParticleTransparency(ParticlesObject::TypeProperty, tname, 0);
		childItem->setData(1, Qt::DisplayRole, QVariant::fromValue((QColor)color));
		childItem->setData(2, Qt::DisplayRole, QVariant::fromValue(radius));
		//Modif
		childItem->setData(3, Qt::DisplayRole, QVariant::fromValue(transparency));
		childItem->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
		_particleTypesItem->addChild(childItem);
	}

	QStringList structureNames;
	for(int i = 0; i < ParticleType::PredefinedStructureType::NUMBER_OF_PREDEFINED_STRUCTURE_TYPES; i++)
		structureNames << ParticleType::getPredefinedStructureTypeName((ParticleType::PredefinedStructureType)i);
	settings.beginGroup("particles/defaults/color");
	settings.beginGroup(QString::number((int)ParticlesObject::StructureTypeProperty));
	structureNames.append(settings.childKeys());
	structureNames.removeDuplicates();

	for(const QString& tname : structureNames) {
		QTreeWidgetItem* childItem = new QTreeWidgetItem();
		childItem->setText(0, tname);
		Color color = ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, tname, 0);
		childItem->setData(1, Qt::DisplayRole, QVariant::fromValue((QColor)color));
		childItem->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
		_structureTypesItem->addChild(childItem);
	}

	layout1->addWidget(new QLabel(tr("Default particle colors and sizes:")));
	_predefTypesTable = new QTreeWidget();
	layout1->addWidget(_predefTypesTable, 1);
	_predefTypesTable->setColumnCount(3);
	_predefTypesTable->setHeaderLabels(QStringList() << tr("Type") << tr("Color") << tr("Radius"));
	_predefTypesTable->setRootIsDecorated(true);
	_predefTypesTable->setAllColumnsShowFocus(true);
	_predefTypesTable->addTopLevelItem(_particleTypesItem);
	_predefTypesTable->setFirstItemColumnSpanned(_particleTypesItem, true);
	_predefTypesTable->addTopLevelItem(_structureTypesItem);
	_predefTypesTable->setFirstItemColumnSpanned(_structureTypesItem, true);
	_predefTypesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	_predefTypesTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
	_predefTypesTable->setColumnWidth(0, 280);

	NameColumnDelegate* nameDelegate = new NameColumnDelegate(this);
	_predefTypesTable->setItemDelegateForColumn(0, nameDelegate);
	ColorColumnDelegate* colorDelegate = new ColorColumnDelegate(this);
	_predefTypesTable->setItemDelegateForColumn(1, colorDelegate);
	RadiusColumnDelegate* radiusDelegate = new RadiusColumnDelegate(this);
	_predefTypesTable->setItemDelegateForColumn(2, radiusDelegate);
	std::cout << " DELEGATIONSSSSSSSSSSS\n";
	TransparencyColumnDelegate* transparencyDelegate = new TransparencyColumnDelegate(this);
	_predefTypesTable->setItemDelegateForColumn(3, transparencyDelegate);

	QHBoxLayout* buttonLayout = new QHBoxLayout();
	buttonLayout->setContentsMargins(0,0,0,0);
	QPushButton* restoreBuiltinDefaultsButton = new QPushButton(tr("Restore built-in defaults"));
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(restoreBuiltinDefaultsButton);
	connect(restoreBuiltinDefaultsButton, &QPushButton::clicked, this, &ParticleSettingsPage::restoreBuiltinParticlePresets);
	layout1->addLayout(buttonLayout);
}

/******************************************************************************
* Lets the page save all changed settings.
******************************************************************************/
bool ParticleSettingsPage::saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget)
{
	// Clear all existing user-defined settings first.
	QSettings settings;
	settings.beginGroup("particles/defaults/color");
	settings.beginGroup(QString::number((int)ParticlesObject::TypeProperty));
	settings.remove(QString());
	settings.endGroup();
	settings.endGroup();
	settings.beginGroup("particles/defaults/radius");
	settings.beginGroup(QString::number((int)ParticlesObject::TypeProperty));
	settings.remove(QString());
	//Begin of modification
	settings.endGroup();
	settings.endGroup();
	settings.beginGroup("particles/defaults/transparency");
	settings.beginGroup(QString::number((int)ParticlesObject::TypeProperty));
	settings.remove(QString());
	//End of modification

	for(int i = 0; i < _particleTypesItem->childCount(); i++) {
		QTreeWidgetItem* item = _particleTypesItem->child(i);
		QColor color = item->data(1, Qt::DisplayRole).value<QColor>();
		FloatType radius = item->data(2, Qt::DisplayRole).value<FloatType>();
		FloatType transparency = item->data(4, Qt::DisplayRole).value<FloatType>();
		std::cout << "Transparency of UI is equal to: " << transparency << std::endl;
		color.setAlphaF((qreal)item->data(3, Qt::DisplayRole).value<FloatType>());
		ParticleType::setDefaultParticleColor(ParticlesObject::TypeProperty, item->text(0), color);
		ParticleType::setDefaultParticleRadius(ParticlesObject::TypeProperty, item->text(0), radius);
		//Modif
		ParticleType::setDefaultParticleTransparency(ParticlesObject::TypeProperty, item->text(0), transparency);
	}

	for(int i = 0; i < _structureTypesItem->childCount(); i++) {
		QTreeWidgetItem* item = _structureTypesItem->child(i);
		QColor color = item->data(1, Qt::DisplayRole).value<QColor>();
		color.setAlphaF((qreal)item->data(3, Qt::DisplayRole).value<FloatType>());
		ParticleType::setDefaultParticleColor(ParticlesObject::StructureTypeProperty, item->text(0), color);
	}

	return true;
}

/******************************************************************************
* Restores the built-in default particle colors and sizes.
******************************************************************************/
void ParticleSettingsPage::restoreBuiltinParticlePresets()
{
	for(int i = 0; i < ParticleType::PredefinedParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES; i++) {
		QTreeWidgetItem* item = _particleTypesItem->child(i);
		Color color = ParticleType::getDefaultParticleColor(ParticlesObject::TypeProperty, item->text(0), 0, false);
		FloatType radius = ParticleType::getDefaultParticleRadius(ParticlesObject::TypeProperty, item->text(0), 0, false);
		FloatType transparency = ParticleType::getDefaultParticleTransparency(ParticlesObject::TypeProperty, item->text(0), 0, false);
		//color.a() = (float)transparency;
		item->setData(1, Qt::DisplayRole, QVariant::fromValue((QColor)color));
		item->setData(2, Qt::DisplayRole, QVariant::fromValue(radius));
		item->setData(3, Qt::DisplayRole, QVariant::fromValue(transparency));
	}
	for(int i = _particleTypesItem->childCount() - 1; i >= ParticleType::PredefinedParticleType::NUMBER_OF_PREDEFINED_PARTICLE_TYPES; i--) {
		delete _particleTypesItem->takeChild(i);
	}

	for(int i = 0; i < ParticleType::PredefinedStructureType::NUMBER_OF_PREDEFINED_STRUCTURE_TYPES; i++) {
		QTreeWidgetItem* item = _structureTypesItem->child(i);
		Color color = ParticleType::getDefaultParticleColor(ParticlesObject::StructureTypeProperty, item->text(0), 0, false);
		item->setData(1, Qt::DisplayRole, QVariant::fromValue((QColor)color));
	}
}

}	// End of namespace
}	// End of namespace
