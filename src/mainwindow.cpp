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
#include "mainwindow.h"
#include "preferences.h"
#include "vncview.h"

#include <QtMaemo5>
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <iostream>

MainWindow::MainWindow(QString url, int quality):
	QMainWindow(0),
	vnc_view(0),
	scroll_area(new QScrollArea(0))
{
	setWindowTitle("Presence VNC");
	setAttribute(Qt::WA_Maemo5StackedWindow);

	migrateConfiguration();

	QSettings settings;

	//set up toolbar
	toolbar = new QToolBar(0);
	toolbar->addAction("Mod", this, SLOT(showModifierMenu()));
	toolbar->addAction("Tab", this, SLOT(sendTab()));
	toolbar->addAction("Esc", this, SLOT(sendEsc()));
	toolbar->addAction("PgUp", this, SLOT(sendPgUp()));
	toolbar->addAction("PgDn", this, SLOT(sendPgDn()));
	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/chat_enter.png"), "", this, SLOT(sendReturn()));
	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/control_keyboard.png"), "", this, SLOT(showInputPanel()));

	//move fullscreen button to the right
	QWidget *spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	toolbar->addWidget(spacer);

	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/general_fullsize.png"), "", this, SLOT(toggleFullscreen()));
	addToolBar(toolbar);
	toolbar->setVisible(settings.value("show_toolbar", true).toBool());

	//set up menu
	QMenuBar *menu = new QMenuBar(this);
	QAction *connect_action = new QAction("Connect", this);
	disconnect_action = new QAction("Disconnect", this);
	menu->addAction(connect_action);
	menu->addAction(disconnect_action);
	scaling = new QAction("Fit to Screen", this);
	scaling->setCheckable(true);
	scaling->setChecked(settings.value("rescale", true).toBool());
	menu->addAction(scaling);
	show_toolbar = new QAction("Show Toolbar", this);
	show_toolbar->setCheckable(true);
	show_toolbar->setChecked(settings.value("show_toolbar", true).toBool());
	menu->addAction(show_toolbar);
	QAction *pref_action = new QAction("Preferences", this);
	menu->addAction(pref_action);
	QAction *about_action = new QAction("About", this);
	menu->addAction(about_action);

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

	connect(QApplication::desktop(), SIGNAL(resized(int)),
		this, SLOT(forceResize()));

	if(url.isNull()) {
		disconnect_action->setEnabled(false);
		toolbar->setEnabled(false);
		showConnectDialog();
	} else {
		vnc_view = new VncView(this, url, RemoteView::Quality(quality));
		connect(scaling, SIGNAL(toggled(bool)),
			vnc_view, SLOT(enableScaling(bool)));
		connect(vnc_view, SIGNAL(statusChanged(RemoteView::RemoteStatus)),
			this, SLOT(statusChanged(RemoteView::RemoteStatus)));
		scroll_area->setWidget(vnc_view);
		vnc_view->start();
		vnc_view->enableScaling(scaling->isChecked());
	}
}

void MainWindow::grabZoomKeys(bool grab)
{
	unsigned long val = (grab)?1:0;
	Atom atom = XInternAtom(QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", False);
	if(!atom) {
		qWarning("Couldn't get zoom key atom");
		return;
	}
	XChangeProperty(QX11Info::display(), winId(), atom, XA_INTEGER,
		32, PropModeReplace, reinterpret_cast<unsigned char *>(&val), 1);
}

void MainWindow::closeEvent(QCloseEvent*) {
	grabZoomKeys(false);

	QSettings settings;
	settings.setValue("show_toolbar", show_toolbar->isChecked());
	settings.setValue("rescale", scaling->isChecked());
	settings.sync();

	hide();

	disconnectFromHost();
}

void MainWindow::about() {
	QMessageBox::about(this, tr("About Presence VNC"),
		tr("<center><h1>Presence VNC 0.4</h1>\
A touchscreen friendly VNC client\
<small><p>&copy;2010 Christian Pulvermacher &lt;pulvermacher@gmx.de&gt;</p>\
<p>Based on KRDC, &copy; 2007-2008 Urs Wolfer</p>\
<p>and LibVNCServer, &copy; 2001-2003 Johannes E. Schindelin</p></small></center>\
<p>This program is free software; License: <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU GPL 2</a> or later.</p>"));
}

void MainWindow::showConnectDialog()
{
	/*
	QSettings settings;
	QString url = QInputDialog::getText(this, "Connect to Host", "VNC Server:", QLineEdit::Normal, settings.value("last_hostname", "").toString());
	if(url.isEmpty()) { //dialog dismissed or nothing entered
		return;
	}
	*/

	ConnectDialog *connect_dialog = new ConnectDialog(this);
	if(!connect_dialog->exec()) { //dialog rejected
		delete connect_dialog;
		return;
	}

	QString url = connect_dialog->getUrl();
	delete connect_dialog;

	disconnectFromHost();

	vnc_view = new VncView(this, url, RemoteView::Quality(2)); //TODO: get quality in dialog

	connect(scaling, SIGNAL(toggled(bool)),
		vnc_view, SLOT(enableScaling(bool)));
	connect(vnc_view, SIGNAL(statusChanged(RemoteView::RemoteStatus)),
		this, SLOT(statusChanged(RemoteView::RemoteStatus)));
	scroll_area->setWidget(vnc_view);
	vnc_view->start();
	vnc_view->enableScaling(scaling->isChecked());
	disconnect_action->setEnabled(true);
	toolbar->setEnabled(true);
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
		setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
		break;
	case RemoteView::Connected:
		setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
		if(!scaling->isChecked()) {
			//if remote desktop is shown in full size, 2nd connection will have black screen
			//ugly hack to force a refresh (forceFullRepaint() doesn't repaint?? -> vnc_view hidden???)
			vnc_view->resize(scroll_area->size());
			vnc_view->enableScaling(false);
		}
		break;
	case RemoteView::Disconnecting:
		if(old_status != RemoteView::Disconnected) { //Disconnecting also occurs while connecting, so check last state
			QMaemo5InformationBox::information(this, "Connection lost");
			
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
		setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
		if(old_status == RemoteView::Disconnecting) {
			scroll_area->setWidget(0); //remove widget
		}
		break;
	}

	old_status = status;
}

//when rescaling is enabled, this resizes the widget to use available screen space
//necessary when rotating, showing fullscreen, etc.
void MainWindow::forceResize()
{
	if(vnc_view and scaling->isChecked()) {
		vnc_view->resize(scroll_area->size());
	}
} 

void MainWindow::forceResizeDelayed()
{
	QTimer::singleShot(500, this, SLOT(forceResize()));
}

void MainWindow::toggleFullscreen()
{
	setWindowState(windowState() ^ Qt::WindowFullScreen); 
	toolbar->setVisible(show_toolbar->isChecked() and !(windowState() & Qt::WindowFullScreen)); //hide toolbar in fullscreen
	forceResizeDelayed();
}

void MainWindow::showModifierMenu()
{
	static QMenu *mod_menu = new QMenu(tr("Modifiers"), this);
	static QAction *win = mod_menu->addAction(tr("Win"));
	static QAction *alt = mod_menu->addAction(tr("Alt"));
	win->setCheckable(true);
	alt->setCheckable(true);

	//show menu at top-left corner of toolbar
	QAction *chosen = mod_menu->exec(toolbar->mapToGlobal(QPoint(0,0)));
	if(!chosen) {
		return;
	} else if(chosen == alt) {
		vnc_view->sendKey(Qt::Key_Alt);
	} else if(chosen == win) {
		vnc_view->sendKey(Qt::Key_Meta);
	} else {
		std::cout << "unhandled action?\n";
	}
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
	int rotation = settings.value("screen_rotation", 0).toInt();
	setAttribute(Qt::WA_Maemo5AutoOrientation, rotation == 0);
	setAttribute(Qt::WA_Maemo5LandscapeOrientation, rotation == 1);
	setAttribute(Qt::WA_Maemo5PortraitOrientation, rotation == 2);

	if(vnc_view)
		vnc_view->reloadSettings();
}

void MainWindow::showInputPanel()
{
	vnc_view->setAttribute(Qt::WA_InputMethodEnabled, true);
	QEvent event(QEvent::RequestSoftwareInputPanel);
	QApplication::sendEvent(vnc_view, &event);
}
