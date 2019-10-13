////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Alexander Stukowski
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
#include "OpenGLSharedResource.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A wrapper class for OpenGL textures.
 */
class OpenGLTexture : private OpenGLSharedResource
{
public:

	/// Constructor.
	OpenGLTexture() : _id(0) {}

	/// Destructor.
	~OpenGLTexture() { destroyOpenGLResources(); }

	/// Create the texture object.
	void create() {
		if(_id != 0) return;

		QOpenGLContext::currentContext()->functions()->glActiveTexture(GL_TEXTURE0);

		// Create OpenGL texture.
		QOpenGLContext::currentContext()->functions()->glGenTextures(1, &_id);

		// Make sure texture gets deleted when this object is destroyed.
		attachOpenGLResources();
	}

	/// Returns true if the texture has been created; false otherwise.
	bool isCreated() const { return _id != 0; }

	/// Makes this the active texture.
	void bind() {
		QOpenGLContext::currentContext()->functions()->glActiveTexture(GL_TEXTURE0);
		QOpenGLContext::currentContext()->functions()->glBindTexture(GL_TEXTURE_2D, _id);
	}

protected:

    /// This method that takes care of freeing the shared OpenGL resources owned by this class.
    virtual void freeOpenGLResources() override {
    	if(_id) {
    		QOpenGLContext::currentContext()->functions()->glDeleteTextures(1, &_id);
			_id = 0;
    	}
    }

private:

	/// Resource identifier of the OpenGL texture.
	GLuint _id;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


