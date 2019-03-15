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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include <core/app/Application.h>
#include <core/utilities/io/FileManager.h>
#include "PythonBinding.h"

namespace pybind11 { namespace detail {

///////////////////////////////////////////////////////////////////////////////////////
// Helper method for converting a Python string to a QString.
///////////////////////////////////////////////////////////////////////////////////////
QString castToQString(handle src) 
{
#ifndef Q_CC_MSVC
	return src.cast<QString>();
#else
	type_caster<QString> caster;
	if(!caster.load(src, false))
		throw cast_error();
	return (QString&&)std::move(caster);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
// Empty constructor (needed as a workaround for MSVC compiler bug)
///////////////////////////////////////////////////////////////////////////////////////
type_caster<QString>::type_caster() noexcept
{
}
	
///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python string <--> QString conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<QString>::load(handle src, bool) 
{
	if(!src) return false;
	object temp;
	handle load_src = src;
	if(PyUnicode_Check(load_src.ptr())) {
		temp = reinterpret_steal<object>(PyUnicode_AsUTF8String(load_src.ptr()));
		if(!temp) {
			PyErr_Clear(); 
			return false; // UnicodeEncodeError
		}  
		load_src = temp;
	}
	char *buffer;
	ssize_t length;
	if(PYBIND11_BYTES_AS_STRING_AND_SIZE(load_src.ptr(), &buffer, &length)) { 
		PyErr_Clear();
		return false; // TypeError
	}  
	value = QString::fromUtf8(buffer, (int)length);
	return true;
}

handle type_caster<QString>::cast(const QString& src, return_value_policy /* policy */, handle /* parent */) 
{
#if PY_VERSION_HEX >= 0x03030000	// Python 3.3
	OVITO_STATIC_ASSERT(sizeof(QChar) == 2);
	return PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, src.constData(), src.length());
#else
	QByteArray a = src.toUtf8();
	return PyUnicode_FromStringAndSize(a.data(), (ssize_t)a.length());
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
// Empty constructor (needed as a workaround for MSVC compiler bug)
///////////////////////////////////////////////////////////////////////////////////////
type_caster<QUrl>::type_caster() noexcept
{
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python string <--> QUrl conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<QUrl>::load(handle src, bool) 
{
	if(!src) return false;
	try {
		QString str = castToQString(src);
		value = Ovito::Application::instance()->fileManager()->urlFromUserInput(str);
		return true;
	}
	catch(const cast_error&) {
		// ignore
	}
	return false;
}

handle type_caster<QUrl>::cast(const QUrl& src, return_value_policy /* policy */, handle /* parent */) 
{
	QByteArray a = src.toString().toUtf8();
	return PyUnicode_FromStringAndSize(a.data(), (ssize_t)a.length());
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> QVariant conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<QVariant>::load(handle src, bool) 
{
	if(!src) return false;
	try {
		value = QVariant::fromValue(src.cast<int>());
		return true;
	}
	catch(const cast_error&) { /* ignore */ }
	try {
		value = QVariant::fromValue(src.cast<Ovito::FloatType>());
		return true;
	}
	catch(const cast_error&) { /* ignore */ }
	try {
		value = QVariant::fromValue(castToQString(src));
		return true;
	}
	catch(const cast_error&) { /* ignore */ }
	return false;
}

handle type_caster<QVariant>::cast(const QVariant& src, return_value_policy /* policy */, handle /* parent */) 
{
	switch(static_cast<QMetaType::Type>(src.type())) {
		case QMetaType::Bool: return pybind11::cast(src.toBool()).release();
		case QMetaType::Int: return pybind11::cast(src.toInt()).release();
		case QMetaType::UInt: return pybind11::cast(src.toUInt()).release();
		case QMetaType::Long: return pybind11::cast(src.value<long>()).release();
		case QMetaType::ULong: return pybind11::cast(src.value<unsigned long>()).release();
		case QMetaType::LongLong: return pybind11::cast(src.toLongLong()).release();
		case QMetaType::ULongLong: return pybind11::cast(src.toULongLong()).release();
		case QMetaType::Double: return pybind11::cast(src.toDouble()).release();
		case QMetaType::Float: return pybind11::cast(src.toFloat()).release();
		case QMetaType::QString: return pybind11::cast(src.toString()).release();
		case QMetaType::QVariantList:
		{
			list lst;
			QVariantList vlist = src.toList();
			for(int i = 0; i < vlist.size(); i++)
				lst.append(pybind11::cast(vlist[i]));
			return lst.release();
		}
		default: return pybind11::none().release();
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> QStringList conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<QStringList>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	for(size_t i = 0; i < seq.size(); i++)
		value.push_back(castToQString(seq[i]));
	return true;
}

handle type_caster<QStringList>::cast(const QStringList& src, return_value_policy /* policy */, handle /* parent */) 
{
	list lst;
	for(const QString& s : src)
		lst.append(pybind11::cast(s));
	return lst.release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Vector3 conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Vector3>::load(handle src, bool)
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	if(seq.size() != value.size())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.size(); i++)
		value[i] = seq[i].cast<Ovito::Vector3::value_type>();
	return true;
}

handle type_caster<Ovito::Vector3>::cast(const Ovito::Vector3& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::make_tuple(src[0], src[1], src[2]).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Vector3I conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Vector3I>::load(handle src, bool)
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	if(seq.size() != value.size())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.size(); i++)
		value[i] = seq[i].cast<Ovito::Vector3I::value_type>();
	return true;
}

handle type_caster<Ovito::Vector3I>::cast(const Ovito::Vector3I& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::make_tuple(src[0], src[1], src[2]).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Point3 conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Point3>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	if(seq.size() != value.size())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.size(); i++)
		value[i] = seq[i].cast<Ovito::Point3::value_type>();
	return true;
}

handle type_caster<Ovito::Point3>::cast(const Ovito::Point3& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::make_tuple(src[0], src[1], src[2]).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Point3I conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Point3I>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	if(seq.size() != value.size())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.size(); i++)
		value[i] = seq[i].cast<Ovito::Point3I::value_type>();
	return true;
}

handle type_caster<Ovito::Point3I>::cast(const Ovito::Point3I& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::make_tuple(src[0], src[1], src[2]).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Color conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Color>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	if(seq.size() != value.size())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.size(); i++)
		value[i] = seq[i].cast<Ovito::Color::value_type>();
	return true;
}

handle type_caster<Ovito::Color>::cast(const Ovito::Color& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::make_tuple(src[0], src[1], src[2]).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> ColorA conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::ColorA>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq = reinterpret_borrow<sequence>(src);
	if(seq.size() != value.size())
		throw value_error("Expected sequence of length 4.");
	for(size_t i = 0; i < value.size(); i++)
		value[i] = seq[i].cast<Ovito::ColorA::value_type>();
	return true;
}

handle type_caster<Ovito::ColorA>::cast(const Ovito::ColorA& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::make_tuple(src[0], src[1], src[2], src[3]).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> AffineTransformation conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::AffineTransformation>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq1 = reinterpret_borrow<sequence>(src);
	if(seq1.size() != value.row_count())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.row_count(); i++) {
		if(!isinstance<sequence>(seq1[i])) 
			throw value_error("Expected nested sequence of length 4.");
		sequence seq2 = reinterpret_borrow<sequence>(seq1[i]);
		if(seq2.size() != value.col_count())
			throw value_error("Expected nested sequence of length 4.");
		for(size_t j = 0; j < value.col_count(); j++) {
			value(i,j) = seq2[j].cast<Ovito::AffineTransformation::element_type>();
		}
	}
	return true;
}

handle type_caster<Ovito::AffineTransformation>::cast(const Ovito::AffineTransformation& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::array_t<Ovito::AffineTransformation::element_type>({ src.row_count(), src.col_count() }, 
		{ sizeof(Ovito::AffineTransformation::element_type), sizeof(typename Ovito::AffineTransformation::column_type) }, 
		src.elements()).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Matrix3 conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Matrix3>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq1 = reinterpret_borrow<sequence>(src);
	if(seq1.size() != value.row_count())
		throw value_error("Expected sequence of length 3.");
	for(size_t i = 0; i < value.row_count(); i++) {
		if(!isinstance<sequence>(seq1[i])) 
			throw value_error("Expected nested sequence of length 3.");
		sequence seq2 = reinterpret_borrow<sequence>(seq1[i]);
		if(seq2.size() != value.col_count())
			throw value_error("Expected nested sequence of length 3.");
		for(size_t j = 0; j < value.col_count(); j++) {
			value(i,j) = seq2[j].cast<Ovito::Matrix3::element_type>();
		}
	}
	return true;
}

handle type_caster<Ovito::Matrix3>::cast(const Ovito::Matrix3& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::array_t<Ovito::Matrix3::element_type>({ src.row_count(), src.col_count() }, 
		{ sizeof(Ovito::Matrix3::element_type), sizeof(typename Ovito::Matrix3::column_type) }, 
		src.elements()).release();
}

///////////////////////////////////////////////////////////////////////////////////////
// Automatic Python <--> Matrix4 conversion
///////////////////////////////////////////////////////////////////////////////////////
bool type_caster<Ovito::Matrix4>::load(handle src, bool) 
{
	if(!isinstance<sequence>(src)) return false;
	sequence seq1 = reinterpret_borrow<sequence>(src);
	if(seq1.size() != value.row_count())
		throw value_error("Expected sequence of length 4.");
	for(size_t i = 0; i < value.row_count(); i++) {
		if(!isinstance<sequence>(seq1[i])) 
			throw value_error("Expected nested sequence of length 4.");
		sequence seq2 = reinterpret_borrow<sequence>(seq1[i]);
		if(seq2.size() != value.col_count())
			throw value_error("Expected nested sequence of length 4.");
		for(size_t j = 0; j < value.col_count(); j++) {
			value(i,j) = seq2[j].cast<Ovito::Matrix4::element_type>();
		}
	}
	return true;
}

handle type_caster<Ovito::Matrix4>::cast(const Ovito::Matrix4& src, return_value_policy /* policy */, handle /* parent */) 
{
	return pybind11::array_t<Ovito::Matrix4::element_type>({ src.row_count(), src.col_count() }, 
		{ sizeof(Ovito::Matrix4::element_type), sizeof(typename Ovito::Matrix4::column_type) }, 
		src.elements()).release();
}
	
}
}

namespace PyScript {

///////////////////////////////////////////////////////////////////////////////////////
// Initalizes the properties of the new object using the values stored in a dictionary.
///////////////////////////////////////////////////////////////////////////////////////
void ovito_class_initialization_helper::initializeParameters(py::object pyobj, const py::args& args, const py::kwargs& kwargs, const OvitoClass& clazz) 
{
	if(py::len(args) > 0) {
		if(py::len(args) > 1 || !PyDict_Check(args[0].ptr()))
			throw Exception("Constructor function accepts only keyword arguments.");
	}
	// Set attributes based on keyword arguments.
	if(kwargs)
		applyParameters(pyobj, kwargs, clazz);
	// The caller may alternatively provide a dictionary with attributes.
	if(py::len(args) == 1) {
		applyParameters(pyobj, args[0].cast<py::dict>(), clazz);
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Sets attributes of the given object as specified in the dictionary.
///////////////////////////////////////////////////////////////////////////////////////
void ovito_class_initialization_helper::applyParameters(py::object& pyobj, const py::dict& params, const OvitoClass& clazz) 
{
	// Iterate over the keys of the dictionary and set attributes of the
	// newly created object.
	for(const auto& item : params) {
		// Check if the attribute exists. Otherwise raise error.
		if(!py::hasattr(pyobj, item.first)) {
			PyErr_SetObject(PyExc_AttributeError, 
				py::str("Object type {} does not have an attribute named '{}'.").format(clazz.className(), item.first).ptr());
			throw py::error_already_set();
		}
		// Set attribute value.
		py::setattr(pyobj, item.first, item.second);
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Determine the DataSet from the context the current script is running in.
///////////////////////////////////////////////////////////////////////////////////////
DataSet* ovito_class_initialization_helper::getCurrentDataset()
{
	return ScriptEngine::getCurrentDataset();
}

}
