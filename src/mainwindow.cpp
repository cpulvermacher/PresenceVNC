/*
    Presence VNC
    Copyright (C) 2010 Christian Pulvermacher

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "connectdialog.h"
#include "fullscreenexitbutton.h"
#include "keymenu.h"
#include "mainwindow.h"
#include "preferences.h"
#include "vncview.h"

#ifdef Q_WS_MAEMO_5
#include <QtMaemo5>
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif



MainWindow::MainWindow(QString url, int quality):
	QMainWindow(0),
	vnc_view(0),
	scroll_area(new ScrollArea(0)),
	key_menu(0)
{
	setWindowTitle("Presence VNC");

	migrateConfiguration();
	QSettings settings;

#ifdef Q_WS_MAEMO_5
	setAttribute(Qt::WA_Maemo5StackedWindow);
#endif

	//set up toolbar
	toolbar = new QToolBar(0);
	toolbar->addAction(QChar(0x2026), this, SLOT(showKeyMenu())); //"..." button
	toolbar->addAction(tr("Tab"), this, SLOT(sendTab()));
	toolbar->addAction(tr("Esc"), this, SLOT(sendEsc()));
	toolbar->addAction(tr("PgUp"), this, SLOT(sendPgUp()));
	toolbar->addAction(tr("PgDn"), this, SLOT(sendPgDn()));
#ifdef Q_WS_MAEMO_5
	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/chat_enter.png"), "", this, SLOT(sendReturn()));
	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/control_keyboard.png"), "", this, SLOT(showInputPanel()));
#endif

	//move remaining buttons to the right
	QWidget *spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	toolbar->addWidget(spacer);

	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/general_fullsize.png"), "", this, SLOT(toggleFullscreen()));
	addToolBar(toolbar);
	toolbar->setVisible(settings.value("show_toolbar", true).toBool());

	//set up zoombar
	zoombar = new QToolBar(0);
	zoom_slider = new QSlider(Qt::Horizontal, 0);
	zoom_slider->setRange(0, 100);
	connect(zoom_slider, SIGNAL(valueChanged(int)),
		this, SLOT(setZoomLevel(int)));
	zoom_slider->setValue(settings.value("zoomlevel", 95).toInt());
	zoombar->addWidget(zoom_slider);
	addToolBar(zoombar);

	//set up menu
	QAction *connect_action = new QAction(tr("Connect"), this);
	disconnect_action = new QAction(tr("Disconnect"), this);
	show_toolbar = new QAction(tr("Show Toolbar"), this);
	show_toolbar->setCheckable(true);
	show_toolbar->setChecked(settings.value("show_toolbar", true).toBool());
	QAction *pref_action = new QAction(tr("Preferences"), this);
	QAction *about_action = new QAction(tr("About"), this);

#ifdef Q_WS_MAEMO_5
	menuBar()->addAction(connect_action);
	menuBar()->addAction(disconnect_action);
	menuBar()->addAction(show_toolbar);
	menuBar()->addAction(pref_action);
	menuBar()->addAction(about_action);
#else
	QMenu* session_menu = menuBar()->addMenu(tr("&Session"));
	session_menu->addAction(connect_action);
	session_menu->addAction(disconnect_action);
	session_menu->addSeparator();
	session_menu->addAction(pref_action);
	session_menu->addSeparator();
	session_menu->addAction(tr("&Quit"), this, SLOT(close()));

	QMenu* view_menu = menuBar()->addMenu(tr("&View"));
	view_menu->addAction(show_toolbar);

	QMenu* help_menu = menuBar()->addMenu(tr("&Help"));
	help_menu->addAction(about_action);
#endif

	connect(about_action, SIGNAL(triggered()),
		this, SLOT(about()));
	connect(pref_action, SIGNAL(triggered()),
		this, SLOT(showPreferences()));
	connect(connect_action, SIGNAL(triggered()),
		this, SLOT(showConnectDialog()));
	connect(disconnect_action, SIGNAL(triggered()),
		this, SLOT(disconnectFromHost()));
	connect(show_toolbar, SIGNAL(toggled(bool)),
		toolbar, SLOT(setVisible(bool)));
	connect(show_toolbar, SIGNAL(toggled(bool)),
		this, SLOT(forceResizeDelayed()));

	setCentralWidget(scroll_area);
	new FullScreenExitButton(this);

	grabZoomKeys(true);
	reloadSettings();

	if(url.isNull()) {
		disconnect_action->setEnabled(false);
		toolbar->setEnabled(false);
		showConnectDialog();
	} else {
		vnc_view = new VncView(this, url, RemoteView::Quality(quality));
		connect(vnc_view, SIGNAL(statusChanged(RemoteView::RemoteStatus)),
			this, SLOT(statusChanged(RemoteView::RemoteStatus)));
		scroll_area->setWidget(vnc_view);
		vnc_view->start();
		key_menu = new KeyMenu(this);
	}

	//QTimer::singleShot(500, this, SLOT(close()));
}

void MainWindow::grabZoomKeys(bool grab)
{
#ifdef Q_WS_MAEMO_5
	unsigned long val = (grab)?1:0;
	Atom atom = XInternAtom(QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", False);
	if(!atom) {
		qWarning("Couldn't get zoom key atom");
		return;
	}
	XChangeProperty(QX11Info::display(), winId(), atom, XA_INTEGER,
		32, PropModeReplace, reinterpret_cast<unsigned char *>(&val), 1);
#endif
}

void MainWindow::closeEvent(QCloseEvent*) {
	grabZoomKeys(false);

	QSettings settings;
	settings.setValue("show_toolbar", show_toolbar->isChecked());
	settings.setValue("zoomlevel", zoom_slider->value());
	settings.sync();

	hide();

	disconnectFromHost();
}

void MainWindow::about() {
	QMessageBox::about(this, tr("About Presence VNC"),
		tr("<center><h1>Presence VNC 0.6</h1>\
<p>A touchscreen friendly VNC client</p>\
<p><a href=\"https://garage.maemo.org/projects/presencevnc/\">https://garage.maemo.org/projects/presencevnc</a></p></center>\
<small><p>&copy;2010 Christian Pulvermacher &lt;pulvermacher@gmx.de&gt;<br />\
Based on KRDC, &copy; 2007-2008 Urs Wolfer<br />\
and LibVNCServer, &copy; 2001-2003 Johannes E. Schindelin</p>\
<p>This program is free software; License: <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU GPL 2</a> or later.</p></small>"));
}

void MainWindow::showConnectDialog()
{
	ConnectDialog *connect_dialog = new ConnectDialog(this);
	connect(connect_dialog, SIGNAL(connectToHost(QString, int)),
		this, SLOT(connectToHost(QString, int)));
	connect_dialog->exec();
}

void MainWindow::connectToHost(QString url, int quality)
{
	disconnectFromHost();

	vnc_view = new VncView(this, url, RemoteView::Quality(quality));

	connect(vnc_view, SIGNAL(statusChanged(RemoteView::RemoteStatus)),
		this, SLOT(statusChanged(RemoteView::RemoteStatus)));
	scroll_area->setWidget(vnc_view);
	vnc_view->start();
	disconnect_action->setEnabled(true);
	toolbar->setEnabled(true);

	if(key_menu) //reset
		delete key_menu;
	key_menu = new KeyMenu(this);
}

void MainWindow::disconnectFromHost()
{
	if(!vnc_view)
		return;

	scroll_area->setWidget(0);

	vnc_view->disconnect(); //remove all signal-slot connections
	delete vnc_view;
	vnc_view = 0;
	disconnect_action->setEnabled(false);
	toolbar->setEnabled(false);
}

void MainWindow::statusChanged(RemoteView::RemoteStatus status)
{
	static RemoteView::RemoteStatus old_status = RemoteView::Disconnected;

	switch(status) {
	case RemoteView::Connecting:
#ifdef Q_WS_MAEMO_5
		setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif
		break;
	case RemoteView::Connected:
#ifdef Q_WS_MAEMO_5
		setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
		vnc_view->setZoomLevel(zoom_slider->value());
		vnc_view->forceFullRepaint();
		break;
	case RemoteView::Disconnecting:
		if(old_status != RemoteView::Disconnected) { //Disconnecting also occurs while connecting, so check last state
#ifdef Q_WS_MAEMO_5
			QMaemo5InformationBox::information(this, tr("Connection lost"));
#endif
			
			//clean up
			scroll_area->setWidget(0);
			vnc_view = 0;
			disconnect_action->setEnabled(false);
			toolbar->setEnabled(false);

			//exit fullscreen mode
			if(windowState() & Qt::WindowFullScreen)
				setWindowState(windowState() ^ Qt::WindowFullScreen);
		}
		break;
	case RemoteView::Disconnected:
#ifdef Q_WS_MAEMO_5
		setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
		if(old_status == RemoteView::Disconnecting) {
			scroll_area->setWidget(0); //remove widget
		}
		break;
	}

	old_status = status;
}

//resizes the widget to use available screen space
//necessary when rotating, showing fullscreen, etc.
void MainWindow::forceResize()
{
	if(vnc_view) {
		vnc_view->resize(scroll_area->size());
	}
} 

void MainWindow::forceResizeDelayed()
{
	QTimer::singleShot(500, this, SLOT(forceResize()));
}

void MainWindow::toggleFullscreen()
{
	bool in_fullscreen = windowState() & Qt::WindowFullScreen;

	//hide menu/toolbar in fullscreen (new state is !in_fullscreen)
	toolbar->setVisible(show_toolbar->isChecked() and in_fullscreen);

#ifndef Q_WS_MAEMO_5
	//menu bar is invisible by default on maemo
	menuBar()->setVisible(in_fullscreen);
#endif

	setWindowState(windowState() ^ Qt::WindowFullScreen); 
	forceResizeDelayed();
}

void MainWindow::showKeyMenu()
{
	key_menu->exec();
	vnc_view->sendKeySequence(key_menu->getKeySequence());
}

void MainWindow::showPreferences()
{
	Preferences *p = new Preferences(this);
	p->exec();
	delete p;

	reloadSettings();
}

void MainWindow::reloadSettings()
{
	QSettings settings;
#ifdef Q_WS_MAEMO_5
	int rotation = settings.value("screen_rotation", 0).toInt();
	setAttribute(Qt::WA_Maemo5AutoOrientation, rotation == 0);
	setAttribute(Qt::WA_Maemo5LandscapeOrientation, rotation == 1);
	setAttribute(Qt::WA_Maemo5PortraitOrientation, rotation == 2);
#endif

	if(vnc_view)
		vnc_view->reloadSettings();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);

	forceResize();
	if(vnc_view)
		vnc_view->setZoomLevel(zoom_slider->value());
}

void MainWindow::showInputPanel()
{
#ifdef Q_WS_MAEMO_5
	//TODO: when hardware keyboard is open, this will only cause the IM to mess up 'real' key events
	vnc_view->setAttribute(Qt::WA_InputMethodEnabled, true);
	vnc_view->setInputMethodHints(Qt::ImhNoAutoUppercase); //without this, IM starts with caps lock

	QEvent event(QEvent::RequestSoftwareInputPanel);
	QApplication::sendEvent(vnc_view, &event);
#endif
}

void MainWindow::setZoomLevel(int level)
{
	if(vnc_view)
		vnc_view->setZoomLevel(level);
}
