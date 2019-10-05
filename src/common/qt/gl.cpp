#include "gl.hpp"

#if defined(HAVE_QT)

#include <mutex>

#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLDebugLogger>
#include <QtGui/qopenglfunctions_4_5_core.h>

using namespace xr_examples::qt;

GLF& xr_examples::qt::getFunctions() {
    static GLF glf;
    static std::once_flag once;
    std::call_once(once, [&] { glf.initializeOpenGLFunctions(); });
    return glf;
}

#endif