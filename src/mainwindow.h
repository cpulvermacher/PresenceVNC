
#include <QtGui>

#include <QX11Info>

#include "vncview.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <iostream>

class MainWindow : public QScrollArea {
	Q_OBJECT
private:
	VncView *vnc_view;
	QWidget *menu;
	QPoint swipe_start;

public:
	MainWindow(QString url, int quality) : QScrollArea(0) {
		swipe_start = QPoint(0,0);
		setAttribute(Qt::WA_Maemo5StackedWindow);

		vnc_view = new VncView(0, url, RemoteView::Quality(quality));
		setWidget(vnc_view);

		//set up menu
		QMenuBar *menu = new QMenuBar(this);
		QAction *scaling = new QAction("Rescale Remote Screen", this);
		scaling->setCheckable(true);
		scaling->setChecked(true);
		menu->addAction(scaling);
		QAction *about_action = new QAction("About", this);
		menu->addAction(about_action);

		//menu->setAttribute(Qt::WA_Maemo5StackedWindow);
		//menu->hide();

		connect(scaling, SIGNAL(toggled(bool)),
			vnc_view, SLOT(enableScaling(bool)));
		connect(about_action, SIGNAL(triggered()),
			this, SLOT(about()));

		grabZoomKeys(true);
		setAttribute(Qt::WA_Maemo5AutoOrientation, true);
		//setAttribute(Qt::WA_Maemo5PortraitOrientation, true);

		vnc_view->start();
	}

	void grabZoomKeys(bool grab)
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
	void closeEvent(QCloseEvent*) {
		hide();
		grabZoomKeys(false);
		vnc_view->startQuitting();
	}
public slots:
	void about() {
		QMessageBox::about(this, tr("About Presence VNC"),
			tr("<center><h1>Presence VNC 0.1 alpha</h1>\
A touch screen friendly VNC client\
<small><p>&copy;2010 Christian Pulvermacher &lt;pulvermacher@gmx.de&gt</p>\
<p>Based on KRDC, &copy; 2007-2008 Urs Wolfer</small></center>\
<p>This program is free software; License: <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU GPL 2</a> or later.</p>"));
	}

protected:
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
};
