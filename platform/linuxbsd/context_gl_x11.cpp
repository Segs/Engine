/*************************************************************************/
/*  context_gl_x11.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "context_gl_x11.h"
#include "core/string_utils.h"

#ifdef X11_ENABLED
#if defined(OPENGL_ENABLED)
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

using GLXCREATECONTEXTATTRIBSARBPROC = GLXContext (*)(Display *, GLXFBConfig, GLXContext, int, const int *);

struct ContextGL_X11_Private {

    ::GLXContext glx_context;
    ::GLXContext glx_context_offscreen;
};

void ContextGL_X11::release_current() {

    glXMakeCurrent(x11_display, None, nullptr);
}

void ContextGL_X11::make_current() {

    glXMakeCurrent(x11_display, x11_window, p->glx_context);
}

bool ContextGL_X11::is_offscreen_available() const {
    return p->glx_context_offscreen;
}

void ContextGL_X11::make_offscreen_current() {
    glXMakeCurrent(x11_display, x11_window, p->glx_context_offscreen);
}

void ContextGL_X11::release_offscreen_current() {
    glXMakeCurrent(x11_display, None, NULL);
}
void ContextGL_X11::swap_buffers() {

    glXSwapBuffers(x11_display, x11_window);
}

static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
    ctxErrorOccurred = true;
    return 0;
}

static void set_class_hint(Display *p_display, Window p_window) {
    XClassHint *classHint;

    /* set the name and class hints for the window manager to use */
    classHint = XAllocClassHint();
    if (classHint) {
        classHint->res_name = (char *)"Godot_Engine";
        classHint->res_class = (char *)"Godot";
    }
    XSetClassHint(p_display, p_window, classHint);
    XFree(classHint);
}

Error ContextGL_X11::initialize() {

    //const char *extensions = glXQueryExtensionsString(x11_display, DefaultScreen(x11_display));

    GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte *)"glXCreateContextAttribsARB");

    ERR_FAIL_COND_V(!glXCreateContextAttribsARB, ERR_UNCONFIGURED);

    static int visual_attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DEPTH_SIZE, 24,
        None
    };

    static int visual_attribs_layered[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, true,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        None
    };

    int fbcount;
    GLXFBConfig fbconfig = nullptr;
    XVisualInfo *vi = nullptr;

    XSetWindowAttributes swa;
    swa.event_mask = StructureNotifyMask;
    swa.border_pixel = 0;
    unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask;

    if (OS::get_singleton()->is_layered_allowed()) {
        GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs_layered, &fbcount);
        ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);

        for (int i = 0; i < fbcount; i++) {
            vi = (XVisualInfo *)glXGetVisualFromFBConfig(x11_display, fbc[i]);
            if (!vi)
                continue;

            XRenderPictFormat *pict_format = XRenderFindVisualFormat(x11_display, vi->visual);
            if (!pict_format) {
                XFree(vi);
                vi = nullptr;
                continue;
            }

            fbconfig = fbc[i];
            if (pict_format->direct.alphaMask > 0) {
                break;
            }
        }
        XFree(fbc);
        ERR_FAIL_COND_V(!fbconfig, ERR_UNCONFIGURED);

        swa.background_pixmap = None;
        swa.background_pixel = 0;
        swa.border_pixmap = None;
        valuemask |= CWBackPixel;

    } else {
        GLXFBConfig *fbc = glXChooseFBConfig(x11_display, DefaultScreen(x11_display), visual_attribs, &fbcount);
        ERR_FAIL_COND_V(!fbc, ERR_UNCONFIGURED);

        vi = glXGetVisualFromFBConfig(x11_display, fbc[0]);

        fbconfig = fbc[0];
        XFree(fbc);
    }

    int (*oldHandler)(Display *, XErrorEvent *) = XSetErrorHandler(&ctxErrorHandler);


            static int context_attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
                GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#ifdef DEBUG
                | GLX_CONTEXT_DEBUG_BIT_ARB
#endif
                ,
                None
            };

            p->glx_context = glXCreateContextAttribsARB(x11_display, fbconfig, nullptr, true, context_attribs);
            ERR_FAIL_COND_V(ctxErrorOccurred || !p->glx_context, ERR_UNCONFIGURED);
    p->glx_context_offscreen = glXCreateContextAttribsARB(x11_display, fbconfig, nullptr, true, context_attribs);

    swa.colormap = XCreateColormap(x11_display, RootWindow(x11_display, vi->screen), vi->visual, AllocNone);
    x11_window = XCreateWindow(x11_display, RootWindow(x11_display, vi->screen), 0, 0, OS::get_singleton()->get_video_mode().width, OS::get_singleton()->get_video_mode().height, 0, vi->depth, InputOutput, vi->visual, valuemask, &swa);
    XStoreName(x11_display, x11_window, "Godot Engine");

    ERR_FAIL_COND_V(!x11_window, ERR_UNCONFIGURED);
    set_class_hint(x11_display, x11_window);

    if (!OS::get_singleton()->is_no_window_mode_enabled()) {
        XMapWindow(x11_display, x11_window);
    }

    XSync(x11_display, False);
    XSetErrorHandler(oldHandler);

    glXMakeCurrent(x11_display, x11_window, p->glx_context);

    XFree(vi);

    return OK;
}

int ContextGL_X11::get_window_width() {

    XWindowAttributes xwa;
    XGetWindowAttributes(x11_display, x11_window, &xwa);

    return xwa.width;
}

int ContextGL_X11::get_window_height() {
    XWindowAttributes xwa;
    XGetWindowAttributes(x11_display, x11_window, &xwa);

    return xwa.height;
}

void *ContextGL_X11::get_glx_context() {
    if (!p) {
        return nullptr;
    }
    return p->glx_context;
}

void ContextGL_X11::set_use_vsync(bool p_use) {
    static bool setup = false;
    static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = nullptr;
    static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalMESA = nullptr;
    static PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = nullptr;
    using namespace StringUtils;
    if (!setup) {
        setup = true;
        String extensions(glXQueryExtensionsString(x11_display, DefaultScreen(x11_display)));
        if (contains(extensions,"GLX_EXT_swap_control"))
            glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
        if (contains(extensions,"GLX_MESA_swap_control"))
            glXSwapIntervalMESA = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalMESA");
        if (contains(extensions,"GLX_SGI_swap_control"))
            glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalSGI");
    }
    int val = p_use ? 1 : 0;
    if (glXSwapIntervalMESA) {
        glXSwapIntervalMESA(val);
    } else if (glXSwapIntervalSGI) {
        glXSwapIntervalSGI(val);
    } else if (glXSwapIntervalEXT) {
        GLXDrawable drawable = glXGetCurrentDrawable();
        glXSwapIntervalEXT(x11_display, drawable, val);
    } else {
        return;
    }
    use_vsync = p_use;
}
bool ContextGL_X11::is_using_vsync() const {

    return use_vsync;
}

ContextGL_X11::ContextGL_X11(::Display *p_x11_display, ::Window &p_x11_window, const OS::VideoMode &p_default_video_mode) :
        x11_window(p_x11_window) {

    default_video_mode = p_default_video_mode;
    x11_display = p_x11_display;


    double_buffer = false;
    direct_render = false;
    glx_minor = glx_major = 0;
    p = memnew(ContextGL_X11_Private);
    p->glx_context = nullptr;
    p->glx_context_offscreen = nullptr;
    use_vsync = false;
}

ContextGL_X11::~ContextGL_X11() {
    release_current();
    glXDestroyContext(x11_display, p->glx_context);
    if (p->glx_context_offscreen) {
        glXDestroyContext(x11_display, p->glx_context_offscreen);
    }
    memdelete(p);
}

#endif
#endif
