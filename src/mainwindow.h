#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>

#include "remoteview.h"
#include "vncview.h"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QString url, int quality);
public slots:
	void about();
	void connectDialog();
	void disconnectFromHost();
	void forceResize();
	void forceResizeDelayed();
	void sendTab() { vnc_view->sendKey(Qt::Key_Tab); }
	void sendEsc() { vnc_view->sendKey(Qt::Key_Escape); }
	void sendPgUp() { vnc_view->sendKey(Qt::Key_PageUp); }
	void sendPgDn() { vnc_view->sendKey(Qt::Key_PageDown); }
	void showModifierMenu();
	void showPreferences();
	void statusChanged(RemoteView::RemoteStatus status);
	void toggleFullscreen();
protected:
	//virtual bool event(QEvent *event);
	void closeEvent(QCloseEvent*);
private:
	void grabZoomKeys(bool grab);
	void reloadSettings();
	VncView *vnc_view;
	QScrollArea *scroll_area;
	//QWidget *menu;
	QToolBar *toolbar;	
	//QPoint swipe_start;
	QAction *scaling;
	QAction *disconnect_action;
};
#endif
