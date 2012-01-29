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
#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include <QScrollArea>
#include <QLabel>
#include <QTimer>

//fixes tearing during scrolling and can display messages
class ScrollArea : public QScrollArea
{
public:
    explicit ScrollArea(QWidget *parent):
        QScrollArea(parent) {
        message.setParent(this);
        message.setVisible(false);
        message.setAlignment(Qt::AlignCenter);
        message.setWordWrap(true);

        QPalette pal = message.palette();
        QColor backgroundColor = pal.color(backgroundRole());
        backgroundColor.setAlpha(128);
        pal.setColor(backgroundRole(), backgroundColor);
        message.setPalette(pal);
        message.setAutoFillBackground(true);

        message_timer.setSingleShot(true);
        message_timer.setInterval(2500);
        connect(&message_timer, SIGNAL(timeout()),
                &message, SLOT(hide()));

#ifdef Q_WS_MAEMO_5
        // disable overshooting because it somehow causes repaint events for the whole widget (slow)
        QAbstractKineticScroller *scroller = property("kineticScroller").value<QAbstractKineticScroller *>();
        if (scroller)
            scroller->setOvershootPolicy(QAbstractKineticScroller::OvershootAlwaysOff);
#endif
    }

    void showMessage(const QString &s) {
        message.setText(s);
        message.show();
        message_timer.start();
    }
protected:
    virtual void resizeEvent(QResizeEvent* ev) {
        QScrollArea::resizeEvent(ev);
        message.setFixedWidth(width());
    }
    virtual void scrollContentsBy(int dx, int dy) {
        QScrollArea::scrollContentsBy(dx, dy);
        if(widget())
            message.hide(); //overlay-widget slows down scrolling
    }
private:
    QLabel message;
    QTimer message_timer;
};
#endif
