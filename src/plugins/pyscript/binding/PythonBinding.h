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


#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include <core/oo/OORef.h>

PYBIND11_DECLARE_HOLDER_TYPE(T, Ovito::OORef<T>, true);

// Needed by modifier_operate_on_list() below.
PYBIND11_MAKE_OPAQUE(std::vector<Ovito::OORef<Ovito::ModifierDelegate>>);


namespace pybind11 { namespace detail {

	/// Automatic Python string <--> QString conversion
    template<> struct type_caster<QString> {
    public:
        PYBIND11_TYPE_CASTER(QString, _("QString"));

        bool load(handle src, bool) {
            if(!src) return false;
            object temp;
            handle load_src = src;
			if(PyUnicode_Check(load_src.ptr())) {
                temp = reinterpret_steal<object>(PyUnicode_AsUTF8String(load_src.ptr()));
                if (!temp) { PyErr_Clear(); return false; }  // UnicodeEncodeError
                load_src = temp;
            }
            char *buffer;
            ssize_t length;
            int err = PYBIND11_BYTES_AS_STRING_AND_SIZE(load_src.ptr(), &buffer, &length);
            if (err == -1) { PyErr_Clear(); return false; }  // TypeError
            value = QString::fromUtf8(buffer, (int)length);
            return true;
        }

        static handle cast(const QString& src, return_value_policy /* policy */, handle /* parent */) {
#if PY_VERSION_HEX >= 0x03030000	// Python 3.3
			OVITO_STATIC_ASSERT(sizeof(QChar) == 2);
			return PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, src.constData(), src.length());
#else
        	QByteArray a = src.toUtf8();
        	return PyUnicode_FromStringAndSize(a.data(), (ssize_t)a.length());
#endif
        }
    };
	
	inline QString castToQString(handle src) {
#ifndef Q_CC_MSVC
		return src.cast<QString>();
#else
		type_caster<QString> caster;
		if(!caster.load(src, false))
			throw cast_error();
		return (QString&)caster;
#endif
	}

	/// Automatic Python string <--> QUrl conversion
    template<> struct type_caster<QUrl> {
    public:
        PYBIND11_TYPE_CASTER(QUrl, _("QUrl"));

        bool load(handle src, bool) {
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

        static handle cast(const QUrl& src, return_value_policy /* policy */, handle /* parent */) {
        	QByteArray a = src.toString().toUtf8();
        	return PyUnicode_FromStringAndSize(a.data(), (ssize_t)a.length());
        }
    };	

	/// Automatic Python <--> QVariant conversion
    template<> struct type_caster<QVariant> {
    public:
        bool load(handle src, bool) {
			return false;
        }

        static handle cast(const QVariant& src, return_value_policy /* policy */, handle /* parent */) {
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
				default: return pybind11::none();
			}
        }

		PYBIND11_TYPE_CASTER(QVariant, _("QVariant"));
    };	

	/// Automatic Python <--> QStringList conversion
    template<> struct type_caster<QStringList> {
    public:
		PYBIND11_TYPE_CASTER(QStringList, _("QStringList"));
		
        bool load(handle src, bool) {
			if(!isinstance<sequence>(src)) return false;
			sequence seq = reinterpret_borrow<sequence>(src);
			for(size_t i = 0; i < seq.size(); i++)
				value.push_back(castToQString(seq[i]));
			return true;
        }

        static handle cast(const QStringList& src, return_value_policy /* policy */, handle /* parent */) {
			list lst;
			for(const QString& s : src)
				lst.append(pybind11::cast(s));
			return lst.release();
        }
    };

	/// Automatic Python <--> Vector3 conversion
    template<typename T> struct type_caster<Ovito::Vector_3<T>> {
    public:
        bool load(handle src, bool) {
			if(!isinstance<sequence>(src)) return false;
			sequence seq = reinterpret_borrow<sequence>(src);
			if(seq.size() != value.size())
				throw value_error("Expected sequence of length 3.");
			for(size_t i = 0; i < value.size(); i++)
				value[i] = seq[i].cast<T>();
			return true;
        }

        static handle cast(const Ovito::Vector_3<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::make_tuple(src[0], src[1], src[2]).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::Vector_3<T>, _("Vector3<") + make_caster<T>::name() + _(">"));
    };	

	/// Automatic Python <--> Point3 conversion
    template<typename T> struct type_caster<Ovito::Point_3<T>> {
    public:
        bool load(handle src, bool) {
			if(!isinstance<sequence>(src)) return false;
			sequence seq = reinterpret_borrow<sequence>(src);
			if(seq.size() != value.size())
				throw value_error("Expected sequence of length 3.");
			for(size_t i = 0; i < value.size(); i++)
				value[i] = seq[i].cast<T>();
			return true;
        }

        static handle cast(const Ovito::Point_3<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::make_tuple(src[0], src[1], src[2]).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::Point_3<T>, _("Point3<") + make_caster<T>::name() + _(">"));
    };		

	/// Automatic Python <--> Color conversion
    template<typename T> struct type_caster<Ovito::ColorT<T>> {
    public:
        bool load(handle src, bool) {
			if(!isinstance<sequence>(src)) return false;
			sequence seq = reinterpret_borrow<sequence>(src);
			if(seq.size() != value.size())
				throw value_error("Expected sequence of length 3.");
			for(size_t i = 0; i < value.size(); i++)
				value[i] = seq[i].cast<T>();
			return true;
        }

        static handle cast(const Ovito::ColorT<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::make_tuple(src[0], src[1], src[2]).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::ColorT<T>, _("Color<") + make_caster<T>::name() + _(">"));
    };	

	/// Automatic Python <--> ColorA conversion
    template<typename T> struct type_caster<Ovito::ColorAT<T>> {
    public:
        bool load(handle src, bool) {
			if(!isinstance<sequence>(src)) return false;
			sequence seq = reinterpret_borrow<sequence>(src);
			if(seq.size() != value.size())
				throw value_error("Expected sequence of length 4.");
			for(size_t i = 0; i < value.size(); i++)
				value[i] = seq[i].cast<T>();
			return true;
        }

        static handle cast(const Ovito::ColorT<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::make_tuple(src[0], src[1], src[2], src[3]).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::ColorAT<T>, _("ColorA<") + make_caster<T>::name() + _(">"));
    };

	/// Automatic Python <--> AffineTransformation conversion
    template<typename T> struct type_caster<Ovito::AffineTransformationT<T>> {
    public:
        bool load(handle src, bool) {
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
					value(i,j) = seq2[j].cast<T>();
				}
			}
			return true;
        }

        static handle cast(const Ovito::AffineTransformationT<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::array_t<T>({ src.row_count(), src.col_count() }, 
				{ sizeof(T), sizeof(typename Ovito::AffineTransformationT<T>::column_type) }, 
				src.elements()).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::AffineTransformationT<T>, _("AffineTransformation<") + make_caster<T>::name() + _(">"));
    };	

	/// Automatic Python <--> Matrix3 conversion
    template<typename T> struct type_caster<Ovito::Matrix_3<T>> {
    public:
        bool load(handle src, bool) {
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
					value(i,j) = seq2[j].cast<T>();
				}
			}
			return true;
        }

        static handle cast(const Ovito::Matrix_3<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::array_t<T>({ src.row_count(), src.col_count() }, 
				{ sizeof(T), sizeof(typename Ovito::Matrix_3<T>::column_type) }, 
				src.elements()).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::Matrix_3<T>, _("Matrix3<") + make_caster<T>::name() + _(">"));
    };

	/// Automatic Python <--> Matrix4 conversion
    template<typename T> struct type_caster<Ovito::Matrix_4<T>> {
    public:
        bool load(handle src, bool) {
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
					value(i,j) = seq2[j].cast<T>();
				}
			}
			return true;
        }

        static handle cast(const Ovito::Matrix_4<T>& src, return_value_policy /* policy */, handle /* parent */) {
			return pybind11::array_t<T>({ src.row_count(), src.col_count() }, 
				{ sizeof(T), sizeof(typename Ovito::Matrix_4<T>::column_type) }, 
				src.elements()).release();
        }

		PYBIND11_TYPE_CASTER(Ovito::Matrix_4<T>, _("Matrix4<") + make_caster<T>::name() + _(">"));
    };		

	// Automatic QSet<int> conversion.
	template<> struct type_caster<QSet<int>> : set_caster<QSet<int>, int> {};

	// Automatic QSet<QString> conversion.
	template<> struct type_caster<QSet<QString>> : set_caster<QSet<QString>, QString> {};	
	
}} // namespace pybind11::detail

namespace PyScript {

using namespace Ovito;
namespace py = pybind11;

/// Registers the initXXX() function of a plugin so that the scripting engine can discover and load all internal modules.
/// Use the OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE macro to create an instance of this structure on application startup.
///
/// This helper class makes our internal script modules available by registering their initXXX functions with the Python interpreter.
/// This is required for static builds where all Ovito plugins are linked into the main executable file.
/// On Windows this is also needed, because OVITO plugins have an .dll extension and the Python interpreter 
/// only looks for modules that have a .pyd extension.
struct OVITO_PYSCRIPT_EXPORT PythonPluginRegistration
{
#if PY_MAJOR_VERSION >= 3
	typedef PyObject* (*InitFuncPointer)();
#else
	typedef void (*InitFuncPointer)();
#endif

	/// The identifier of the plugin to register.
	std::string _moduleName;
	/// The initXXX() function to be registered with the Python interpreter.
	InitFuncPointer _initFunc;
	/// Next structure in linked list.
	PythonPluginRegistration* _next;

	PythonPluginRegistration(const char* moduleName, InitFuncPointer initFunc) : _initFunc(initFunc) {
		_next = linkedlist;
		linkedlist = this;
		_moduleName = std::string("ovito.plugins.") + moduleName;
	}

	/// Head of linked list of initXXX() functions.
	static PythonPluginRegistration* linkedlist;
};

/// This macro must be used exactly once by every plugin that contains a Python scripting interface.
#if PY_MAJOR_VERSION >= 3
	#define OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(pluginName) \
		static PyScript::PythonPluginRegistration __pyscript_unused_variable##pluginName(#pluginName, PyInit_##pluginName);
#else
	#define OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(pluginName) \
		static PyScript::PythonPluginRegistration __pyscript_unused_variable##pluginName(#pluginName, init##pluginName);
#endif

/// Defines a Python class for an abstract OvitoObject-derived C++ class.
template<class OvitoObjectClass, class BaseClass>
class ovito_abstract_class : public py::class_<OvitoObjectClass, BaseClass, OORef<OvitoObjectClass>>
{
public:
	/// Constructor.
	ovito_abstract_class(py::handle scope, const char* docstring = nullptr, const char* pythonClassName = nullptr)
		: py::class_<OvitoObjectClass, BaseClass, OORef<OvitoObjectClass>>(scope, pythonClassName ? pythonClassName : OvitoObjectClass::OOClass().className(), docstring) {}
};

/// Defines a Python class for an OvitoObject-derived C++ class.
template<class OvitoObjectClass, class BaseClass>
class ovito_class : public py::class_<OvitoObjectClass, BaseClass, OORef<OvitoObjectClass>>
{
public:

	/// Constructor.
	ovito_class(py::handle scope, const char* docstring = nullptr, const char* pythonClassName = nullptr) 
		: py::class_<OvitoObjectClass, BaseClass, OORef<OvitoObjectClass>>(scope, pythonClassName ? pythonClassName : OvitoObjectClass::OOClass().className(), docstring) {
		// Define a constructor that takes a variable number of keyword arguments, which are used to initialize
		// properties of the newly created object.
		this->def(py::init([](py::args args, py::kwargs kwargs) {
			OORef<OvitoObjectClass> instance = constructInstance();
			initializeParameters(py::cast(instance), std::move(args), std::move(kwargs));
			return instance;
		}));
	}

	/// Constructor.
	ovito_class(py::handle scope, const char* docstring, const char* pythonClassName, py::dynamic_attr dyn_attr)
		: py::class_<OvitoObjectClass, BaseClass, OORef<OvitoObjectClass>>(scope, pythonClassName ? pythonClassName : OvitoObjectClass::OOClass().className(), docstring, dyn_attr) {
		// Define a constructor that takes a variable number of keyword arguments, which are used to initialize
		// properties of the newly created object.
		this->def(py::init([](py::args args, py::kwargs kwargs) {
			OORef<OvitoObjectClass> instance = constructInstance();
			initializeParameters(py::cast(instance), std::move(args), std::move(kwargs));
			return instance;
		}));
	}

private:

	/// Constructs the object instance in place and passes the current DataSet to the C++ constructor.
	static OORef<OvitoObjectClass> constructInstance() {
		DataSet* dataset = ScriptEngine::activeDataset();
		if(!dataset) throw Exception("Invalid interpreter state. There is no active dataset.");			
		return {new OvitoObjectClass(dataset)};
	}

	/// Initalizes the properties of the new object using the values stored in a dictionary.
	static void initializeParameters(py::object pyobj, const py::args& args, const py::kwargs& kwargs) {
		if(py::len(args) > 1) {
			if(py::len(args) > 2 || !PyDict_Check(args[1].ptr()))
				throw Exception("Constructor function accepts only keyword arguments.");
		}
		// Set attributes based on keyword arguments.
		if(kwargs)
			applyParameters(pyobj, kwargs);
		// The caller may alternatively provide a dictionary with attributes.
		if(py::len(args) == 2) {
			applyParameters(pyobj, args[1].cast<py::dict>());
		}
	}

	// Sets attributes of the given object as specified in the dictionary.
	static void applyParameters(py::object& pyobj, const py::dict& params) {
		// Iterate over the keys of the dictionary and set attributes of the
		// newly created object.
		for(auto item : params) {
			// Check if the attribute exists. Otherwise raise error.
			if(!py::hasattr(pyobj, item.first)) {
				PyErr_SetObject(PyExc_AttributeError, 
					py::str("Object type {} does not have an attribute named '{}'.").format(OvitoObjectClass::OOClass().className(), item.first).ptr());
				throw py::error_already_set();
			}
			// Set attribute value.
			py::setattr(pyobj, item.first, item.second);
		}
	}
};

template<typename Vector, typename holder_type = std::unique_ptr<Vector>, typename... Args>
pybind11::class_<Vector, holder_type> bind_vector_readonly(pybind11::module &m, const char* name, Args&&... args) {
    using T = typename Vector::value_type;
    using SizeType = typename Vector::size_type;
    using DiffType = typename Vector::difference_type;
    using ItType   = typename Vector::iterator;
    using Class_ = pybind11::class_<Vector, holder_type>;

    Class_ cl(m, name, std::forward<Args>(args)...);

    // Register comparison-related operators and functions (if possible)
    pybind11::detail::vector_if_equal_operator<Vector, Class_>(cl);

    cl.def("__bool__",
        [](const Vector &v) -> bool {
            return !v.empty();
        },
        "Check whether the list is nonempty"
    );

    cl.def("__getitem__",
        [](const Vector &v, SizeType i) -> T {
            if (i >= v.size())
                throw pybind11::index_error();
            return v[i];
        }
    );

    cl.def("__len__", &Vector::size);

    cl.def("__iter__",
           [](Vector &v) {
               return pybind11::make_iterator<
                   pybind11::return_value_policy::reference_internal, ItType, ItType, T>(
                   v.begin(), v.end());
           },
           pybind11::keep_alive<0, 1>() /* Essential: keep list alive while iterator exists */
    );

    /// Slicing protocol
    cl.def("__getitem__",
        [](const Vector &v, pybind11::slice slice) -> Vector * {
            size_t start, stop, step, slicelength;

            if(!slice.compute(v.size(), &start, &stop, &step, &slicelength))
                throw pybind11::error_already_set();

            Vector *seq = new Vector();
            seq->reserve((size_t) slicelength);

            for(size_t i=0; i<slicelength; ++i) {
                seq->push_back(v[start]);
                start += step;
            }
            return seq;
        },
        pybind11::arg("s"),
        "Retrieve list elements using a slice object"
    );

    return cl;
}

namespace detail {

template<typename PythonClass, typename ListGetterFunction>
auto register_subobject_list_wrapper(PythonClass& parentClass, const char* wrapperClassName, ListGetterFunction&& listGetter)
{
	using ObjectType = typename PythonClass::type;
	struct ObjectWrapper : public std::reference_wrapper<ObjectType> {
		using std::reference_wrapper<ObjectType>::reference_wrapper;
		ObjectType& get() { return static_cast<ObjectType&>(*this); }
		const ObjectType& get() const { return static_cast<const ObjectType&>(*this); }
	};
	using VectorType = std::decay_t<std::result_of_t<ListGetterFunction(ObjectType&)>>;
	using ElementType = typename VectorType::value_type;
	using ConstIterType = typename VectorType::const_iterator;
	
	py::class_<ObjectWrapper> pyWrapperClass(parentClass, wrapperClassName);
	pyWrapperClass.def("__bool__", [listGetter](const ObjectWrapper& wrapper) {
				return !listGetter(wrapper.get()).empty();
			});
	pyWrapperClass.def("__len__", [listGetter](const ObjectWrapper& wrapper) {
				return listGetter(wrapper.get()).size();
			});
	pyWrapperClass.def("__repr__", [](py::object listObj) {
				return py::repr(py::list(listObj));
			});
	pyWrapperClass.def("__getitem__", [listGetter](const ObjectWrapper& wrapper, int index) {
				const auto& list = listGetter(wrapper.get());
				if(index < 0) index += list.size();
				if(index < 0 || index >= list.size()) throw py::index_error();
				return list[index]; 
			});
	pyWrapperClass.def("__iter__", [listGetter](const ObjectWrapper& wrapper) {
				const auto& list = listGetter(wrapper.get());
				return py::make_iterator<py::return_value_policy::reference_internal, ConstIterType, ConstIterType, ElementType>(
					list.begin(), list.end());
			}, 
			py::keep_alive<0, 1>());
	pyWrapperClass.def("__getitem__", [listGetter](const ObjectWrapper& wrapper, py::slice slice) {
				size_t start, stop, step, slicelength;
				const auto& list = listGetter(wrapper.get());

				if(!slice.compute(list.size(), &start, &stop, &step, &slicelength))
					throw py::error_already_set();

				py::list seq;
				for(size_t i = 0; i < slicelength; ++i) {
					seq.append(py::cast(list[start]));
					start += step;
				}
				return seq;
        	}, 
			py::arg("s"), "Retrieve list elements using a slice object");
	pyWrapperClass.def("index", [listGetter](const ObjectWrapper& wrapper, py::object& item) {
				const auto& list = listGetter(wrapper.get());
				auto iter = std::find(list.cbegin(), list.cend(), item.cast<ElementType>());
				if(iter == list.cend()) throw py::value_error("Item does not exist in list");
				return std::distance(list.cbegin(), iter);
			});
	pyWrapperClass.def("__contains__", [listGetter](const ObjectWrapper& wrapper, py::object& item) {
				const auto& list = listGetter(wrapper.get());
				return std::find(list.cbegin(), list.cend(), item.cast<ElementType>()) != list.cend();
			});
	pyWrapperClass.def("count", [listGetter](const ObjectWrapper& wrapper, py::object& item) {
				const auto& list = listGetter(wrapper.get());
				return std::count(list.cbegin(), list.cend(), item.cast<ElementType>());
			});
	return pyWrapperClass;
}

template<typename PythonClass, typename ListGetterFunction, typename ListInserterFunction, typename ListRemoverFunction>
auto register_mutable_subobject_list_wrapper(PythonClass& parentClass, const char* wrapperObjectName, ListGetterFunction&& listGetter, ListInserterFunction&& listInserter, ListRemoverFunction&& listRemover) 
{
	auto pyWrapperClass = register_subobject_list_wrapper(parentClass, wrapperObjectName, std::forward<ListGetterFunction>(listGetter));
	using ObjectType = typename PythonClass::type;
	using ObjectWrapper = typename decltype(pyWrapperClass)::type;
	using VectorType = std::decay_t<std::result_of_t<ListGetterFunction(ObjectType&)>>;	
	using ElementType = typename VectorType::value_type;

	pyWrapperClass.def("append", [listGetter,listInserter](ObjectWrapper& wrapper, ElementType element) {
				if(!element) throw py::value_error("Cannot insert 'None' elements into this collection.");
				auto index = listGetter(wrapper.get()).size();
				listInserter(wrapper.get(), index, element);
			});
	pyWrapperClass.def("extend", [listGetter,listInserter](ObjectWrapper& wrapper, py::sequence seq) {
				auto index = listGetter(wrapper.get()).size();
				for(size_t i = 0; i < seq.size(); i++) {
					ElementType el = seq[i].cast<ElementType>();
					if(!el) throw py::value_error("Cannot insert 'None' elements into this collection.");
					listInserter(wrapper.get(), index++, el);
				}
			});
	pyWrapperClass.def("insert", [listGetter,listInserter](ObjectWrapper& wrapper, int index, ElementType element) {
				if(!element) throw py::value_error("Cannot insert 'None' elements into this collection.");
				const auto& list = listGetter(wrapper.get());
				if(index < 0) index += list.size();
				if(index < 0 || index >= list.size()) throw py::index_error();
				listInserter(wrapper.get(), index, element);
			});
	pyWrapperClass.def("__setitem__", [listGetter,listInserter,listRemover](ObjectWrapper& wrapper, int index, ElementType element) {
				if(!element) throw py::value_error("Cannot insert 'None' elements into this collection.");
				const auto& list = listGetter(wrapper.get());
				if(index < 0) index += list.size();
				if(index < 0 || index >= list.size()) throw py::index_error();
				listRemover(wrapper.get(), index);
				listInserter(wrapper.get(), index, element);
			});
	pyWrapperClass.def("__delitem__", [listGetter,listRemover](ObjectWrapper& wrapper, int index) {
				const auto& list = listGetter(wrapper.get());
				if(index < 0) index += list.size();
				if(index < 0 || index >= list.size()) throw py::index_error();
				listRemover(wrapper.get(), index);
			});
	pyWrapperClass.def("__delitem__", [listGetter,listRemover](ObjectWrapper& wrapper, py::slice slice) {
				size_t start, stop, step, slicelength;
				const auto& list = listGetter(wrapper.get());
				
				if(!slice.compute(list.size(), &start, &stop, &step, &slicelength))
					throw py::error_already_set();

				for(size_t i = 0; i < slicelength; ++i) {
					listRemover(wrapper.get(), start);
					start += step - 1;
				}
			},
			"Delete list elements using a slice object");			
	pyWrapperClass.def("remove", [listGetter,listRemover](ObjectWrapper& wrapper, ElementType element) {
				if(!element) throw py::value_error("Cannot remove 'None' elements from this collection.");
				const auto& list = listGetter(wrapper.get());
				auto iter = std::find(list.cbegin(), list.cend(), element);
				if(iter == list.cend()) throw py::value_error("Item does not exist in list");
				listRemover(wrapper.get(), std::distance(list.cbegin(), iter));
			});
	
	return pyWrapperClass;
}

};	// End of namespace detail

template<class PythonClass, typename ListGetterFunction>
auto expose_subobject_list(PythonClass& parentClass, ListGetterFunction&& listGetter, const char* pyPropertyName, const char* wrapperObjectName, const char* docstring = nullptr) 
{
	auto pyWrapperClass = detail::register_subobject_list_wrapper(parentClass, wrapperObjectName, std::forward<ListGetterFunction>(listGetter));
	using WapperClassType = typename decltype(pyWrapperClass)::type;
	
	parentClass.def_property_readonly(pyPropertyName, py::cpp_function(
		[](typename PythonClass::type& parent) { return WapperClassType(parent); }, 
		py::keep_alive<0,1>()), docstring);

	return pyWrapperClass;
}

template<class PythonClass, typename ListGetterFunction, typename ListInserterFunction, typename ListRemoverFunction>
auto expose_mutable_subobject_list(PythonClass& parentClass, ListGetterFunction&& listGetter, ListInserterFunction&& listInserter, ListRemoverFunction&& listRemover, const char* pyPropertyName, const char* wrapperObjectName, const char* docstring = nullptr) 
{
	auto pyWrapperClass = detail::register_mutable_subobject_list_wrapper(parentClass, wrapperObjectName, 
			std::forward<ListGetterFunction>(listGetter),
			std::forward<ListInserterFunction>(listInserter),
			std::forward<ListRemoverFunction>(listRemover));
	using WapperClassType = typename decltype(pyWrapperClass)::type;
	using ObjectType = typename PythonClass::type;
	using ObjectWrapper = typename decltype(pyWrapperClass)::type;
	using VectorType = std::decay_t<std::result_of_t<ListGetterFunction(ObjectType&)>>;	
	using ElementType = typename VectorType::value_type;
		
	parentClass.def_property(pyPropertyName, py::cpp_function(
		// getter
		[](typename PythonClass::type& parent) {	
			return WapperClassType(parent);
		}, py::keep_alive<0,1>()), 
		// setter
		[listGetter,listInserter,listRemover](typename PythonClass::type& parent, py::object& obj) {
			if(!py::isinstance<py::sequence>(obj))
				throw py::value_error("Can only assign a sequence.");
			py::sequence seq(obj);
			// First, clear the existing list.
			while(listGetter(parent).size() != 0)
				listRemover(parent, listGetter(parent).size() - 1);
			// Then insert elements from assigned sequence.
			for(size_t i = 0; i < seq.size(); i++) {
				ElementType el = seq[i].cast<ElementType>();
				if(!el) throw py::value_error("Cannot insert 'None' elements into this collection.");
				listInserter(parent, listGetter(parent).size(), el);
			}
		},
		docstring);

	return pyWrapperClass;
}

template<typename ParentClass, typename VectorClass, const VectorClass& (ParentClass::*getter_func)() const>
py::cpp_function VectorGetter() 
{
	return py::cpp_function([](py::object& obj) {
		const VectorClass& v = (obj.cast<ParentClass&>().*getter_func)();
		py::array_t<typename VectorClass::value_type> array({ v.size() }, 
				{ sizeof(typename VectorClass::value_type) }, 
				v.data(), obj);
		// Mark array as read-only.
		reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
		return array;
	});
}

template<typename ParentClass, typename VectorClass, VectorClass (ParentClass::*getter_func)() const>
py::cpp_function VectorGetter() 
{
	return py::cpp_function([](py::object& obj) {
		VectorClass v = (obj.cast<ParentClass&>().*getter_func)();
		py::array_t<typename VectorClass::value_type> array({ v.size() }, 
				{ sizeof(typename VectorClass::value_type) }, 
				v.data());
		// Mark array as read-only.
		reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
		return array;
	});
}

template<typename ParentClass, typename VectorClass, void (ParentClass::*setter_func)(const VectorClass&)>
py::cpp_function VectorSetter() 
{
	return py::cpp_function([](py::object& obj, py::array_t<typename VectorClass::value_type> array) {
		if(array.ndim() != 1)
			throw py::value_error("Array must be one-dimensional.");
		const VectorClass* v = reinterpret_cast<const VectorClass*>(array.data());
		if(array.shape(0) != v->size()) {
			std::ostringstream str;
			str << "Tried to assign an array of length " << array.shape(0) << ", "
				<< "but expected an array of length " << v->size() << ".";
			throw py::value_error(str.str());
		}
		if(array.strides(0) != sizeof(typename VectorClass::value_type))
			throw py::value_error("Array stride is not compatible. Must be a compact array.");
		(obj.cast<ParentClass&>().*setter_func)(*v);
	});
}

template<typename ParentClass, typename MatrixClass, const MatrixClass& (ParentClass::*getter_func)() const>
py::cpp_function MatrixGetter() 
{
	return py::cpp_function([](py::object& obj) {
		const MatrixClass& tm = (obj.cast<ParentClass&>().*getter_func)();
		py::array_t<typename MatrixClass::element_type> array({ tm.row_count(), tm.col_count() }, 
				{ sizeof(typename MatrixClass::element_type), sizeof(typename MatrixClass::column_type) }, 
				tm.elements(), obj);
		// Mark array as read-only.
		reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
		return array;
	});
}

template<typename ParentClass, typename MatrixClass, MatrixClass (ParentClass::*getter_func)() const>
py::cpp_function MatrixGetterCopy()
{
	return py::cpp_function([](py::object& obj) {
		const MatrixClass tm = (obj.cast<ParentClass&>().*getter_func)();
		py::array_t<typename MatrixClass::element_type> array({ tm.row_count(), tm.col_count() }, 
				{ sizeof(typename MatrixClass::element_type), sizeof(typename MatrixClass::column_type) }, 
				tm.elements());
		// Mark array as read-only.
		reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
		return array;
	});
}

template<typename ParentClass, typename MatrixClass, void (ParentClass::*setter_func)(const MatrixClass&)>
py::cpp_function MatrixSetter() 
{
	return py::cpp_function([](py::object& obj, py::array_t<typename MatrixClass::element_type, py::array::f_style | py::array::forcecast> array) {
		if(array.ndim() != 2)
			throw py::value_error("Array must be two-dimensional.");
		const MatrixClass* tm = reinterpret_cast<const MatrixClass*>(array.data());
		if(array.shape(0) != tm->row_count() || array.shape(1) != tm->col_count()) {
			std::ostringstream str;
			str << "Tried to assign a " << array.shape(0) << "x" << array.shape(1) << " array, "
				<< "but expected a " << tm->row_count() << "x" << tm->col_count() << " matrix.";
			throw py::value_error(str.str());
		}
		if(array.strides(0) != sizeof(typename MatrixClass::element_type) || array.strides(1) != sizeof(typename MatrixClass::column_type))
			throw py::value_error("Array stride is not compatible. Must be a compact array.");
		(obj.cast<ParentClass&>().*setter_func)(*tm);
	});
}

template<class PythonClass, typename DelegateListGetter>
void modifier_operate_on_list(PythonClass& parentClass, DelegateListGetter&& delegatesGetter, const char* pyPropertyName, const char* docstring = nullptr)
{	
	parentClass.def_property(pyPropertyName, 
		[delegatesGetter](typename PythonClass::type& parent) { 
			const auto& list = delegatesGetter(parent);
			return std::vector<OORef<ModifierDelegate>>(std::begin(list), std::end(list));
		}, 
		[delegatesGetter](typename PythonClass::type& parent, py::object obj) { 
			const auto& list = delegatesGetter(parent);
			py::object wrapper = py::cast(std::vector<OORef<ModifierDelegate>>(std::begin(list), std::end(list)));
			wrapper.attr("assign")(std::move(obj));
		}, 
		docstring);
}

/// Helper function that generates a getter function for the 'operate_on' attribute of a DelegatingModifier subclass
OVITO_PYSCRIPT_EXPORT py::cpp_function modifierDelegateGetter();

/// Helper function that generates a setter function for the 'operate_on' attribute of a DelegatingModifier subclass.
OVITO_PYSCRIPT_EXPORT py::cpp_function modifierDelegateSetter(const OvitoClass& delegateType);

}	// End of namespace
