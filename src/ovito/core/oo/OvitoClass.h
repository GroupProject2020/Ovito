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


#include <ovito/core/Core.h>
#include <ovito/core/oo/OORef.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Meta-class for classes derived from OvitoObject.
 */
class OVITO_CORE_EXPORT OvitoClass
{
public:

	/// Structure holding the serialized metadata for a class that was loaded from a file.
	/// It may be subclassed by metaclasses if they want to store additional information
	/// for each of their classes. This structure is used by the ObjectLoadStream class.
	struct SerializedClassInfo
	{
		/// Virtual destructor.
		virtual ~SerializedClassInfo() = default;

		/// The metaclass instance.
		OvitoClassPtr clazz;
	};

public:

	/// \brief Constructor.
	OvitoClass(const QString& name, OvitoClassPtr superClass, const char* pluginId, const QMetaObject* qtClassInfo);

	/// \brief Returns the name of the C++ class described by this meta-class instance.
	/// \return The name of the class (without namespace qualifier).
	const QString& name() const { return _name; }

	/// \brief Returns the name of the C++ class as a C string.
	/// \return A pointer to the class name string (without namespace qualifier).
	const char* className() const { return _pureClassName; }

	/// \brief Returns the human-readable display name of the class.
	/// \return The human-readable name of this object type that should be shown in the user interface.
	const QString& displayName() const { return _displayName; }

	/// Returns the name alias that has been set for this class.
	/// It will be used as an alternative name when looking up the class for a serialized object in a state file.
	/// This allows to maintain backward compatibility when renaming classes in the C++ source code.
	const QString& nameAlias() const { return _nameAlias; }

	/// \brief Returns the meta-class of the base class.
	OvitoClassPtr superClass() const { return _superClass; }

	/// \brief Returns the identifier of the plugin that defined the class.
	const char* pluginId() const { return _pluginId; }

	/// \brief Returns the plugin that defined the class.
	Plugin* plugin() const { return _plugin; }

	/// Returns the Qt runtime-type information associated with the C++ class.
	/// This may be NULL if this is not a native class type.
	const QMetaObject* qtMetaObject() const { return _qtClassInfo; }

	/// \brief Indicates whether the class is abstract (meaning that no instances can be created using createInstance()).
	bool isAbstract() const { return _isAbstract; }

	/// \brief Determines whether the class is directly or indirectly derived from some other class.
	/// \note This method also returns \c true if the class \a other is the class itself.
	bool isDerivedFrom(const OvitoClass& other) const {
		OvitoClassPtr c = this;
		do {
			if(c == &other) return true;
			c = c->superClass();
		}
		while(c);
		return false;
	}

	/// \brief Determines if an object is an instance of the class or one of its subclasses.
	bool isMember(const OvitoObject* obj) const;

	/// \brief Creates an instance of the class.
	/// \param dataset The dataset the newly created object will belong to.
	///                This may only be NULL when creating an instance of a class that is not derived from RefTarget.
	/// \return The new instance of the class. The pointer can be safely cast to the corresponding C++ class type.
	/// \throw Exception if a required plugin failed to load, or if the instantiation failed for some other reason.
	OORef<OvitoObject> createInstance(DataSet* dataset) const;

	/// Compares two types.
	bool operator==(const OvitoClass& other) const { return (this == &other); }

	/// Compares two types.
	bool operator!=(const OvitoClass& other) const { return (this != &other); }

	/// \brief Writes a type descriptor to the stream.
	/// \note This method is for internal use only.
	static void serializeRTTI(SaveStream& stream, OvitoClassPtr type);

	/// \brief Loads a type descriptor from the stream.
	/// \throw Exception if the class is not defined or the required plugin is not installed.
	/// \note This method is for internal use only.
	static OvitoClassPtr deserializeRTTI(LoadStream& stream);

	/// \brief Encodes the plugin ID and the class name as a string.
	static QString encodeAsString(OvitoClassPtr type);

	/// \brief Decodes a class descriptor from a string, which has been generated by encodeAsString().
	/// \throw Exception if the class is invalid or the plugin is no longer available.
	static OvitoClassPtr decodeFromString(const QString& str);

	/// \brief This method is called by the ObjectSaveStream class when saving one or more object instances of
	///        a class belonging to this metaclass. May be overriden by sub-metaclasses if they want to store
	///        additional meta information for the class in the output stream.
	virtual void saveClassInfo(SaveStream& stream) const {}

	/// \brief This method is called by the ObjectLoadStream class when loading one or more object instances
	///        of a class belonging to this metaclass. May be overriden by sub-metaclasses if they want to restore
	///        additional meta information for the class from the input stream.
	virtual void loadClassInfo(LoadStream& stream, SerializedClassInfo* classInfo) const {}

	/// \brief Creates a new instance of the SerializedClassInfo structure.
	virtual std::unique_ptr<SerializedClassInfo> createClassInfoStructure() const {
		return std::make_unique<SerializedClassInfo>();
	}

protected:

	/// \brief Is called by the system after construction of the meta-class instance.
	virtual void initialize();

	/// \brief Creates an instance of the class described by this meta-class.
	/// \param dataset The dataset the newly created object will belong to.
	/// \return The new instance of the class. The pointer can be safely cast to the corresponding C++ class type.
	/// \throw Exception if the instance could not be created.
	virtual OvitoObject* createInstanceImpl(DataSet* dataset) const;

	/// \brief Marks this class as an abstract class that cannot be instantiated.
	void setAbstract(bool abstract) { _isAbstract = abstract; }

	/// \brief Changes the the human-readable display name of this plugin class.
	void setDisplayName(const QString& name) { _displayName = name; }

	/// Sets a name alias for the class.
	/// It will be used as an alternative name when looking up the class for a serialized object in a state file.
	/// This allows to maintain backward compatibility when renaming classes in the C++ source code.
	void setNameAlias(const QString& alias) { _nameAlias = alias; }

protected:

	/// The class name.
	QString _name;

	/// The human-readable display name of this plugin class.
	QString _displayName;

	/// The identifier of the plugin that defined the class.
	const char* _pluginId = nullptr;

	/// The plugin that defined the class.
	Plugin*	_plugin;

	/// An alias for the class name, which is used when looking up a class for a serialized object.
	/// This can help to maintain backward file compatibility when renaming classes.
	QString _nameAlias;

	/// The base class descriptor (or NULL if this is the descriptor for the root OvitoObject class).
	OvitoClassPtr _superClass;

	/// Indicates whether the class is abstract.
	bool _isAbstract;

	/// The runtime-type information provided by Qt.
	const QMetaObject* _qtClassInfo;

	/// The name of the C++ class.
	const char* _pureClassName = nullptr;

	/// All meta-classes form a linked list.
	OvitoClass* _nextMetaclass;

	/// The head of the linked list with all meta-classes.
	static OvitoClass* _firstMetaClass;

	friend class PluginManager;
};

/// This macro must be included in the class definition of any OvitoObject-derived class.
#define OVITO_CLASS_INTERNAL(classname, baseclassname) \
public: \
	using ovito_parent_class = baseclassname; \
	using ovito_class = classname; \
	static const OOMetaClass& OOClass() { return __OOClass_instance; } \
	virtual const Ovito::OvitoClass& getOOClass() const override { return OOClass(); } \
	const OOMetaClass& getOOMetaClass() const { return static_cast<const OOMetaClass&>(getOOClass()); } \
private: \
	static const OOMetaClass __OOClass_instance;

/// This macro must be included in the class definition of any OvitoObject-derived class.
#define OVITO_CLASS(classname) \
	OVITO_CLASS_INTERNAL(classname, ovito_class)

/// This macro must be included in the class definition of a class template that inherits from a OvitoObject class.
#define OVITO_CLASS_TEMPLATE(clazz, baseclazz) \
public: \
	using OOMetaClass = typename baseclazz::OOMetaClass; \
	using ovito_parent_class = baseclazz; \
	using ovito_class = clazz; \
	static const OOMetaClass& OOClass() { return __OOClass_instance; } \
	virtual const Ovito::OvitoClass& getOOClass() const override { return OOClass(); } \
	const OOMetaClass& getOOMetaClass() const { return static_cast<const OOMetaClass&>(getOOClass()); } \
private: \
	static const OOMetaClass __OOClass_instance;

/// This macro is used instead of the default one above when the class should get its own metaclass type.
#define OVITO_CLASS_META(classname, metaclassname) \
	public: \
		using OOMetaClass = metaclassname; \
	OVITO_CLASS(classname)

/// This macro must be included in the .cpp file for any OvitoObject-derived class.
#define IMPLEMENT_OVITO_CLASS(classname) \
	const classname::OOMetaClass classname::__OOClass_instance(\
		QStringLiteral(#classname), \
		&classname::ovito_parent_class::OOClass(), \
		OVITO_PLUGIN_NAME, \
		&classname::staticMetaObject);

/// This macro must be included in the .cpp file for any OvitoObject-derived class template.
#define IMPLEMENT_OVITO_CLASS_TEMPLATE(classname) \
	template<> const classname::OOMetaClass classname::__OOClass_instance(\
		QStringLiteral(#classname), \
		&classname::ovito_parent_class::OOClass(), \
		OVITO_PLUGIN_NAME, \
		nullptr);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::OvitoClassPtr);
Q_DECLARE_TYPEINFO(Ovito::OvitoClassPtr, Q_PRIMITIVE_TYPE);
