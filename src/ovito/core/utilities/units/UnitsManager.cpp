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

#include <ovito/core/Core.h>
#include <ovito/core/dataset/DataSet.h>
#include <ovito/core/dataset/animation/AnimationSettings.h>
#include "UnitsManager.h"

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
UnitsManager::UnitsManager(DataSet* dataset) : _dataset(dataset)
{
	// Create standard unit objects.
	_units[&FloatParameterUnit::staticMetaObject] = _floatIdentityUnit = new FloatParameterUnit(this, dataset);
	_units[&IntegerParameterUnit::staticMetaObject] = _integerIdentityUnit = new IntegerParameterUnit(this, dataset);
	_units[&TimeParameterUnit::staticMetaObject] = _timeUnit = new TimeParameterUnit(this, dataset);
	_units[&PercentParameterUnit::staticMetaObject] = _percentUnit = new PercentParameterUnit(this, dataset);
	_units[&AngleParameterUnit::staticMetaObject] = _angleUnit = new AngleParameterUnit(this, dataset);
	_units[&WorldParameterUnit::staticMetaObject] = _worldUnit = new WorldParameterUnit(this, dataset);
}

/******************************************************************************
* Returns the global instance of the given parameter unit service.
******************************************************************************/
ParameterUnit* UnitsManager::getUnit(const QMetaObject* parameterUnitClass)
{
	OVITO_CHECK_POINTER(parameterUnitClass);

	auto iter = _units.find(parameterUnitClass);
	if(iter != _units.end())
		return iter->second;

	// Create an instance of this class.
	ParameterUnit* unit = qobject_cast<ParameterUnit*>(parameterUnitClass->newInstance(Q_ARG(QObject*, this), Q_ARG(DataSet*, _dataset)));
	OVITO_ASSERT_MSG(unit != nullptr, "UnitsManager::getUnit()", "Failed to create instance of requested parameter unit type.");
	_units.insert({ parameterUnitClass, unit });

	return unit;
}

/******************************************************************************
* Converts the given string to a value.
******************************************************************************/
FloatType FloatParameterUnit::parseString(const QString& valueString)
{
	double value;
	bool ok;
	value = valueString.toDouble(&ok);
	if(!ok)
		dataset()->throwException(tr("Invalid floating-point value: %1").arg(valueString));
	return (FloatType)value;
}

/******************************************************************************
* Converts the given string to a value.
******************************************************************************/
FloatType IntegerParameterUnit::parseString(const QString& valueString)
{
	int value;
	bool ok;
	value = valueString.toInt(&ok);
	if(!ok)
		dataset()->throwException(tr("Invalid integer value: %1").arg(valueString));
	return (FloatType)value;
}

/******************************************************************************
* Converts the given string to a value.
******************************************************************************/
FloatType PercentParameterUnit::parseString(const QString& valueString)
{
	return FloatParameterUnit::parseString(QString(valueString).remove(QChar('%')));
}

/******************************************************************************
* Converts a numeric value to a string.
******************************************************************************/
QString PercentParameterUnit::formatValue(FloatType value)
{
	return FloatParameterUnit::formatValue(value) + QStringLiteral("%");
}

/******************************************************************************
* Constructor.
******************************************************************************/
TimeParameterUnit::TimeParameterUnit(QObject* parent, DataSet* dataset) : IntegerParameterUnit(parent, dataset)
{
	connect(dataset, &DataSet::animationSettingsReplaced, this, &TimeParameterUnit::onAnimationSettingsReplaced);
	_animSettings = dataset->animationSettings();
}

/******************************************************************************
* Converts the given string to a time value.
******************************************************************************/
FloatType TimeParameterUnit::parseString(const QString& valueString)
{
	if(!_animSettings) return 0;
	return _animSettings->stringToTime(valueString);
}

/******************************************************************************
* Converts a time value to a string.
******************************************************************************/
QString TimeParameterUnit::formatValue(FloatType value)
{
	if(!_animSettings) return QString();
	return _animSettings->timeToString((TimePoint)value);
}

/******************************************************************************
* Returns the (positive) step size used by spinner widgets for this
* parameter unit type.
******************************************************************************/
FloatType TimeParameterUnit::stepSize(FloatType currentValue, bool upDirection)
{
	if(!_animSettings) return 0;
	if(upDirection)
		return ceil((currentValue + FloatType(1)) / _animSettings->ticksPerFrame()) * _animSettings->ticksPerFrame() - currentValue;
	else
		return currentValue - floor((currentValue - FloatType(1)) / _animSettings->ticksPerFrame()) * _animSettings->ticksPerFrame();
}

/******************************************************************************
* Given an arbitrary value, which is potentially invalid, rounds it to the
* closest valid value.
******************************************************************************/
FloatType TimeParameterUnit::roundValue(FloatType value)
{
	if(!_animSettings) return value;
	return floor(value / _animSettings->ticksPerFrame() + FloatType(0.5)) * _animSettings->ticksPerFrame();
}

/******************************************************************************
* This is called whenever the current animation settings of the dataset have
* been replaced by new ones.
******************************************************************************/
void TimeParameterUnit::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	disconnect(_speedChangedConnection);
	disconnect(_timeFormatChangedConnection);
	_animSettings = newAnimationSettings;
	if(newAnimationSettings) {
		_speedChangedConnection = connect(newAnimationSettings, &AnimationSettings::speedChanged, this, &TimeParameterUnit::formatChanged);
		_timeFormatChangedConnection = connect(newAnimationSettings, &AnimationSettings::timeFormatChanged, this, &TimeParameterUnit::formatChanged);
	}
	Q_EMIT formatChanged();
}

}	// End of namespace
