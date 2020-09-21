#include <QGuiApplication>
#include <QCommandLineParser>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>
#include <QtQml/qqml.h>
#include <QWindow>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <qpa/qplatformnativeinterface.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <wayland-client.h>
#include <time.h>

#include "wayland-agl-shell-client-protocol.h"
#include "shell.h"

// QGuiApplication::platformNativeInterface

static void global_add(void *data, struct wl_registry *reg, uint32_t name,
		       const char *interface, uint32_t)
{
	struct agl_shell **shell = static_cast<struct agl_shell **>(data);

	if (strcmp(interface, agl_shell_interface.name) == 0) {
		*shell = static_cast<struct agl_shell *>(
			wl_registry_bind(reg, name, &agl_shell_interface, 1)
		);
	}
}

static void global_remove(void *data, struct wl_registry *reg, uint32_t id)
{
	// Don't care
	(void) data;
	(void) reg;
	(void) id;
}

static const struct wl_registry_listener registry_listener = {
	global_add,
	global_remove,
};

wl_surface *
getWlSurface(QWindow *window)
{
	QPlatformNativeInterface *native = qApp->platformNativeInterface();
	
	//void *surf = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", window);
	void *surf = native->nativeResourceForWindow("surface", window);
	return static_cast<struct ::wl_surface *>(surf);
}

struct wl_display *
getWlDisplay(void)
{
	QPlatformNativeInterface *native = qApp->platformNativeInterface();

	//void *display = QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("display");
	void *display = native->nativeResourceForIntegration("display");
	return static_cast<struct ::wl_display *>(display);
}

struct wl_compositor *
getWlCompositor(QWindow *window)
{
	QPlatformNativeInterface *native = qApp->platformNativeInterface();

	//void *compositor = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("compositor", window);
	void *compositor = native->nativeResourceForWindow("compositor", window);
	return static_cast<struct ::wl_compositor*>(compositor);
}

struct wl_output *
getWlOutput(QWindow *window)
{
	QPlatformNativeInterface *native = qApp->platformNativeInterface();

	//void *output = QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", window->screen());
	void *output = native->nativeResourceForScreen("output", window->screen());
	return static_cast<struct ::wl_output*>(output);
}

struct wl_output *
getWlOutput(QScreen *screen)
{
	QPlatformNativeInterface *native = qApp->platformNativeInterface();

	//void *output = QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", window->screen());
	void *output = native->nativeResourceForScreen("output", screen);
	return static_cast<struct ::wl_output*>(output);
}

static struct wl_surface *
create_component(QQmlComponent *comp, QScreen *screen)
{
	QObject *obj = comp->create();
	obj->setParent(screen);

	QWindow *win = qobject_cast<QWindow *>(obj);
	return getWlSurface(win);
}


int main(int argc, char *argv[])
{
	setenv("QT_QPA_PLATFORM", "wayland", 1);
	QGuiApplication app(argc, argv);
	app.setDesktopFileName("homescreen-demo-ci");

	QPlatformNativeInterface *native = qApp->platformNativeInterface();
	struct agl_shell *agl_shell = nullptr;

	struct wl_display *wl;
	struct wl_registry *registry;

	char *xdgdir = getenv("XDG_RUNTIME_DIR");
	if (xdgdir) {
		qInfo() << "xdg runtime dir " << xdgdir;
	}

	wl = static_cast<struct wl_display *>(native->nativeResourceForIntegration("display"));
	registry = wl_display_get_registry(wl);

	wl_registry_add_listener(registry, &registry_listener, &agl_shell);
	// Roundtrip to get all globals advertised by the compositor
	wl_display_roundtrip(wl);
	wl_registry_destroy(registry);

	// check if binding to agl shell worked
	std::shared_ptr<struct agl_shell> shell{agl_shell, agl_shell_destroy};

	QQmlApplicationEngine engine;
	QQmlContext *context = engine.rootContext();

	context->setContextProperty("shell", new Shell(shell, &app));

	QQmlComponent bg_comp(&engine, QUrl("qrc:/bg.qml"));
	qInfo() << bg_comp.errors();

	QQmlComponent top_comp(&engine, QUrl("qrc:/top.qml"));
	qInfo() << top_comp.errors();

	QQmlComponent bot_comp(&engine, QUrl("qrc:/bottom.qml"));
	qInfo() << bot_comp.errors();

	QQmlComponent left_comp(&engine, QUrl("qrc:/left.qml"));
	qInfo() << left_comp.errors();

	QQmlComponent right_comp(&engine, QUrl("qrc:/right.qml"));
	qInfo() << right_comp.errors();

	struct wl_surface *bg, *top, *bot;
	struct wl_output *output;

	QScreen *screen = app.primaryScreen();
	qDebug() << "primary screen is " << screen->name();

	if (qApp->screens().length() > 1) {
		for (auto &ss: qApp->screens()) {
			bg = create_component(&bg_comp, ss);
			output = getWlOutput(ss);
			qDebug() << "Seting background to screen " << ss->name();
			agl_shell_set_background(agl_shell, bg, output);
		}
	} else {
		bg = create_component(&bg_comp, screen);
		output = getWlOutput(screen);
		qDebug() << "Seting background to screen " << screen->name();
		agl_shell_set_background(agl_shell, bg, output);
	}

	top = create_component(&top_comp, screen);
	bot = create_component(&bot_comp, screen);

	agl_shell_set_panel(agl_shell, bot, output, AGL_SHELL_EDGE_BOTTOM);
	agl_shell_set_panel(agl_shell, top, output, AGL_SHELL_EDGE_TOP);

	/* need to wait a bit in order for qtwayland to start dispatching */
	QTimer::singleShot(500, [agl_shell](){
		agl_shell_ready(agl_shell);
	});

	return app.exec();
}
