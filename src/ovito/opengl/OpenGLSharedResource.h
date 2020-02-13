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

/**
 * \file OpenGLSharedResource.h
 * \brief Contains the definition of the Ovito::OpenGLSharedResource class.
 */

#pragma once


#include <ovito/core/Core.h>

namespace Ovito {

class OpenGLContextInfo;
class OpenGLContextManager;

class OpenGLSharedResource
{
public:

	/// Destructor.
    ~OpenGLSharedResource();

    /// This should be called after the OpenGL resources have been allocated.
    void attachOpenGLResources();

    /// This will free the OpenGL resources. It is automatically called by the destructor.
    void destroyOpenGLResources();

protected:

    /// This method that takes care of freeing the shared OpenGL resources.
    virtual void freeOpenGLResources() = 0;

private:
    OpenGLContextInfo* _contextInfo = nullptr;
    OpenGLSharedResource* _next = nullptr;
    OpenGLSharedResource* _prev = nullptr;

    friend class OpenGLContextManager;
    friend class OpenGLContextInfo;
};

}	// End of namespace


