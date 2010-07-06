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

#include "vncview.h"

#include <QMessageBox>
#include <QInputDialog>
#define KMessageBox QMessageBox
#define error(parent, message, caption) \
critical(parent, caption, message)

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <QEvent>
#include <QTime>
#include <QTimer>

// Definition of key modifier mask constants
#define KMOD_Alt_R 	0x01
#define KMOD_Alt_L 	0x02
#define KMOD_Meta_L 	0x04
#define KMOD_Control_L 	0x08
#define KMOD_Shift_L	0x10

VncView::VncView(QWidget *parent, const KUrl &url, RemoteView::Quality quality)
        : RemoteView(parent),
        m_initDone(false),
        m_buttonMask(0),
	cursor_x(0),
	cursor_y(0),
        m_repaint(false),
        m_quitFlag(false),
        m_firstPasswordTry(true),
        m_authenticaionCanceled(false),
        m_dontSendClipboard(false),
        m_horizontalFactor(1.0),
        m_verticalFactor(1.0),
        m_forceLocalCursor(false),
	force_full_repaint(false),
	quality(quality)
{
    m_url = url;
    m_host = url.host();
    m_port = url.port();

    connect(&vncThread, SIGNAL(imageUpdated(int, int, int, int)), this, SLOT(updateImage(int, int, int, int)), Qt::BlockingQueuedConnection);
    connect(&vncThread, SIGNAL(gotCut(const QString&)), this, SLOT(setCut(const QString&)), Qt::BlockingQueuedConnection);
    connect(&vncThread, SIGNAL(passwordRequest()), this, SLOT(requestPassword()), Qt::BlockingQueuedConnection);
    connect(&vncThread, SIGNAL(outputErrorMessage(QString)), this, SLOT(outputErrorMessage(QString)));

    m_clipboard = QApplication::clipboard();
    connect(m_clipboard, SIGNAL(selectionChanged()), this, SLOT(clipboardSelectionChanged()));
    connect(m_clipboard, SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));
}

VncView::~VncView()
{
    unpressModifiers();

    // Disconnect all signals so that we don't get any more callbacks from the client thread
    disconnect(&vncThread, SIGNAL(imageUpdated(int, int, int, int)), this, SLOT(updateImage(int, int, int, int)));
    disconnect(&vncThread, SIGNAL(gotCut(const QString&)), this, SLOT(setCut(const QString&)));
    disconnect(&vncThread, SIGNAL(passwordRequest()), this, SLOT(requestPassword()));
    disconnect(&vncThread, SIGNAL(outputErrorMessage(QString)), this, SLOT(outputErrorMessage(QString)));

    startQuitting();
}

void VncView::forceFullRepaint()
{
	force_full_repaint = true;
	repaint();
}

bool VncView::eventFilter(QObject *obj, QEvent *event)
{
    if (m_viewOnly) {
        if (event->type() == QEvent::KeyPress ||
                event->type() == QEvent::KeyRelease ||
                event->type() == QEvent::MouseButtonDblClick ||
                event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseButtonRelease ||
                event->type() == QEvent::Wheel ||
                event->type() == QEvent::MouseMove)
            return true;
    }
    return RemoteView::eventFilter(obj, event);
}

QSize VncView::framebufferSize()
{
    return m_frame.size();
}

QSize VncView::sizeHint() const
{
    return size();
}

QSize VncView::minimumSizeHint() const
{
    return size();
}

void VncView::scaleResize(int w, int h)
{
    RemoteView::scaleResize(w, h);
    
    kDebug(5011) << "scaleResize(): " <<w << h;
    if (m_scale) {
        m_verticalFactor = (qreal) h / m_frame.height();
        m_horizontalFactor = (qreal) w / m_frame.width();

        m_verticalFactor = m_horizontalFactor = qMin(m_verticalFactor, m_horizontalFactor);

        const qreal newW = m_frame.width() * m_horizontalFactor;
        const qreal newH = m_frame.height() * m_verticalFactor;
	/*
        setMaximumSize(newW, newH); //This is a hack to force Qt to center the view in the scroll area
	//also causes the widget's size to flicker
	*/
        resize(newW, newH);
    } 
}

void VncView::updateConfiguration()
{
    RemoteView::updateConfiguration();

    // Update the scaling mode in case KeepAspectRatio changed
    if(parentWidget())
	    scaleResize(parentWidget()->width(), parentWidget()->height());
    else
	scaleResize(width(), height());
}

void VncView::startQuitting()
{
    kDebug(5011) << "about to quit";

    const bool connected = status() == RemoteView::Connected;

    setStatus(Disconnecting);

    m_quitFlag = true;

    if (connected) {
        vncThread.stop();
    }

    vncThread.quit();

    const bool quitSuccess = vncThread.wait(500);

    kDebug(5011) << "Quit VNC thread success:" << quitSuccess;

    setStatus(Disconnected);
}

bool VncView::isQuitting()
{
    return m_quitFlag;
}

bool VncView::start()
{
    vncThread.setHost(m_host);
    vncThread.setPort(m_port);

    vncThread.setQuality(quality);

    // set local cursor on by default because low quality mostly means slow internet connection
    if (quality == RemoteView::Low) {
        showDotCursor(RemoteView::CursorOn);
    }

    setStatus(Connecting);

    vncThread.start();
    return true;
}

bool VncView::supportsScaling() const
{
    return true;
}

bool VncView::supportsLocalCursor() const
{
    return true;
}

void VncView::requestPassword()
{
    kDebug(5011) << "request password";

    if (m_authenticaionCanceled) {
        startQuitting();
        return;
    }

    setStatus(Authenticating);

    if (!m_url.password().isNull()) {
        vncThread.setPassword(m_url.password());
        return;
    }

    bool ok;
    QString password = QInputDialog::getText(this, //krazy:exclude=qclasses
                                             tr("Password required"),
                                             tr("Please enter the password for the remote desktop:"),
                                             QLineEdit::Password, QString(), &ok);
    m_firstPasswordTry = false;
    if (ok)
        vncThread.setPassword(password);
    else
        m_authenticaionCanceled = true;
}

void VncView::outputErrorMessage(const QString &message)
{
    kDebug(5011) << message;

    if (message == "INTERNAL:APPLE_VNC_COMPATIBILTY") {
        setCursor(localDotCursor());
        m_forceLocalCursor = true;
        return;
    }

    startQuitting();

    emit errorMessage(i18n("VNC failure"), message);
}

void VncView::updateImage(int x, int y, int w, int h)
{
//     kDebug(5011) << "got update" << width() << height();

    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;

    if (m_horizontalFactor != 1.0 || m_verticalFactor != 1.0) {
        // If the view is scaled, grow the update rectangle to avoid artifacts
        m_x-=1;
        m_y-=1;
        m_w+=2;
        m_h+=2;
    }

    m_frame = vncThread.image();

    if (!m_initDone) {
        setAttribute(Qt::WA_StaticContents);
        setAttribute(Qt::WA_OpaquePaintEvent);
        installEventFilter(this);

        setCursor(((m_dotCursorState == CursorOn) || m_forceLocalCursor) ? localDotCursor() : Qt::BlankCursor);

        setMouseTracking(true); // get mouse events even when there is no mousebutton pressed
        setFocusPolicy(Qt::WheelFocus);
        setStatus(Connected);
//         emit framebufferSizeChanged(m_frame.width(), m_frame.height());
        emit connected();
        
        if (m_scale) {
            if (parentWidget())
                scaleResize(parentWidget()->width(), parentWidget()->height());
	    else
                scaleResize(width(), height());
        } 
        
        m_initDone = true;

    }

	static QSize old_frame_size = QSize();
    if ((y == 0 && x == 0) && (m_frame.size() != old_frame_size)) {
	    old_frame_size = m_frame.size();
        kDebug(5011) << "Updating framebuffer size";
        if (m_scale) {
            //setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
            if (parentWidget())
                scaleResize(parentWidget()->width(), parentWidget()->height());
        } else {
            kDebug(5011) << "Resizing: " << m_frame.width() << m_frame.height();
            resize(m_frame.width(), m_frame.height());
            //setMaximumSize(m_frame.width(), m_frame.height()); //This is a hack to force Qt to center the view in the scroll area
            //setMinimumSize(m_frame.width(), m_frame.height());
        }
        emit framebufferSizeChanged(m_frame.width(), m_frame.height());
    }

    m_repaint = true;
    repaint(qRound(m_x * m_horizontalFactor), qRound(m_y * m_verticalFactor), qRound(m_w * m_horizontalFactor), qRound(m_h * m_verticalFactor));
    m_repaint = false;
}

void VncView::setViewOnly(bool viewOnly)
{
    RemoteView::setViewOnly(viewOnly);

    m_dontSendClipboard = viewOnly;

    if (viewOnly)
        setCursor(Qt::ArrowCursor);
    else
        setCursor(m_dotCursorState == CursorOn ? localDotCursor() : Qt::BlankCursor);
}

void VncView::showDotCursor(DotCursorState state)
{
    RemoteView::showDotCursor(state);

    setCursor(state == CursorOn ? localDotCursor() : Qt::BlankCursor);
}

void VncView::enableScaling(bool scale)
{
    RemoteView::enableScaling(scale);

    if (scale) {
        //setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        //setMinimumSize(1, 1);
        if (parentWidget())
            scaleResize(parentWidget()->width(), parentWidget()->height());
	    else
		scaleResize(width(), height());
    } else {
        m_verticalFactor = 1.0;
        m_horizontalFactor = 1.0;

        //setMaximumSize(m_frame.width(), m_frame.height()); //This is a hack to force Qt to center the view in the scroll area
        //setMinimumSize(m_frame.width(), m_frame.height());
        resize(m_frame.width(), m_frame.height());
    }
}

void VncView::setCut(const QString &text)
{
    m_dontSendClipboard = true;
    m_clipboard->setText(text, QClipboard::Clipboard);
    m_clipboard->setText(text, QClipboard::Selection);
    m_dontSendClipboard = false;
}

void VncView::paintEvent(QPaintEvent *event)
{
//     kDebug(5011) << "paint event: x: " << m_x << ", y: " << m_y << ", w: " << m_w << ", h: " << m_h;
    if (m_frame.isNull() || m_frame.format() == QImage::Format_Invalid) {
        kDebug(5011) << "no valid image to paint";
        RemoteView::paintEvent(event);
        return;
    }

    event->accept();

    QPainter painter(this);

    if (m_repaint) {
//         kDebug(5011) << "normal repaint";
        painter.drawImage(QRect(qRound(m_x*m_horizontalFactor), qRound(m_y*m_verticalFactor),
                                qRound(m_w*m_horizontalFactor), qRound(m_h*m_verticalFactor)), 
                          m_frame.copy(m_x, m_y, m_w, m_h).scaled(qRound(m_w*m_horizontalFactor), 
                                                                  qRound(m_h*m_verticalFactor),
                                                                  Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
         kDebug(5011) << "resize repaint";
        QRect rect = event->rect();
        if (!force_full_repaint and (rect.width() != width() || rect.height() != height())) {
             kDebug(5011) << "Partial repaint";
            const int sx = rect.x()/m_horizontalFactor;
            const int sy = rect.y()/m_verticalFactor;
            const int sw = rect.width()/m_horizontalFactor;
            const int sh = rect.height()/m_verticalFactor;
            painter.drawImage(rect, 
                              m_frame.copy(sx, sy, sw, sh).scaled(rect.width(), rect.height(),
                                                                  Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        } else {
             kDebug(5011) << "Full repaint" << width() << height() << m_frame.width() << m_frame.height();
            painter.drawImage(QRect(0, 0, width(), height()), 
                              m_frame.scaled(m_frame.width() * m_horizontalFactor, m_frame.height() * m_verticalFactor,
                                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	    force_full_repaint = false;
        }
    }

    RemoteView::paintEvent(event);
}

void VncView::resizeEvent(QResizeEvent *event)
{
    RemoteView::resizeEvent(event);
    scaleResize(event->size().width(), event->size().height());
    forceFullRepaint();
}

bool VncView::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
//         kDebug(5011) << "keyEvent";
        keyEventHandler(static_cast<QKeyEvent*>(event));
        return true;
        break;
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
//         kDebug(5011) << "mouseEvent";
        mouseEventHandler(static_cast<QMouseEvent*>(event));
        return true;
        break;
    case QEvent::Wheel:
//         kDebug(5011) << "wheelEvent";
        wheelEventHandler(static_cast<QWheelEvent*>(event));
        return true;
        break;
    default:
        return RemoteView::event(event);
    }
}


void VncView::mouseEventHandler(QMouseEvent *e)
{
	static bool tap_detected = false;
	static bool double_tap_detected = false;
	static bool tap_drag_detected = false;
	static QTime press_time;
	static QTime up_time; //used for double clicks/tap&drag, for time after first tap
static QTime foo;

	if(!e) { //flush held taps
		if(tap_detected) {
			m_buttonMask |= 0x01;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			m_buttonMask &= 0xfe;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			tap_detected = false;
		} else if(double_tap_detected and press_time.elapsed() > 200) { //got tap + another press -> tap & drag
			m_buttonMask |= 0x01;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			foo.start();
			double_tap_detected = false;
			tap_drag_detected = true;
		}
			
		return;
	}

	if(e->x() < 0 or e->y() < 0) { //QScrollArea tends to send invalid events sometimes...
		e->ignore();
		return;
	}

	cursor_x = qRound(e->x()/m_horizontalFactor);
	cursor_y = qRound(e->y()/m_verticalFactor);
	vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask); // plain move event

	if(e->type() == QEvent::MouseButtonPress or e->type() == QEvent::MouseButtonDblClick) {
		press_time.start();
		if(tap_detected and up_time.elapsed() < 500) {
			tap_detected = false;
			double_tap_detected = true;

			QTimer::singleShot(200, this, SLOT(mouseEventHandler()));
		}
	} else if(e->type() == QEvent::MouseButtonRelease) {
		if(tap_drag_detected) {
			m_buttonMask &= 0xfe;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			tap_drag_detected = false;
		} else if(double_tap_detected) { //double click
			double_tap_detected = false;

			m_buttonMask |= 0x01;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			m_buttonMask &= 0xfe;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			m_buttonMask |= 0x01;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			m_buttonMask &= 0xfe;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
		} else if(press_time.elapsed() < 200) { //tap
			up_time.start();
			tap_detected = true;
			QTimer::singleShot(500, this, SLOT(mouseEventHandler()));
		}

	}

/* for reference:
    if (e->type() != QEvent::MouseMove) {
        if ((e->type() == QEvent::MouseButtonPress)) {
            if (e->button() & Qt::LeftButton)
                m_buttonMask |= 0x01;
            if (e->button() & Qt::MidButton)
                m_buttonMask |= 0x02;
            if (e->button() & Qt::RightButton)
                m_buttonMask |= 0x04;
        } else if (e->type() == QEvent::MouseButtonRelease) {
            if (e->button() & Qt::LeftButton)
                m_buttonMask &= 0xfe;
            if (e->button() & Qt::MidButton)
                m_buttonMask &= 0xfd;
            if (e->button() & Qt::RightButton)
                m_buttonMask &= 0xfb;
	*/
}

void VncView::wheelEventHandler(QWheelEvent *event)
{
    int eb = 0;
    if (event->delta() < 0)
        eb |= 0x10;
    else
        eb |= 0x8;

    const int x = qRound(event->x() / m_horizontalFactor);
    const int y = qRound(event->y() / m_verticalFactor);

	kDebug(5011) << "Wheelevent";
    vncThread.mouseEvent(x, y, eb | m_buttonMask);
    vncThread.mouseEvent(x, y, m_buttonMask);
}

void VncView::keyEventHandler(QKeyEvent *e)
{
    // strip away autorepeating KeyRelease; see bug #206598
    if (e->isAutoRepeat() && (e->type() == QEvent::KeyRelease))
        return;

// parts of this code are based on http://italc.sourcearchive.com/documentation/1.0.9.1/vncview_8cpp-source.html
    rfbKeySym k = e->nativeVirtualKey();

    // we do not handle Key_Backtab separately as the Shift-modifier
    // is already enabled
    if (e->key() == Qt::Key_Backtab) {
        k = XK_Tab;
    }

    const bool pressed = (e->type() == QEvent::KeyPress);

    // handle modifiers
    if (k == XK_Shift_L || k == XK_Control_L || k == XK_Meta_L || k == XK_Alt_L) {
        if (pressed) {
            m_mods[k] = true;
        } else if (m_mods.contains(k)) {
            m_mods.remove(k);
        } else {
            unpressModifiers();
        }
    }


    //handle clicks via zoom buttons
    if(e->key() == Qt::Key_F8) {
	if(pressed)
		m_buttonMask |= 0x01;
	else
		m_buttonMask &= 0xfe;
	vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	kDebug(5011) << "left zoom";
    } else if(e->key() == Qt::Key_F7) {
	if(pressed)
		m_buttonMask |= 0x04;
	else
		m_buttonMask &= 0xfb;
	vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	kDebug(5011) << "right zoom";
    } else if (k) {
        vncThread.keyEvent(k, pressed);
    }
}

void VncView::unpressModifiers()
{
    const QList<unsigned int> keys = m_mods.keys();
    QList<unsigned int>::const_iterator it = keys.constBegin();
    while (it != keys.end()) {
        vncThread.keyEvent(*it, false);
        it++;
    }
    m_mods.clear();
}

void VncView::clipboardSelectionChanged()
{
    kDebug(5011);

    if (m_status != Connected)
        return;

    if (m_clipboard->ownsSelection() || m_dontSendClipboard)
        return;

    const QString text = m_clipboard->text(QClipboard::Selection);

    vncThread.clientCut(text);
}

void VncView::clipboardDataChanged()
{
    kDebug(5011);

    if (m_status != Connected)
        return;

    if (m_clipboard->ownsClipboard() || m_dontSendClipboard)
        return;

    const QString text = m_clipboard->text(QClipboard::Clipboard);

    vncThread.clientCut(text);
}

#include "moc_vncview.cpp"
