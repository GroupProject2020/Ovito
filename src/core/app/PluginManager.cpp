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

#include <core/Core.h>
#include "PluginManager.h"

#include <QLibrary>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/// The singleton instance of this class.
PluginManager* PluginManager::_instance = nullptr;

/******************************************************************************
* Initializes the plugin manager.
******************************************************************************/
PluginManager::PluginManager()
{
	OVITO_ASSERT_MSG(!_instance, "PluginManager constructor", "Multiple instances of this singleton class have been created.");
}

/******************************************************************************
* Unloads all plugins.
******************************************************************************/
PluginManager::~PluginManager()
{
	// Unload plugins in reverse order.
	for(int i = plugins().size() - 1; i >= 0; --i) {
		delete plugins()[i];
	}
}

/******************************************************************************
* Returns the plugin with the given identifier.
* Returns NULL when no such plugin is installed.
******************************************************************************/
Plugin* PluginManager::plugin(const QString& pluginId)
{
	for(Plugin* plugin : plugins()) {
		if(plugin->pluginId() == pluginId)
			return plugin;
	}
	return nullptr;
}

/******************************************************************************
* Registers a new plugin with the manager.
******************************************************************************/
void PluginManager::registerPlugin(Plugin* plugin)
{
	OVITO_CHECK_POINTER(plugin);

	// Make sure the plugin's ID is unique.
	if(this->plugin(plugin->pluginId())) {
		QString id = plugin->pluginId();
		delete plugin;
		throw Exception(tr("Non-unique plugin identifier detected: %1").arg(id));
	}

	_plugins.push_back(plugin);
}

/******************************************************************************
* Returns the list of directories containing the Ovito plugins.
******************************************************************************/
QList<QDir> PluginManager::pluginDirs()
{
	QDir prefixDir(QCoreApplication::applicationDirPath());
#if defined(Q_OS_WIN)
	return { QDir(prefixDir.absolutePath() + QStringLiteral("/plugins")) };
#elif defined(Q_OS_MAC)
	prefixDir.cdUp();
	return { QDir(prefixDir.absolutePath() + QStringLiteral("/PlugIns")) };
#else
	prefixDir.cdUp();
	return { QDir(prefixDir.absolutePath() + QStringLiteral("/lib/ovito/plugins")) };
#endif
}

/******************************************************************************
* Searches the plugin directories for installed plugins and loads them.
******************************************************************************/
void PluginManager::loadAllPlugins()
{
// Only load plugin dynamic libraries if they are not already linked into the executable.
#ifndef OVITO_BUILD_MONOLITHIC

#ifdef Q_OS_WIN
	// Modify PATH enviroment variable so that Windows finds the plugin DLLs if 
	// there are dependencies between them.
	QByteArray path = qgetenv("PATH");
	for(QDir pluginDir : pluginDirs()) {
		path = QDir::toNativeSeparators(pluginDir.absolutePath()).toUtf8() + ";" + path;
	}
	qputenv("PATH", path);
#endif
	
	// Scan the plugin directories for installed plugins.
	// This only done in standalone mode.
	// When OVITO is being used from an external Python interpreter, 
	// then plugins are loaded via explicit import statements.
	for(QDir pluginDir : pluginDirs()) {
		if(!pluginDir.exists())
			throw Exception(tr("Failed to scan the plugin directory. Path %1 does not exist.").arg(pluginDir.path()));

		// List all plugin files.
		pluginDir.setNameFilters(QStringList() << "*.so" << "*.dll");
		pluginDir.setFilter(QDir::Files);
		for(const QString& file : pluginDir.entryList()) {
			QString filePath = pluginDir.absoluteFilePath(file);	
			QLibrary* library = new QLibrary(filePath, this);
			library->setLoadHints(QLibrary::ExportExternalSymbolsHint);
			if(!library->load()) {
				Exception ex(QString("Failed to load native plugin library.\nLibrary file: %1\nError: %2").arg(filePath, library->errorString()));
				ex.reportError(true);
			}
		}
	}
#endif

	registerLoadedPluginClasses();
}

/******************************************************************************
* Registers all classes of all plugins already loaded so far.
******************************************************************************/
void PluginManager::registerLoadedPluginClasses()
{
	for(OvitoClass* clazz = OvitoClass::_firstMetaClass; clazz != _lastRegisteredClass; clazz = clazz->_nextMetaclass) {
		Plugin* classPlugin = this->plugin(clazz->pluginId());
		if(!classPlugin) {
			classPlugin = new Plugin(clazz->pluginId());
			registerPlugin(classPlugin);
		}
		OVITO_ASSERT(clazz->plugin() == nullptr);
		clazz->_plugin = classPlugin;
		clazz->initialize();
		classPlugin->registerClass(clazz);
	}
	_lastRegisteredClass = OvitoClass::_firstMetaClass;
}

/******************************************************************************
* Returns the metaclass with the given name defined by the given plugin.
******************************************************************************/
OvitoClassPtr PluginManager::findClass(const QString& pluginId, const QString& className)
{
	if(Plugin* p = plugin(pluginId)) {
		return p->findClass(className);
	}
	return nullptr;
}

/******************************************************************************
* Returns all installed plugin classes derived from the given type.
******************************************************************************/
QVector<OvitoClassPtr> PluginManager::listClasses(const OvitoClass& superClass, bool skipAbstract)
{
	QVector<OvitoClassPtr> result;

	for(Plugin* plugin : plugins()) {
		for(OvitoClassPtr clazz : plugin->classes()) {
			if(!skipAbstract || !clazz->isAbstract()) {
				if(clazz->isDerivedFrom(superClass))
					result.push_back(clazz);
			}
		}
	}

	return result;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
