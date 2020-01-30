////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2018 Alexander Stukowski
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

//
// Standard precompiled header file included by all source files in this module
//

#pragma once

/******************************************************************************
* Standard Template Library (STL)
******************************************************************************/
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <stack>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <utility>
#include <random>
#include <memory>
#include <mutex>
#include <thread>
#include <clocale>
#include <atomic>
#include <tuple>
#include <numeric>
#include <functional>

/******************************************************************************
* QT Library
******************************************************************************/
#include <QCoreApplication>
#include <QStringList>
#include <QSettings>
#include <QUrl>
#include <QPointer>
#include <QFileInfo>
#include <QResource>
#include <QDir>
#include <QtDebug>
#include <QtGlobal>
#include <QMetaClassInfo>
#include <QColor>
#include <QGenericMatrix>
#include <QMatrix4x4>
#include <QDateTime>
#include <QThread>
#include <QMutex>
#include <QRunnable>
#include <QImage>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QTimer>
#include <QCache>
#include <QMutex>
#include <QTemporaryFile>
#include <QElapsedTimer>
#include <QtMath>
#include <QBuffer>
#ifndef Q_OS_WASM
#include <QException>
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
#  error "OVITO requires Qt 5.6 or newer."
#endif

/******************************************************************************
* Boost Library
******************************************************************************/
#include <boost/dynamic_bitset.hpp>
#include <boost/optional/optional.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/cxx11/none_of.hpp>
#include <boost/algorithm/cxx11/one_of.hpp>
#include <boost/algorithm/cxx11/iota.hpp>

/*! \namespace Ovito
    \brief The root namespace of OVITO.
*/
/*! \namespace Ovito::Util
    \brief This namespace contains general utility classes and typedefs used throughout OVITO's codebase.
*/
/*! \namespace Ovito::IO
    \brief This namespace contains I/O-related utility classes.
*/
/*! \namespace Ovito::Concurrency
    \brief This namespace contains class related to multi-threading, parallelism, and asynchronous tasks.
*/
/*! \namespace Ovito::Mesh
    \brief This namespace contains classes for working with triangular and polyhedral meshes.
*/
/*! \namespace Ovito::Math
    \brief This namespace contains classes related to linear algebra and geometry (vectors, transformation matrices, etc).
*/
/*! \namespace Ovito::Rendering
    \brief This namespace contains classes related to scene rendering.
*/
/*! \namespace Ovito::View
    \brief This namespace contains classes related to 3d viewports.
*/
/*! \namespace Ovito::DataIO
    \brief This namespace contains the framework for data import and export.
*/
/*! \namespace Ovito::Anim
    \brief This namespace contains classes related to object and parameter animation.
*/
/*! \namespace Ovito::PluginSystem
    \brief This namespace contains classes related to OVITO's plugin-based extension system.
*/
/*! \namespace Ovito::ObjectSystem
    \brief This namespace contains basic classes of OVITO's object system.
*/
/*! \namespace Ovito::Units
    \brief This namespace contains classes related to parameter units.
*/
/*! \namespace Ovito::Undo
    \brief This namespace contains the implementation of OVITO's undo framework.
*/
/*! \namespace Ovito::Scene
    \brief This namespace contains the scene graph and modification pipeline framework.
*/

/******************************************************************************
* Forward declaration of classes.
******************************************************************************/
#include "ForwardDecl.h"

/******************************************************************************
* Our own basic headers
******************************************************************************/
#include <ovito/core/utilities/Debugging.h>
#include <ovito/core/utilities/FloatType.h>
#include <ovito/core/utilities/Exception.h>
#include <ovito/core/utilities/linalg/LinAlg.h>
#include <ovito/core/utilities/Color.h>
#include <ovito/core/oo/OvitoObject.h>
