#include "mainwindow.h"
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
//	swipe_start = QPoint(0,0);
	setAttribute(Qt::WA_Maemo5StackedWindow);

	//set up toolbar
	toolbar = new QToolBar(0);
	toolbar->addAction("Mod"); //TODO
	toolbar->addAction("Tab", this, SLOT(sendTab()));
	toolbar->addAction("Esc", this, SLOT(sendEsc()));
	toolbar->addAction("PgUp", this, SLOT(sendPgUp()));
	toolbar->addAction("PgDn", this, SLOT(sendPgDn()));
	toolbar->addAction(QIcon("/usr/share/icons/hicolor/48x48/hildon/general_fullsize.png"), "", this, SLOT(toggleFullscreen()));
	addToolBar(toolbar);

	//set up menu
	QMenuBar *menu = new QMenuBar(this);
	QAction *connect_action = new QAction("Connect", this);
	disconnect_action = new QAction("Disconnect", this);
	menu->addAction(connect_action);
	menu->addAction(disconnect_action);
	scaling = new QAction("Rescale Remote Screen", this);
	scaling->setCheckable(true);
	scaling->setChecked(true);
	menu->addAction(scaling);
	QAction *show_toolbar = new QAction("Show Toolbar", this);
	show_toolbar->setCheckable(true);
	show_toolbar->setChecked(true);
	menu->addAction(show_toolbar);
	QAction *about_action = new QAction("About", this);
	menu->addAction(about_action);

	//menu->setAttribute(Qt::WA_Maemo5StackedWindow);
	//menu->hide();

	connect(about_action, SIGNAL(triggered()),
		this, SLOT(about()));
	connect(connect_action, SIGNAL(triggered()),
		this, SLOT(connectDialog()));
	connect(disconnect_action, SIGNAL(triggered()),
		this, SLOT(disconnectFromHost()));
	connect(show_toolbar, SIGNAL(toggled(bool)),
		toolbar, SLOT(setVisible(bool)));
	connect(show_toolbar, SIGNAL(toggled(bool)),
		this, SLOT(forceResizeDelayed()));

	setCentralWidget(scroll_area);

	grabZoomKeys(true);
	setAttribute(Qt::WA_Maemo5AutoOrientation, true);
	//setAttribute(Qt::WA_Maemo5PortraitOrientation, true);

	if(url.isNull()) {
		disconnect_action->setEnabled(false);
		toolbar->setEnabled(false);
		connectDialog();
	} else {
		vnc_view = new VncView(0, url, RemoteView::Quality(quality));
		connect(scaling, SIGNAL(toggled(bool)),
			vnc_view, SLOT(enableScaling(bool)));
		connect(scaling, SIGNAL(toggled(bool)),
			scroll_area, SLOT(setWidgetResizable(bool)));
		connect(vnc_view, SIGNAL(statusChanged(RemoteView::RemoteStatus)),
			this, SLOT(statusChanged(RemoteView::RemoteStatus)));
		scroll_area->setWidget(vnc_view);
		vnc_view->start();
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
	hide();
	grabZoomKeys(false);
	disconnectFromHost();
}

void MainWindow::about() {
	QMessageBox::about(this, tr("About Presence VNC"),
		tr("<center><h1>Presence VNC 0.1 alpha</h1>\
A touch screen friendly VNC client\
<small><p>&copy;2010 Christian Pulvermacher &lt;pulvermacher@gmx.de&gt</p>\
<p>Based on KRDC, &copy; 2007-2008 Urs Wolfer</small></center>\
<p>This program is free software; License: <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU GPL 2</a> or later.</p>"));
}

/* swipe not used, and doesn't work without scaling anyway :/
virtual bool event(QEvent *event) {
	if(event->type() == QEvent::MouseMove) {
		QMouseEvent *ev = dynamic_cast<QMouseEvent* >(event);
		if(!swipe_start.isNull()) {
			QPoint diff = swipe_start - ev->pos();
			const int swipe_dist = 60;
			if(diff.x() > swipe_dist and diff.y() < swipe_dist and diff.y() > -swipe_dist) { //
				menu->show();
				swipe_start = QPoint(0,0);
			}
		} else if((width() - ev->x()) < 10) {
			swipe_start = ev->pos();
		}
		std::cout << "mousex: " << width() - ev->x() << "\n";
		//TODO: make scrolling over border result in wheel events? i get weird (out of range) mouse events when that happens
		return true;
	} else if(event->type() == QEvent::MouseButtonRelease) {
		swipe_start = QPoint(0,0);
		return true;
	} else {
//			std::cout << "event " << event->type() << "\n";
		return QScrollArea::event(event);
	}
}
*/

void MainWindow::connectDialog()
{
	QSettings settings;
	QString url = QInputDialog::getText(this, "Connect to Host", "VNC Server:", QLineEdit::Normal, settings.value("last_hostname", "").toString());
	if(url.isEmpty()) { //dialog dismissed or nothing entered
		return;
	}
	settings.setValue("last_hostname", url);
	url = "vnc://" + url;

	disconnectFromHost();

	vnc_view = new VncView(0, url, RemoteView::Quality(2)); //TODO: get quality in dialog
	scroll_area->setWidget(vnc_view);

	connect(scaling, SIGNAL(toggled(bool)),
		vnc_view, SLOT(enableScaling(bool)));
	connect(scaling, SIGNAL(toggled(bool)),
		scroll_area, SLOT(setWidgetResizable(bool)));
	connect(vnc_view, SIGNAL(statusChanged(RemoteView::RemoteStatus)),
		this, SLOT(statusChanged(RemoteView::RemoteStatus)));
	vnc_view->start();
	disconnect_action->setEnabled(true);
	toolbar->setEnabled(true);
}

void MainWindow::disconnectFromHost()
{
	if(!vnc_view)
		return;

	vnc_view->startQuitting();
	scroll_area->setWidget(0);

	vnc_view->disconnect(); //remove all connections
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
		break;
	case RemoteView::Disconnecting:
		if(old_status != RemoteView::Disconnected) { //Disconnecting also occurs while connecting, so check last state
			QMaemo5InformationBox::information(this, "Connection lost");
		}
		break;
	case RemoteView::Disconnected:
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
	forceResizeDelayed();
}
