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
	void connectDialog();
	void disconnectFromHost();
	void about();
	void statusChanged(RemoteView::RemoteStatus status);
protected:
	//virtual bool event(QEvent *event);
	void closeEvent(QCloseEvent*);
private:
	void grabZoomKeys(bool grab);
	VncView *vnc_view;
	QScrollArea *scroll_area;
	//QWidget *menu;
	QToolBar *toolbar;	
	QPoint swipe_start;
	QAction *scaling;
	QAction *disconnect_action;

private slots:
	void sendEsc() { vnc_view->sendKey(Qt::Key_Escape); }
	void sendTab() { vnc_view->sendKey(Qt::Key_Tab); }
};
#endif
