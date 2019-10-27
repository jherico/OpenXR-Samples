#include "window.hpp"

#ifdef HAVE_QT

#include <mutex>

#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugMessage>

#ifdef Q_OS_WIN
#include <QtPlatformHeaders/QWGLNativeContext>
#endif

extern int g_argc;
extern char** g_argv;
extern QOpenGLContext* qt_gl_global_share_context();
extern void qt_gl_set_global_share_context(QOpenGLContext* context);

xr_examples::Window* g_window{ nullptr };

namespace xr_examples { namespace qt {

const QSurfaceFormat& getDefaultSurfaceFormat() {
    static QSurfaceFormat format;
    static std::once_flag once;
    std::call_once(once, [] {
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        format.setOption(QSurfaceFormat::DebugContext);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setMajorVersion(4);
        format.setMinorVersion(6);
        QSurfaceFormat::setDefaultFormat(format);
    });
    return format;
}

QGuiApplication* g_app{ nullptr };

void messageHandler(QtMsgType type, const QMessageLogContext & context, const QString & message) {
    OutputDebugStringA(message.toStdString().c_str());
    OutputDebugStringA("\n");
}

void xr_examples::qt::Window::init() {
    g_app = new QGuiApplication(g_argc, g_argv);
    qInstallMessageHandler(messageHandler);
}

struct Window::Private {
    QWindow* window{ nullptr };
    QOpenGLContext* context{ nullptr };

    Private() {
        window = new QWindow();
        window->setSurfaceType(QSurface::OpenGLSurface);
        window->setFormat(getDefaultSurfaceFormat());
        context = new QOpenGLContext();
        context->setFormat(getDefaultSurfaceFormat());
    };

    ~Private() {
        context->deleteLater();
        context = nullptr;
        window->deleteLater();
        window = nullptr;
    }

    void create(const xr::Extent2Di& size) {
        window->create();
        auto shareContext = qt_gl_global_share_context();
        if (shareContext) {
            context->setShareContext(shareContext);
        }
        context->create();
        if (!shareContext) {
            qt_gl_set_global_share_context(context);
        }
        window->setWidth(size.width);
        window->setHeight(size.height);
        window->setVisible(true);
    }
};

Window::Window() {
    g_window = this;
}

Window::~Window() {
    d.reset();
}

xr::Extent2Di Window::getSize() const {
    auto size = d->window->geometry().size();
    return { size.width(), size.height() };
}

void Window::create(const xr::Extent2Di& size) {
    d = std::make_shared<Private>();
    d->create(size);
}

void Window::makeCurrent() const {
    d->context->makeCurrent(d->window);
}

void Window::doneCurrent() const {
    d->context->doneCurrent();
}

void Window::setSwapInterval(uint32_t swapInterval) const {
}

void* Window::getNativeWindowHandle() {
    return (void*)d->window->winId();
}

void* Window::getNativeContextHandle() {
    QVariant nativeHandle = d->context->nativeHandle();
    QWGLNativeContext nativeContext = nativeHandle.value<QWGLNativeContext>();
    return (void*)nativeContext.context();
}

void Window::requestClose() {
    QCoreApplication::quit();
}

void Window::swapBuffers() const {
    d->context->swapBuffers(d->window);
}

void Window::setTitle(const std::string& title) const {
    d->window->setTitle(title.c_str());
}

void Window::runWindowLoop(const std::function<void()>& handler) {
    QTimer timer;
    timer.setInterval(0);
    timer.setSingleShot(false);
    auto connection = QObject::connect(&timer, &QTimer::timeout, handler);
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&] { QObject::disconnect(connection); });
    timer.start();
    qApp->exec();
}

}}  // namespace xr_examples::qt

#endif
