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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "remoteview.h"
#include "vncview.h"

#include <QtGui>

class KeyMenu;
class ScrollArea;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QString url, int quality);
public slots:
	void about();
	void showConnectDialog();
	void connectToHost(QString url, int quality, int listen_port);
	void disconnectFromHost();
	void forceRepaint();
	void updateScreenSpace();
	void updateScreenSpaceDelayed();
	void sendTab() { vnc_view->sendKey(Qt::Key_Tab); }
	void sendEsc() { vnc_view->sendKey(Qt::Key_Escape); }
	void sendPgUp() { vnc_view->sendKey(Qt::Key_PageUp); }
	void sendPgDn() { vnc_view->sendKey(Qt::Key_PageDown); }
	void sendReturn() { vnc_view->sendKey(Qt::Key_Return); }
	void setZoomLevel(int level);
	void showInputPanel();
	void showKeyMenu();
	void showPreferences();
	void statusChanged(RemoteView::RemoteStatus status);
	void toggleFullscreen();
protected:
	void closeEvent(QCloseEvent*);
	void resizeEvent(QResizeEvent *event);
private:
	void grabZoomKeys(bool grab);
	void reloadSettings();
	VncView *vnc_view;
	ScrollArea *scroll_area;
	QToolBar *toolbar;
	QSlider *zoom_slider;
	QAction *scaling, *show_toolbar, *disconnect_action;
	KeyMenu *key_menu;
};
#endif
