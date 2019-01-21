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

#include <gui/GUI.h>
#include <gui/widgets/general/SpinnerWidget.h>
#include <gui/properties/PropertiesEditor.h>
#include <gui/properties/PropertiesPanel.h>
#include <gui/properties/DefaultPropertiesEditor.h>
#include <gui/mainwin/MainWindow.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <core/dataset/io/FileExporter.h>
#include "FileExporterSettingsDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
FileExporterSettingsDialog::FileExporterSettingsDialog(MainWindow* mainWindow, FileExporter* exporter)
	: QDialog(mainWindow), _exporter(exporter)
{
	setWindowTitle(tr("Data Export Settings"));

	_mainLayout = new QVBoxLayout(this);
	QGroupBox* groupBox;
	QGridLayout* groupLayout;

	// Create "Export sequence" group box for selecting the animation interval to be exported.
	groupBox = new QGroupBox(tr("Export frame sequence"), this);
	_mainLayout->addWidget(groupBox);

	groupLayout = new QGridLayout(groupBox);
	groupLayout->setColumnStretch(0, 5);
	groupLayout->setColumnStretch(1, 95);
	_rangeButtonGroup = new QButtonGroup(this);

	bool exportAnim = _exporter->exportAnimation();
	if(!exportAnim && exporter->dataset()->animationSettings()->animationInterval().duration() == 0)
		groupBox->setEnabled(false);
	else
		_skipDialog = false;
	QRadioButton* singleFrameBtn = new QRadioButton(tr("Current frame only"));
	_rangeButtonGroup->addButton(singleFrameBtn, 0);
	groupLayout->addWidget(singleFrameBtn, 0, 0, 1, 2);
	singleFrameBtn->setChecked(!exportAnim);

	QHBoxLayout* frameRangeLayout = new QHBoxLayout();
	frameRangeLayout->setSpacing(0);
	groupLayout->addLayout(frameRangeLayout, 1, 0, 1, 2);

	QRadioButton* frameSequenceBtn = new QRadioButton(tr("Range:"));
	_rangeButtonGroup->addButton(frameSequenceBtn, 1);
	frameRangeLayout->addWidget(frameSequenceBtn);
	frameRangeLayout->addSpacing(10);
	frameSequenceBtn->setChecked(exportAnim);
	frameSequenceBtn->setEnabled(exporter->dataset()->animationSettings()->animationInterval().duration() != 0);

	class ShortLineEdit : public QLineEdit {
	public:
		virtual QSize sizeHint() const override { 
			QSize s = QLineEdit::sizeHint();
			return QSize(s.width() / 2, s.height());
		}
	};

	frameRangeLayout->addWidget(new QLabel(tr("From frame:")));
	_startTimeSpinner = new SpinnerWidget();
	_startTimeSpinner->setUnit(exporter->dataset()->unitsManager().timeUnit());
	_startTimeSpinner->setIntValue(exporter->dataset()->animationSettings()->frameToTime(_exporter->startFrame()));
	_startTimeSpinner->setTextBox(new ShortLineEdit());
	_startTimeSpinner->setMinValue(exporter->dataset()->animationSettings()->animationInterval().start());
	_startTimeSpinner->setMaxValue(exporter->dataset()->animationSettings()->animationInterval().end());
	frameRangeLayout->addWidget(_startTimeSpinner->textBox());
	frameRangeLayout->addWidget(_startTimeSpinner);
	frameRangeLayout->addSpacing(8);
	frameRangeLayout->addWidget(new QLabel(tr("To:")));
	_endTimeSpinner = new SpinnerWidget();
	_endTimeSpinner->setUnit(exporter->dataset()->unitsManager().timeUnit());
	_endTimeSpinner->setIntValue(exporter->dataset()->animationSettings()->frameToTime(_exporter->endFrame()));
	_endTimeSpinner->setTextBox(new ShortLineEdit());
	_endTimeSpinner->setMinValue(exporter->dataset()->animationSettings()->animationInterval().start());
	_endTimeSpinner->setMaxValue(exporter->dataset()->animationSettings()->animationInterval().end());
	frameRangeLayout->addWidget(_endTimeSpinner->textBox());
	frameRangeLayout->addWidget(_endTimeSpinner);
	frameRangeLayout->addSpacing(8);
	frameRangeLayout->addWidget(new QLabel(tr("Every Nth frame:")));
	_nthFrameSpinner = new SpinnerWidget();
	_nthFrameSpinner->setUnit(exporter->dataset()->unitsManager().integerIdentityUnit());
	_nthFrameSpinner->setIntValue(_exporter->everyNthFrame());
	_nthFrameSpinner->setTextBox(new ShortLineEdit());
	_nthFrameSpinner->setMinValue(1);
	frameRangeLayout->addWidget(_nthFrameSpinner->textBox());
	frameRangeLayout->addWidget(_nthFrameSpinner);

	_startTimeSpinner->setEnabled(frameSequenceBtn->isChecked());
	_endTimeSpinner->setEnabled(frameSequenceBtn->isChecked());
	_nthFrameSpinner->setEnabled(frameSequenceBtn->isChecked());
	connect(frameSequenceBtn, &QRadioButton::toggled, _startTimeSpinner, &SpinnerWidget::setEnabled);
	connect(frameSequenceBtn, &QRadioButton::toggled, _endTimeSpinner, &SpinnerWidget::setEnabled);
	connect(frameSequenceBtn, &QRadioButton::toggled, _nthFrameSpinner, &SpinnerWidget::setEnabled);

	QGridLayout* fileGroupLayout = new QGridLayout();
	fileGroupLayout->setColumnStretch(1, 1);
	groupLayout->addLayout(fileGroupLayout, 2, 1, 1, 1);

	if(_exporter->supportsMultiFrameFiles()) {
		_fileGroupButtonGroup = new QButtonGroup(this);
		QRadioButton* singleOutputFileBtn = new QRadioButton(tr("Single output file:   %1").arg(QFileInfo(_exporter->outputFilename()).fileName()));
		_fileGroupButtonGroup->addButton(singleOutputFileBtn, 0);
		fileGroupLayout->addWidget(singleOutputFileBtn, 0, 0, 1, 2);
		singleOutputFileBtn->setChecked(!_exporter->useWildcardFilename());

		QRadioButton* multipleFilesBtn = new QRadioButton(tr("File sequence:"));
		_fileGroupButtonGroup->addButton(multipleFilesBtn, 1);
		fileGroupLayout->addWidget(multipleFilesBtn, 1, 0, 1, 1);
		multipleFilesBtn->setChecked(_exporter->useWildcardFilename());

		_wildcardTextbox = new QLineEdit(_exporter->wildcardFilename());
		fileGroupLayout->addWidget(_wildcardTextbox, 1, 1, 1, 1);
		_wildcardTextbox->setEnabled(multipleFilesBtn->isChecked());
		connect(multipleFilesBtn, &QRadioButton::toggled, _wildcardTextbox, &QLineEdit::setEnabled);

		singleOutputFileBtn->setEnabled(frameSequenceBtn->isChecked());
		multipleFilesBtn->setEnabled(frameSequenceBtn->isChecked());
		connect(frameSequenceBtn, &QRadioButton::toggled, singleOutputFileBtn, &QWidget::setEnabled);
		connect(frameSequenceBtn, &QRadioButton::toggled, multipleFilesBtn, &QWidget::setEnabled);
	}
	else {
		_wildcardTextbox = new QLineEdit(_exporter->wildcardFilename());
		fileGroupLayout->addWidget(new QLabel(tr("Filename pattern:")), 0, 0, 1, 1);
		fileGroupLayout->addWidget(_wildcardTextbox, 0, 1, 1, 1);
	}
	_wildcardTextbox->setEnabled(frameSequenceBtn->isChecked());
	connect(frameSequenceBtn, &QRadioButton::toggled, _wildcardTextbox, &QWidget::setEnabled);

	// Create "Data" group box for selection of the source data pipeline and source data object.
	groupBox = new QGroupBox(tr("Data to export"), this);
	_mainLayout->addWidget(groupBox);

	groupLayout = new QGridLayout(groupBox);
	_sceneNodeBox = new QComboBox();
	_dataObjectBox = new QComboBox();
	groupLayout->addWidget(new QLabel(tr("Pipeline:")), 0, 0);
	groupLayout->setColumnStretch(1, 1);
	groupLayout->addWidget(_sceneNodeBox, 0, 1);
	groupLayout->setColumnMinimumWidth(2, 10);
	groupLayout->addWidget(new QLabel(tr("Data object:")), 0, 3);
	groupLayout->setColumnStretch(4, 1);
	groupLayout->addWidget(_dataObjectBox, 0, 4);

	exporter->dataset()->sceneRoot()->visitChildren([this, exporter](SceneNode* node) {
		if(exporter->isSuitableNode(node)) {
			_sceneNodeBox->addItem(node->objectTitle(), QVariant::fromValue(OORef<OvitoObject>(node)));
			if(node == exporter->nodeToExport())
				_sceneNodeBox->setCurrentIndex(_sceneNodeBox->count() - 1);
		}
		return true;
	});
	updateDataObjectList();
	if(_sceneNodeBox->count() <= 1)
		_sceneNodeBox->setEnabled(false);
	if(_dataObjectBox->count() <= 1)
		_dataObjectBox->setEnabled(false);
	if(_sceneNodeBox->isEnabled() == false && _dataObjectBox->isEnabled() == false)
		groupBox->setEnabled(false);
	else
		_skipDialog = false;

	// Update exporter whenever a new source pipeline has been selected by the user.
	connect(_sceneNodeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
		_exporter->setNodeToExport(static_object_cast<SceneNode>(_sceneNodeBox->currentData().value<OORef<OvitoObject>>()));
	});

	// Update the list of available data objects whenever the user selects a different source pipeline.
	connect(_sceneNodeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileExporterSettingsDialog::updateDataObjectList);

	// Update the exporter whenever the user selects a new data object.
	connect(_dataObjectBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
		_exporter->setDataObjectToExport(_dataObjectBox->currentData().value<DataObjectReference>());
	});

	// Show the optional UI of the exporter.
	if(OORef<PropertiesEditor> editor = PropertiesEditor::create(exporter)) {
		if(editor->getOOMetaClass() != DefaultPropertiesEditor::OOClass()) {
			PropertiesPanel* propPanel = new PropertiesPanel(this, mainWindow);
			_mainLayout->addWidget(propPanel);
			propPanel->setEditObject(exporter);
			_skipDialog = false;
		}
	}

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &FileExporterSettingsDialog::onOk);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &FileExporterSettingsDialog::reject);
	_mainLayout->addWidget(buttonBox);
}

/******************************************************************************
* Updates the displayed list of data object available for export.
******************************************************************************/
void FileExporterSettingsDialog::updateDataObjectList()
{
	// Update the data objects list.
	_dataObjectBox->clear();

	std::vector<const DataObject::OOMetaClass*> objClasses = _exporter->exportableDataObjectClass();
	if(!objClasses.empty()) {
		if(PipelineSceneNode* pipeline = dynamic_object_cast<PipelineSceneNode>(_exporter->nodeToExport())) {
			const PipelineFlowState& state = pipeline->evaluatePipelinePreliminary(true);
			if(!state.isEmpty()) {
				for(const DataObject::OOMetaClass* clazz : objClasses) {
					OVITO_ASSERT(clazz != nullptr);
					for(const ConstDataObjectPath& dataPath : state.data()->getObjectsRecursive(*clazz)) {
						QString title = dataPath.back()->objectTitle();
						DataObjectReference dataRef(clazz, dataPath.toString(), title);
						_dataObjectBox->addItem(title, QVariant::fromValue(dataRef));
						if(dataRef == _exporter->dataObjectToExport())
							_dataObjectBox->setCurrentIndex(_dataObjectBox->count() - 1);
					}
				}
			}
		}
		if(_dataObjectBox->count() > 1) {
			_dataObjectBox->setEnabled(true);
			_exporter->setDataObjectToExport(_dataObjectBox->currentData().value<DataObjectReference>());
		}
		else if(_dataObjectBox->count() == 1) {
			_dataObjectBox->setEnabled(false);
			_exporter->setDataObjectToExport(_dataObjectBox->currentData().value<DataObjectReference>());
		}
		else {
			_dataObjectBox->setEnabled(false);
			_dataObjectBox->addItem(tr("<no exportable data>"));
		}
	}
	else {
		_dataObjectBox->setEnabled(false);
	}
}

/******************************************************************************
* This is called when the user has pressed the OK button.
******************************************************************************/
void FileExporterSettingsDialog::onOk()
{
	try {
		_exporter->setExportAnimation(_rangeButtonGroup->checkedId() == 1);
		_exporter->setUseWildcardFilename(_fileGroupButtonGroup ? (_fileGroupButtonGroup->checkedId() == 1) : _exporter->exportAnimation());
		_exporter->setWildcardFilename(_wildcardTextbox->text());
		_exporter->setStartFrame(_exporter->dataset()->animationSettings()->timeToFrame(_startTimeSpinner->intValue()));
		_exporter->setEndFrame(_exporter->dataset()->animationSettings()->timeToFrame(std::max(_endTimeSpinner->intValue(), _startTimeSpinner->intValue())));
		_exporter->setEveryNthFrame(_nthFrameSpinner->intValue());

		accept();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
