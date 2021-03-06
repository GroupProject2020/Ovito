////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2020 Alexander Stukowski
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
#include "OpenGLImagePrimitive.h"
#include "OpenGLSceneRenderer.h"

#include <QOpenGLPaintDevice> 

namespace Ovito {

/******************************************************************************
* Constructor.
******************************************************************************/
OpenGLImagePrimitive::OpenGLImagePrimitive(OpenGLSceneRenderer* renderer)
{
	_contextGroup = QOpenGLContextGroup::currentContextGroup();
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

    if(!renderer->glcontext()->isOpenGLES() || renderer->glformat().majorVersion() >= 3) {

        // Initialize OpenGL shader.
        _shader = renderer->loadShaderProgram("image", ":/openglrenderer/glsl/image/image.vs", ":/openglrenderer/glsl/image/image.fs");

        // Create vertex buffer.
        if(!_vertexBuffer.create(QOpenGLBuffer::StaticDraw, 4))
            renderer->throwException(QStringLiteral("Failed to create OpenGL vertex buffer."));

        // Create OpenGL texture.
        _texture.create();
    }
}

/******************************************************************************
* Returns true if the buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool OpenGLImagePrimitive::isValid(SceneRenderer* renderer)
{
	OpenGLSceneRenderer* vpRenderer = qobject_cast<OpenGLSceneRenderer*>(renderer);
	if(!vpRenderer) return false;
	return (_contextGroup == vpRenderer->glcontext()->shareGroup());
}

/******************************************************************************
* Renders the image in a rectangle given in viewport coordinates.
******************************************************************************/
void OpenGLImagePrimitive::renderViewport(SceneRenderer* renderer, const Point2& pos, const Vector2& size)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);
	GLint vc[4];
    OVITO_REPORT_OPENGL_ERRORS(vpRenderer);
	vpRenderer->glGetIntegerv(GL_VIEWPORT, vc);

	Point2 windowPos((pos.x() + 1.0f) * vc[2] / 2, (-(pos.y() + size.y()) + 1.0f) * vc[3] / 2);
	Vector2 windowSize(size.x() * vc[2] / 2, size.y() * vc[3] / 2);
	renderWindow(renderer, windowPos, windowSize);
}

/******************************************************************************
* Renders the image in a rectangle given in device pixel coordinates.
******************************************************************************/
void OpenGLImagePrimitive::renderWindow(SceneRenderer* renderer, const Point2& pos, const Vector2& size)
{
	OpenGLSceneRenderer* vpRenderer = dynamic_object_cast<OpenGLSceneRenderer>(renderer);

	if(image().isNull() || !vpRenderer || renderer->isPicking())
		return;
    OVITO_REPORT_OPENGL_ERRORS(vpRenderer);

    if(_texture.isCreated()) {
        OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
        OVITO_ASSERT(_texture.isCreated());
        OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->rebindVAO());

        // Prepare texture.
        OVITO_CHECK_OPENGL(vpRenderer, _texture.bind());

        // Enable texturing when using compatibility OpenGL. In the core profile, this is enabled by default.
        if(vpRenderer->isCoreProfile() == false && !vpRenderer->glcontext()->isOpenGLES())
            vpRenderer->glEnable(GL_TEXTURE_2D);

        if(_needTextureUpdate) {
            _needTextureUpdate = false;

            OVITO_REPORT_OPENGL_ERRORS(vpRenderer);
            OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    #ifndef Q_OS_WASM
            OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));
            OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
    #endif

            // Upload texture data.
            QImage textureImage = convertToGLFormat(image());
            OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureImage.width(), textureImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage.constBits()));
        }

        // Transform rectangle to normalized device coordinates.
        FloatType x = pos.x(), y = pos.y();
        FloatType w = size.x(), h = size.y();
        int aaLevel = vpRenderer->antialiasingLevelInternal();
        if(aaLevel > 1) {
            x = (int)(x / aaLevel) * aaLevel;
            y = (int)(y / aaLevel) * aaLevel;
            int x2 = (int)((x + w) / aaLevel) * aaLevel;
            int y2 = (int)((y + h) / aaLevel) * aaLevel;
            w = x2 - x;
            h = y2 - y;
        }
        QRectF rect2(x, y, w, h);
        GLint vc[4];
        vpRenderer->glGetIntegerv(GL_VIEWPORT, vc);
        Point_3<GLfloat>* vertices = _vertexBuffer.map();
        vertices[0] = Point_3<GLfloat>(rect2.left() / vc[2] * 2 - 1, 1 - rect2.bottom() / vc[3] * 2, 0);
        vertices[1] = Point_3<GLfloat>(rect2.right() / vc[2] * 2 - 1, 1 - rect2.bottom() / vc[3] * 2, 1);
        vertices[2] = Point_3<GLfloat>(rect2.left() / vc[2] * 2 - 1, 1 - rect2.top() / vc[3] * 2, 2);
        vertices[3] = Point_3<GLfloat>(rect2.right() / vc[2] * 2 - 1, 1 - rect2.top() / vc[3] * 2, 3);
        _vertexBuffer.unmap();

        bool wasDepthTestEnabled = vpRenderer->glIsEnabled(GL_DEPTH_TEST);
        bool wasBlendEnabled = vpRenderer->glIsEnabled(GL_BLEND);
        OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDisable(GL_DEPTH_TEST));
        OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glEnable(GL_BLEND));
        OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        if(!_shader->bind())
            renderer->throwException(QStringLiteral("Failed to bind OpenGL shader."));

        // Set up look-up table for texture coordinates.
        static const QVector2D uvcoords[] = {{0,0}, {1,0}, {0,1}, {1,1}};
        _shader->setUniformValueArray("uvcoords", uvcoords, 4);

        _vertexBuffer.bindPositions(vpRenderer, _shader);
        OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        _vertexBuffer.detachPositions(vpRenderer, _shader);

        _shader->release();

        // Restore old state.
        if(wasDepthTestEnabled) vpRenderer->glEnable(GL_DEPTH_TEST);
        if(!wasBlendEnabled) vpRenderer->glDisable(GL_BLEND);

        // Turn off texturing.
        if(vpRenderer->isCoreProfile() == false && !vpRenderer->glcontext()->isOpenGLES())
            vpRenderer->glDisable(GL_TEXTURE_2D);
    }
    else {
        // Disable depth testing and blending.
        bool wasDepthTestEnabled = vpRenderer->glIsEnabled(GL_DEPTH_TEST);
        OVITO_CHECK_OPENGL(vpRenderer, vpRenderer->glDisable(GL_DEPTH_TEST));

        // Query the viewport size in device pixels.
        GLint vc[4];
        vpRenderer->glGetIntegerv(GL_VIEWPORT, vc);

        // Use Qt's QOpenGLPaintDevice to paint the image into the framebuffer.
        QOpenGLPaintDevice paintDevice(vc[2], vc[3]);
        QPainter painter(&paintDevice);

        OVITO_CHECK_OPENGL(vpRenderer, painter.drawImage(QRectF(pos.x(), pos.y(), size.x(), size.y()), image()));

        // Restore old state.
        if(wasDepthTestEnabled)
            vpRenderer->glEnable(GL_DEPTH_TEST);
    }
    OVITO_REPORT_OPENGL_ERRORS(vpRenderer);
}

static inline QRgb qt_gl_convertToGLFormatHelper(QRgb src_pixel, GLenum texture_format)
{
    if(texture_format == 0x80E1 /*GL_BGRA*/) {
        if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            return ((src_pixel << 24) & 0xff000000)
                   | ((src_pixel >> 24) & 0x000000ff)
                   | ((src_pixel << 8) & 0x00ff0000)
                   | ((src_pixel >> 8) & 0x0000ff00);
        }
        else {
            return src_pixel;
        }
    }
    else {  // GL_RGBA
        if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
            return (src_pixel << 8) | ((src_pixel >> 24) & 0xff);
        }
        else {
            return ((src_pixel << 16) & 0xff0000)
                   | ((src_pixel >> 16) & 0xff)
                   | (src_pixel & 0xff00ff00);
        }
    }
}

static void convertToGLFormatHelper(QImage &dst, const QImage &img, GLenum texture_format)
{
    OVITO_ASSERT(dst.depth() == 32);
    OVITO_ASSERT(img.depth() == 32);

    if(dst.size() != img.size()) {
        int target_width = dst.width();
        int target_height = dst.height();
        qreal sx = target_width / qreal(img.width());
        qreal sy = target_height / qreal(img.height());

        quint32 *dest = (quint32 *) dst.scanLine(0); // NB! avoid detach here
        uchar *srcPixels = (uchar *) img.scanLine(img.height() - 1);
        int sbpl = img.bytesPerLine();
        int dbpl = dst.bytesPerLine();

        int ix = int(0x00010000 / sx);
        int iy = int(0x00010000 / sy);

        quint32 basex = int(0.5 * ix);
        quint32 srcy = int(0.5 * iy);

        // scale, swizzle and mirror in one loop
        while (target_height--) {
            const uint *src = (const quint32 *) (srcPixels - (srcy >> 16) * sbpl);
            int srcx = basex;
            for (int x=0; x<target_width; ++x) {
                dest[x] = qt_gl_convertToGLFormatHelper(src[srcx >> 16], texture_format);
                srcx += ix;
            }
            dest = (quint32 *)(((uchar *) dest) + dbpl);
            srcy += iy;
        }
    }
    else {
        const int width = img.width();
        const int height = img.height();
        const uint *p = (const uint*) img.scanLine(img.height() - 1);
        uint *q = (uint*) dst.scanLine(0);

        if(texture_format == 0x80E1 /*GL_BGRA*/) {
            if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                // mirror + swizzle
                for(int i=0; i < height; ++i) {
                    const uint *end = p + width;
                    while(p < end) {
                        *q = ((*p << 24) & 0xff000000)
                             | ((*p >> 24) & 0x000000ff)
                             | ((*p << 8) & 0x00ff0000)
                             | ((*p >> 8) & 0x0000ff00);
                        p++;
                        q++;
                    }
                    p -= 2 * width;
                }
            }
            else {
                const uint bytesPerLine = img.bytesPerLine();
                for (int i=0; i < height; ++i) {
                    memcpy(q, p, bytesPerLine);
                    q += width;
                    p -= width;
                }
            }
        }
        else {
            if(QSysInfo::ByteOrder == QSysInfo::BigEndian) {
                for(int i=0; i < height; ++i) {
                    const uint *end = p + width;
                    while(p < end) {
                        *q = (*p << 8) | ((*p >> 24) & 0xff);
                        p++;
                        q++;
                    }
                    p -= 2 * width;
                }
            }
            else {
                for(int i=0; i < height; ++i) {
                    const uint *end = p + width;
                    while(p < end) {
                        *q = ((*p << 16) & 0xff0000) | ((*p >> 16) & 0xff) | (*p & 0xff00ff00);
                        p++;
                        q++;
                    }
                    p -= 2 * width;
                }
            }
        }
    }
}

QImage OpenGLImagePrimitive::convertToGLFormat(const QImage& img)
{
    QImage res(img.size(), QImage::Format_ARGB32);
    convertToGLFormatHelper(res, img.convertToFormat(QImage::Format_ARGB32), GL_RGBA);
    return res;
}

}	// End of namespace
