/****************************************************************************
**
** Copyright (C) 2007-2008 Urs Wolfer <uwolfer @ kde.org>
**
** This file is part of KDE.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING. If not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301, USA.
**
****************************************************************************/

#ifndef VNCVIEW_H
#define VNCVIEW_H

#include "remoteview.h"
#include "vncclientthread.h"

class KConfigGroup {};

#include <QClipboard>

extern "C" {
#include <rfb/rfbclient.h>
}

class VncView: public RemoteView
{
    Q_OBJECT

public:
    explicit VncView(QWidget *parent = 0, const KUrl &url = KUrl(), RemoteView::Quality quality = RemoteView::Medium, int listen_port = 0);
    ~VncView();

    QSize framebufferSize() const;
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    bool isQuitting() const;
    qreal zoomFactor() const { return m_horizontalFactor;} //assumes fixed aspect ratio
    void reloadSettings();
    bool start();
    bool supportsScaling() const;
    bool supportsLocalCursor() const;

    void setQuality(int q);
    void setViewOnly(bool viewOnly);
    void showDotCursor(DotCursorState state);
    void useFastTransformations(bool enabled);
    QPoint cursorPosition() const { return QPoint(cursor_x, cursor_y); }
    void setDisplayOff(bool off) { display_off = off; }

public slots:
    void setZoomLevel(int level = -1); //'level' doesn't correspond to actual magnification, though mapping is done here
    void sendKey(Qt::Key key);
    void sendKeySequence(QKeySequence keys);
    void startQuitting();

protected:
    void paintEvent(QPaintEvent *event);
    bool event(QEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void inputMethodEvent(QInputMethodEvent *event);

private:
    VncClientThread vncThread;
    QClipboard *m_clipboard;
    bool m_initDone;
    int m_buttonMask;
    QMap<unsigned int, bool> m_mods;
    int m_x, m_y, m_w, m_h;
    int cursor_x, cursor_y;
    bool m_quitFlag;
    bool m_firstPasswordTry;
    bool m_dontSendClipboard;
    qreal m_horizontalFactor;
    qreal m_verticalFactor;
    QImage m_frame;
    bool m_forceLocalCursor;
    int left_zoom, right_zoom;
    bool disable_tapping;
    RemoteView::Quality quality;
    int listen_port;
    Qt::TransformationMode transformation_mode;
    bool display_off;

    void keyEventHandler(QKeyEvent *e);
    void unpressModifiers();
    void wheelEventHandler(QWheelEvent *event);

private slots:
    void mouseEventHandler(QMouseEvent *event = 0);

    void updateImage(int x, int y, int w, int h);
    void setCut(const QString &text);
    void requestPassword();
    void outputErrorMessage(const QString &message);
    void clipboardSelectionChanged();
    void clipboardDataChanged();
};

#endif
