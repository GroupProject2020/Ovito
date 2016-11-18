/*
 *  Copyright (c) 2012-2014, Bruno Levy
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  * Neither the name of the ALICE Project-Team nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: Bruno Levy
 *
 *     Bruno.Levy@inria.fr
 *     http://www.loria.fr/~levy
 *
 *     ALICE Project
 *     LORIA, INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 */

#ifndef __GEOGRAM_BASIC_COMMON__
#define __GEOGRAM_BASIC_COMMON__

#include <geogram/api/defs.h>

// iostream should be included before anything else,
// otherwise 'cin', 'cout' and 'cerr' will be uninitialized.
#include <iostream>

/**
 * \file geogram/basic/common.h
 * \brief Common include file, providing basic definitions. Should be
 *  included before anything else by all header files in Vorpaline.
 */

/**
 * \brief Global Vorpaline namespace
 * \details This namespace contains all the Vorpaline classes and functions
 * organized in sub-namespaces.
 */
namespace GEO {

    /**
     * \brief Initialize Geogram
     * \details This function must be called one at the very beginning of a
     * program to initialize the Vorpaline library. It also installs a exit()
     * handler that calls function terminate() when the program exists
     * normally.
     */
    void GEOGRAM_API initialize();

    /**
     * \brief Cleans up Geogram
     * \details This function is called automatically when the program exists
     * normally.
     * \warning This function should \b not be called directly.
     * \see initialize()
     */
    void GEOGRAM_API terminate();
}

/**
 * \def GEO_DEBUG
 * \brief This macro is set when compiling in debug mode
 *
 * \def GEO_PARANOID
 * \brief This macro is set when compiling in debug mode
 *
 * \def GEO_OS_LINUX
 * \brief This macro is set on Linux systems (Android included).
 *
 * \def GEO_OS_UNIX
 * \brief This macro is set on Unix systems (Android included).
 *
 * \def GEO_OS_WINDOWS
 * \brief This macro is set on Windows systems.
 *
 * \def GEO_OS_APPLE
 * \brief This macro is set on Apple systems.
 *
 * \def GEO_OS_ANDROID
 * \brief This macro is set on Android systems (in addition to GEO_OS_LINUX
 * and GEO_OS_UNIX).
 *
 * \def GEO_OS_X11
 * \brief This macro is set on X11 is supported on the current system.
 *
 * \def GEO_ARCH_32
 * \brief This macro is set if the current system is a 32 bits architecture.
 *
 * \def GEO_ARCH_64
 * \brief This macro is set if the current system is a 64 bits architecture.
 *
 * \def GEO_OPENMP
 * \brief This macro is set if OpenMP is supported on the current system.
 *
 * \def GEO_COMPILER_GCC
 * \brief This macro is set if the source code is compiled with GNU's gcc.
 *
 * \def GEO_COMPILER_INTEL
 * \brief This macro is set if the source code is compiled with Intel's icc.
 *
 * \def GEO_COMPILER_MSVC
 * \brief This macro is set if the source code is compiled with Microsoft's
 * Visual C++.
 */

#ifdef NDEBUG
#undef GEO_DEBUG
#undef GEO_PARANOID
#else
#define GEO_DEBUG
#define GEO_PARANOID
#endif

// =============================== LINUX defines ===========================

#if defined(__ANDROID__)
#define GEO_OS_ANDROID
#endif

#if defined(__linux__) || defined(__CYGWIN__)

#define GEO_OS_LINUX
#define GEO_OS_UNIX

#ifndef GEO_OS_ANDROID
#define GEO_OS_X11
#endif

#if defined(_OPENMP)
#  define GEO_OPENMP
#endif

#if defined(__INTEL_COMPILER)
#  define GEO_COMPILER_INTEL
#elif defined(__clang__)
#  define GEO_COMPILER_CLANG
#elif defined(__GNUC__)
#  define GEO_COMPILER_GCC
#else
#  error "Unsupported compiler"
#endif

// The following works on GCC and ICC
#if defined(__x86_64)
#  define GEO_ARCH_64
#else
#  define GEO_ARCH_32
#endif

// =============================== WINDOWS defines =========================

#elif defined(WIN32) || defined(_WIN64)

#define GEO_OS_WINDOWS

#if defined(_OPENMP)
#  define GEO_OPENMP
#endif

#if defined(_MSC_VER)
#  define GEO_COMPILER_MSVC
#else
#  error "Unsupported compiler"
#endif

#if defined(_WIN64)
#  define GEO_ARCH_64
#else
#  define GEO_ARCH_32
#endif

// =============================== APPLE defines ===========================

#elif defined(__APPLE__)

#define GEO_OS_APPLE
#define GEO_OS_UNIX

#if defined(_OPENMP)
#  define GEO_OPENMP
#endif

#if defined(__x86_64) || defined(__ppc64__)
#  define GEO_ARCH_64
#else
#  define GEO_ARCH_32
#endif

#if defined(__clang__)
#  define GEO_COMPILER_CLANG
#endif

// =============================== Unsupported =============================
#else

#error "Unsupported operating system"

#endif

#ifdef DOXYGEN_ONLY
// Keep doxygen happy
#define GEO_OS_WINDOWS
#define GEO_OS_APPLE
#define GEO_OS_ANDROID
#define GEO_ARCH_32
#define GEO_COMPILER_INTEL
#define GEO_COMPILER_MSVC
#endif

/**
 * \def CPP_CONCAT_(A,B)
 * \brief Helper macro for CPP_CONCAT()
 */
#define CPP_CONCAT_(A, B) A ## B

/**
 * \def CPP_CONCAT(A,B)
 * \brief Creates a new symbol by concatenating its arguments
 */
#define CPP_CONCAT(A, B) CPP_CONCAT_(A, B)

#endif

