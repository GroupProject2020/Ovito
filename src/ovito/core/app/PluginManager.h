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

#pragma once


#include <ovito/core/Core.h>
#include <ovito/core/oo/OvitoClass.h>

namespace Ovito {

/**
 * \brief Represents a plugin that is loaded at runtime.
 */
class OVITO_CORE_EXPORT Plugin : public QObject
{
	Q_OBJECT

public:

	/// \brief Returns the unique identifier of the plugin.
	const QString& pluginId() const { return _pluginId; }

	/// \brief Finds the plugin class with the given name defined by the plugin.
	/// \param name The class name.
	/// \return The descriptor for the plugin class with the given name or \c NULL
	///         if no such class is defined by the plugin.
	/// \sa classes()
	OvitoClassPtr findClass(const QString& name) const {
		for(OvitoClassPtr type : classes()) {
			if(type->name() == name || type->nameAlias() == name)
				return type;
		}
		return nullptr;
	}

	/// \brief Returns whether the plugin's dynamic library has been loaded.
	/// \sa loadPlugin()
	bool isLoaded() const { return true; }

	/// \brief Loads the plugin's dynamic link library into memory.
	/// \throw Exception if an error occurs.
	///
	/// This method may load other plugins first if this plugin
	/// depends on them.
	/// \sa isLoaded()
	void loadPlugin() {}

	/// \brief Returns all classes defined by the plugin.
	/// \sa findClass()
	const QVector<OvitoClass*>& classes() const { return _classes; }

protected:

	/// \brief Constructor.
	Plugin(const QString& pluginId) : _pluginId(pluginId) {}

	/// \brief Adds a class to the list of plugin classes.
	void registerClass(OvitoClass* clazz) { _classes.push_back(clazz); }

private:

	/// The unique identifier of the plugin.
	QString _pluginId;

	/// The classes provided by the plugin.
	QVector<OvitoClass*> _classes;

	friend class PluginManager;
};

/**
 * \brief Loads and manages the installed plugins.
 */
class OVITO_CORE_EXPORT PluginManager : public QObject
{
	Q_OBJECT

public:

	/// Create the singleton instance of this class.
	static void initialize() {
		_instance = new PluginManager();
		_instance->registerLoadedPluginClasses();
	}

	/// Deletes the singleton instance of this class.
	static void shutdown() { delete _instance; _instance = nullptr; }

	/// \brief Returns the one and only instance of this class.
	/// \return The predefined instance of the PluginManager singleton class.
	inline static PluginManager& instance() {
		OVITO_ASSERT_MSG(_instance != nullptr, "PluginManager::instance", "Singleton object is not initialized yet.");
		return *_instance;
	}

	/// Searches the plugin directories for installed plugins and loads them.
	void loadAllPlugins();

	/// \brief Returns the plugin with a given identifier.
	/// \param pluginId The identifier of the plugin to return.
	/// \return The plugin with the given identifier or \c NULL if no such plugin is installed.
	Plugin* plugin(const QString& pluginId);

	/// \brief Returns the list of installed plugins.
	/// \return The list of all installed plugins.
	const QVector<Plugin*>& plugins() const { return _plugins; }

	/// \brief Returns all installed plugin classes derived from the given type.
	/// \param superClass Specifies the base class from which all returned classes should be derived.
	/// \param skipAbstract If \c true only non-abstract classes are returned.
	/// \return A list that contains all requested classes.
	QVector<OvitoClassPtr> listClasses(const OvitoClass& superClass, bool skipAbstract = true);

	/// \brief Returns the metaclass with the given name defined by the given plugin.
	OvitoClassPtr findClass(const QString& pluginId, const QString& className);

	/// Returns a list with all classes that belong to a metaclass.
	template<class C>
	QVector<const typename C::OOMetaClass*> metaclassMembers(const OvitoClass& parentClass = C::OOClass(), bool skipAbstract = true) {
		OVITO_ASSERT(parentClass.isDerivedFrom(C::OOClass()));
		QVector<const typename C::OOMetaClass*> result;
		for(Plugin* plugin : plugins()) {
			for(OvitoClassPtr clazz : plugin->classes()) {
				if(!skipAbstract || !clazz->isAbstract()) {
					if(clazz->isDerivedFrom(parentClass))
						result.push_back(static_cast<const typename C::OOMetaClass*>(clazz));
				}
			}
		}
		return result;
	}

	/// \brief Registers a new plugin with the manager.
	/// \param plugin The plugin to be registered.
	/// \throw Exception when the plugin ID is not unique.
	/// \note The PluginManager becomes the owner of the Plugin class instance and will
	///       delete it on application shutdown.
	void registerPlugin(Plugin* plugin);

	/// \brief Registers all classes of all plugins already loaded so far.
	void registerLoadedPluginClasses();

	/// \brief Returns the list of directories containing the Ovito plugins.
	QList<QDir> pluginDirs();

	/// \brief Destructor that unloads all plugins.
	~PluginManager();

private:

	/////////////////////////////////// Plugins ////////////////////////////////////

	/// The list of installed plugins.
	QVector<Plugin*> _plugins;

	/////////////////////////// Maintenance ////////////////////////////////

	/// Private constructor.
	/// This is a singleton class; no public instances are allowed.
	PluginManager();

	/// The position in the global linked list of native object types up to which classes have already been registered.
	OvitoClass* _lastRegisteredClass = nullptr;

	/// The singleton instance of this class.
	static PluginManager* _instance;

	friend class Plugin;
};

}	// End of namespace


