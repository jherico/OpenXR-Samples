#pragma once

#if defined(HAVE_QT)

class QOpenGLFunctions_4_5_Core;
class QOpenGLContext;

extern QOpenGLContext* qt_gl_global_share_context();
extern void qt_gl_set_global_share_context(QOpenGLContext* context);

namespace xr_examples { namespace qt {

using GLF = QOpenGLFunctions_4_5_Core;
GLF& getFunctions();

}}  // namespace xr_examples::qt

#endif