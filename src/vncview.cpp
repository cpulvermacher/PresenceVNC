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
#include <QBitmap>
#include <QCheckBox>
#include <QDialog>
#include <QImage>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <QEvent>
#include <QSettings>
#include <QTime>
#include <QTimer>


// Definition of key modifier mask constants
#define KMOD_Alt_R 	0x01
#define KMOD_Alt_L 	0x02
#define KMOD_Meta_L 	0x04
#define KMOD_Control_L 	0x08
#define KMOD_Shift_L	0x10

//local cursor width/height in px, should be an odd number
const int cursor_size = 7;

VncView::VncView(QWidget *parent, const KUrl &url, RemoteView::Quality quality, int listen_port)
        : RemoteView(parent),
        m_initDone(false),
        m_buttonMask(0),
	cursor_x(0),
	cursor_y(0),
        m_repaint(false),
        m_quitFlag(false),
        m_firstPasswordTry(true),
        m_dontSendClipboard(false),
        m_horizontalFactor(1.0),
        m_verticalFactor(1.0),
        m_forceLocalCursor(false),
	force_full_repaint(false),
	quality(quality),
	listen_port(listen_port)
{
    m_url = url;
    m_host = url.host();
    m_port = url.port();

	//BlockingQueuedConnection can cause deadlocks when exiting, handled in startQuitting()
    connect(&vncThread, SIGNAL(imageUpdated(int, int, int, int)), this, SLOT(updateImage(int, int, int, int)), Qt::BlockingQueuedConnection);
    connect(&vncThread, SIGNAL(gotCut(const QString&)), this, SLOT(setCut(const QString&)), Qt::BlockingQueuedConnection);
    connect(&vncThread, SIGNAL(passwordRequest()), this, SLOT(requestPassword()), Qt::BlockingQueuedConnection);
    connect(&vncThread, SIGNAL(outputErrorMessage(QString)), this, SLOT(outputErrorMessage(QString)));

    m_clipboard = QApplication::clipboard();
    connect(m_clipboard, SIGNAL(selectionChanged()), this, SLOT(clipboardSelectionChanged()));
    connect(m_clipboard, SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    reloadSettings();
}

VncView::~VncView()
{
    unpressModifiers();

    // Disconnect all signals so that we don't get any more callbacks from the client thread
    vncThread.disconnect();

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

void VncView::startQuitting()
{
    kDebug(5011) << "about to quit";

    //const bool connected = status() == RemoteView::Connected;

    setStatus(Disconnecting);

    m_quitFlag = true;

	//if(connected) //remove if things work without it
	vncThread.stop();

    const bool quitSuccess = vncThread.wait(700);
	if(!quitSuccess) {
		//happens when vncThread wants to call a slot via BlockingQueuedConnection,
		//needs an event loop in this thread so execution continues after 'emit'
		QEventLoop loop;
		if(!loop.processEvents())
			kDebug(5011) << "BUG: deadlocked, but no events to deliver?";
		vncThread.wait(700);
	}
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
	vncThread.setListenPort(listen_port); //if port is != 0, thread will listen for connections
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

    setStatus(Authenticating);

    if (!m_url.password().isNull()) {
        vncThread.setPassword(m_url.password());
        return;
    }

	QSettings settings;
	settings.beginGroup("hosts");
	QString password = settings.value(QString("%1/password").arg(m_host), "").toString();
	//check for saved password
	if(m_firstPasswordTry and !password.isEmpty()) {
		kDebug(5011) << "Trying saved password";
		m_firstPasswordTry = false;
		vncThread.setPassword(password);
		return;
	}
	m_firstPasswordTry = false;

	//build dialog
	QDialog dialog(this);
	dialog.setWindowTitle(tr("Password required"));

	QLineEdit passwordbox;
	passwordbox.setEchoMode(QLineEdit::Password);
	passwordbox.setText(password);
	QCheckBox save_password(tr("Save Password"));
	save_password.setChecked(!password.isEmpty()); //offer to overwrite saved password
	QPushButton ok_button(tr("Done"));
	ok_button.setMaximumWidth(100);
	connect(&ok_button, SIGNAL(clicked()),
		&dialog, SLOT(accept()));

	QHBoxLayout layout1;
	QVBoxLayout layout2;
	layout2.addWidget(&passwordbox);
	layout2.addWidget(&save_password);
	layout1.addLayout(&layout2);
	layout1.addWidget(&ok_button);
	dialog.setLayout(&layout1);

	if(dialog.exec()) { //dialog accepted
		password = passwordbox.text();

		if(save_password.isChecked()) {
			kDebug(5011) << "Saving password for host '" << m_host << "'";

			settings.setValue(QString("%1/password").arg(m_host), password);
			settings.sync();
		}

		vncThread.setPassword(password);
	} else {
		startQuitting();
	}
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
	if(!QApplication::focusWidget()) { //no focus, we're probably minimized
		return;
	}

     //kDebug(5011) << "got update" << width() << height();

     /*
     static unsigned int frames = 0;
     static unsigned int updates = 0;
     static QTime time = QTime::currentTime();
     updates++;
     if(updates % 100 == 0)
	     kDebug(5011) << "u/s: " << updates/double(time.elapsed()) * 1000.0;
if(x == 0 and y == 0) {
	frames++;
     if(frames % 100 == 0)
	     kDebug(5011) << "f/s: " << frames/double(time.elapsed()) * 1000.0;
}
*/

    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;


	//with scaled view, artefacts occur because of rounding errors
	//we'll try to only update chunks of screen space that correspond to integer pixel sizes in m_frame
	//put this into paintEvent
	/*
	int frame_x = x/m_horizontalFactor;
	int frame_w = w/m_horizontalFactor;
	int frame_y = y/m_verticalFactor;
	int frame_h = h/m_verticalFactor;
	
	m_x = frame_x*m_horizontalFactor;
	m_y = frame_y*m_verticalFactor;

	m_w = (frame_w+2)*m_horizontalFactor;
	m_h = (frame_h+2)*m_verticalFactor;
	kDebug(5011)<< "update);
	*/


    if (m_horizontalFactor != 1.0 || m_verticalFactor != 1.0) {
        // If the view is scaled, grow the update rectangle to avoid artifacts
        int x_extrapixels = 1.0/m_horizontalFactor + 1;
        int y_extrapixels = 1.0/m_verticalFactor + 1;

        m_x-=x_extrapixels;
        m_y-=y_extrapixels;
        m_w+=2*x_extrapixels;
        m_h+=2*y_extrapixels;
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
        
		resize(width(), height());
        
        m_initDone = true;

    }

	static QSize old_frame_size = QSize();
    if ((y == 0 && x == 0) && (m_frame.size() != old_frame_size)) {
	    old_frame_size = m_frame.size();
        kDebug(5011) << "Updating framebuffer size";
		setZoomLevel();
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
}

//level should be in [0, 100]
void VncView::setZoomLevel(int level)
{
	Q_ASSERT(parentWidget() != 0);

	if(level == -1) { //handle resize
		resize(m_frame.width()*m_horizontalFactor, m_frame.height()*m_verticalFactor);
		return;
	}

	double factor; //actual magnification
	if(level > 95) {
		factor = 2.0;
	} else if(level > 90) {
		factor = 1.0;
	} else {
		const double min_horiz_factor = double(parentWidget()->width())/m_frame.width();
		const double min_vert_factor = double(parentWidget()->height())/m_frame.height();
		const double fit_screen_factor = qMin(min_horiz_factor, min_vert_factor);

		//level=900 => factor=1.0, level=0 => factor=fit_screen_factor
		factor = (level)/90.0*(1.0 - fit_screen_factor) + fit_screen_factor;
	}

	if(factor < 0) {
		//remote display smaller than local?
		kDebug(5011) << "remote display smaller than local?";
		factor = 1.0;
	}
	if(factor != factor) //nan
		factor = 1.0;
	
	kDebug(5011) << "factor" << factor;

	m_verticalFactor = m_horizontalFactor = factor;
	resize(m_frame.width()*factor, m_frame.height()*factor);
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
     //kDebug(5011) << "paint event: x: " << m_x << ", y: " << m_y << ", w: " << m_w << ", h: " << m_h;
    if (m_frame.isNull() || m_frame.format() == QImage::Format_Invalid) {
        kDebug(5011) << "no valid image to paint";
        RemoteView::paintEvent(event);
        return;
    }

    event->accept();

    QPainter painter(this);

	Qt::TransformationMode transformation_mode = Qt::SmoothTransformation;
	if( m_horizontalFactor >= 1.0 )
		transformation_mode = Qt::FastTransformation;

    if (m_repaint and !force_full_repaint) {
//         kDebug(5011) << "normal repaint";
        painter.drawImage(QRect(qRound(m_x*m_horizontalFactor), qRound(m_y*m_verticalFactor),
                                qRound(m_w*m_horizontalFactor), qRound(m_h*m_verticalFactor)), 
                          m_frame.copy(m_x, m_y, m_w, m_h).scaled(qRound(m_w*m_horizontalFactor), 
                                                                  qRound(m_h*m_verticalFactor),
                                                                  Qt::IgnoreAspectRatio, transformation_mode));
    } else {
         //kDebug(5011) << "resize repaint";
        QRect rect = event->rect();
        if (!force_full_repaint and (rect.width() != width() || rect.height() != height())) {
          //   kDebug(5011) << "Partial repaint";
            const int sx = rect.x()/m_horizontalFactor;
            const int sy = rect.y()/m_verticalFactor;
            const int sw = rect.width()/m_horizontalFactor;
            const int sh = rect.height()/m_verticalFactor;
            painter.drawImage(rect, 
                              m_frame.copy(sx, sy, sw, sh).scaled(rect.width(), rect.height(),
                                                                  Qt::IgnoreAspectRatio, transformation_mode));
        } else {
             kDebug(5011) << "Full repaint" << width() << height() << m_frame.width() << m_frame.height();
            painter.drawImage(QRect(0, 0, width(), height()), 
                              m_frame.scaled(m_frame.width() * m_horizontalFactor, m_frame.height() * m_verticalFactor,
                                             Qt::IgnoreAspectRatio, transformation_mode));
			force_full_repaint = false;
        }
    }

	//draw local cursor ourselves, normal mouse pointer doesn't deal with scrolling
	if((m_dotCursorState == CursorOn) || m_forceLocalCursor) {
#if QT_VERSION >= 0x040500
		painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
#endif
		//rectangle size includes 1px pen width
		painter.drawRect(cursor_x*m_horizontalFactor - cursor_size/2, cursor_y*m_verticalFactor - cursor_size/2, cursor_size-1, cursor_size-1);
	}

    RemoteView::paintEvent(event);
}

void VncView::resizeEvent(QResizeEvent *event)
{
    RemoteView::resizeEvent(event);
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
    case QEvent::WindowActivate: //input panel may have been closed, prevent IM from interfering with hardware keyboard
	setAttribute(Qt::WA_InputMethodEnabled, false);
	//fall through
    default:
        return RemoteView::event(event);
    }
}

//call with e == 0 to flush held events
void VncView::mouseEventHandler(QMouseEvent *e)
{
	static bool tap_detected = false;
	static bool double_tap_detected = false;
	static bool tap_drag_detected = false;
	static QTime press_time;
	static QTime up_time; //used for double clicks/tap&drag, for time after first tap

	const int TAP_PRESS_TIME = 180;
	const int DOUBLE_TAP_UP_TIME = 500;

	if(!e) { //flush held taps
		if(tap_detected) {
			m_buttonMask |= 0x01;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			m_buttonMask &= 0xfe;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
			tap_detected = false;
		} else if(double_tap_detected and press_time.elapsed() > TAP_PRESS_TIME) { //got tap + another press -> tap & drag
			m_buttonMask |= 0x01;
			vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
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

	if(!disable_tapping and e->button() == Qt::LeftButton) { //implement touchpad-like input for left button
		if(e->type() == QEvent::MouseButtonPress or e->type() == QEvent::MouseButtonDblClick) {
			press_time.start();
			if(tap_detected and up_time.elapsed() < DOUBLE_TAP_UP_TIME) {
				tap_detected = false;
				double_tap_detected = true;

				QTimer::singleShot(TAP_PRESS_TIME, this, SLOT(mouseEventHandler()));
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
			} else if(press_time.elapsed() < TAP_PRESS_TIME) { //tap
				up_time.start();
				tap_detected = true;
				QTimer::singleShot(DOUBLE_TAP_UP_TIME, this, SLOT(mouseEventHandler()));
			}

		}
	} else { //middle or right button, send directly
		if ((e->type() == QEvent::MouseButtonPress)) {
		    if (e->button() & Qt::MidButton)
			m_buttonMask |= 0x02;
		    if (e->button() & Qt::RightButton)
			m_buttonMask |= 0x04;
		} else if (e->type() == QEvent::MouseButtonRelease) {
		    if (e->button() & Qt::MidButton)
			m_buttonMask &= 0xfd;
		    if (e->button() & Qt::RightButton)
			m_buttonMask &= 0xfb;
		}
		vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	}

	//prevent local cursor artifacts
	static int old_cursor_x = cursor_x;
	static int old_cursor_y = cursor_y;
	if(((m_dotCursorState == CursorOn) || m_forceLocalCursor)
	and (cursor_x != old_cursor_x or cursor_y != old_cursor_y)) {
		//clear last position
		repaint(old_cursor_x*m_horizontalFactor - cursor_size/2, old_cursor_y*m_verticalFactor - cursor_size/2, cursor_size, cursor_size);
		//and refresh new one
		repaint(cursor_x*m_horizontalFactor - cursor_size/2, cursor_y*m_verticalFactor - cursor_size/2, cursor_size, cursor_size);

		old_cursor_x = cursor_x; old_cursor_y = cursor_y;
	}
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

    vncThread.mouseEvent(x, y, eb | m_buttonMask);
    vncThread.mouseEvent(x, y, m_buttonMask);
}

void VncView::keyEventHandler(QKeyEvent *e)
{
    // strip away autorepeating KeyRelease; see bug #206598
    if (e->isAutoRepeat() && (e->type() == QEvent::KeyRelease)) {
        return;
    }

// parts of this code are based on http://italc.sourcearchive.com/documentation/1.0.9.1/vncview_8cpp-source.html
    rfbKeySym k = e->nativeVirtualKey();

    // we do not handle Key_Backtab separately as the Shift-modifier
    // is already enabled
    if (e->key() == Qt::Key_Backtab) {
        k = XK_Tab;
    }

    const bool pressed = (e->type() == QEvent::KeyPress);

#ifdef Q_WS_MAEMO_5
    //don't send ISO_Level3_Shift (would break things like Win+0-9)
    //also enable IM so symbol key works
    if(k == 0xfe03) {
	    setAttribute(Qt::WA_InputMethodEnabled, pressed);
	    e->ignore();
	    return;
    }
#endif

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


	int current_zoom = -1;
	if(e->key() == Qt::Key_F8)
		current_zoom = left_zoom;
	else if(e->key() == Qt::Key_F7)
		current_zoom = right_zoom;
	else if (k) {
	//	kDebug(5011) << "got '" << e->text() << "'.";
		vncThread.keyEvent(k, pressed);
	} else {
		kDebug(5011) << "nativeVirtualKey() for '" << e->text() << "' failed.";
		return;
	}	
	
	if(current_zoom == -1)
		return;

	//handle zoom buttons
	if(current_zoom == 0) { //left click
		if(pressed)
			m_buttonMask |= 0x01;
		else
			m_buttonMask &= 0xfe;
		vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	} else if(current_zoom == 1) { //right click
		if(pressed)
			m_buttonMask |= 0x04;
		else
			m_buttonMask &= 0xfb;
		vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	} else if(current_zoom == 2) { //middle click
		if(pressed)
			m_buttonMask |= 0x02;
		else
			m_buttonMask &= 0xfd;
		vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	} else if(current_zoom == 3 and pressed) { //wheel up
		int eb = 0x8;
		vncThread.mouseEvent(cursor_x, cursor_y, eb | m_buttonMask);
		vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	} else if(current_zoom == 4 and pressed) { //wheel down
		int eb = 0x10;
		vncThread.mouseEvent(cursor_x, cursor_y, eb | m_buttonMask);
		vncThread.mouseEvent(cursor_x, cursor_y, m_buttonMask);
	} else if(current_zoom == 5) { //page up
		vncThread.keyEvent(0xff55, pressed);
	} else if(current_zoom == 6) { //page down
		vncThread.keyEvent(0xff56, pressed);
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
    if (m_status != Connected)
        return;

    if (m_clipboard->ownsSelection() || m_dontSendClipboard)
        return;

    const QString text = m_clipboard->text(QClipboard::Selection);

    vncThread.clientCut(text);
}

void VncView::clipboardDataChanged()
{
    if (m_status != Connected)
        return;

    if (m_clipboard->ownsClipboard() || m_dontSendClipboard)
        return;

    const QString text = m_clipboard->text(QClipboard::Clipboard);

    vncThread.clientCut(text);
}

//fake key events
void VncView::sendKey(Qt::Key key)
{
	//convert Qt::Key into x11 keysym
	int k = 0;
	switch(key) {
	case Qt::Key_Escape:
		k = 0xff1b;
		break;
	case Qt::Key_Tab:
		k = 0xff09;
		break;
	case Qt::Key_PageUp:
		k = 0xff55;
		break;
	case Qt::Key_PageDown:
		k = 0xff56;
		break;
	case Qt::Key_Return:
		k = 0xff0d;
		break;
	case Qt::Key_Insert:
		k = 0xff63;
		break;
	case Qt::Key_Delete:
		k = 0xffff;
		break;
	case Qt::Key_Home:
		k = 0xff50;
		break;
	case Qt::Key_End:
		k = 0xff57;
		break;
	case Qt::Key_Backspace:
		k = 0xff08;
		break;
	case Qt::Key_F1:
	case Qt::Key_F2:
	case Qt::Key_F3:
	case Qt::Key_F4:
	case Qt::Key_F5:
	case Qt::Key_F6:
	case Qt::Key_F7:
	case Qt::Key_F8:
	case Qt::Key_F9:
	case Qt::Key_F10:
	case Qt::Key_F11:
	case Qt::Key_F12:
		k = 0xffbe + int(key - Qt::Key_F1);
		break;
	case Qt::Key_Pause:
		k = 0xff13;
		break;
	case Qt::Key_Print:
		k = 0xff61;
		break;
	case Qt::Key_Menu:
		k = 0xff67;
		break;
	case Qt::Key_Meta:
	case Qt::MetaModifier:
		k = XK_Super_L;
		break;
	case Qt::Key_Alt:
	case Qt::AltModifier:
		k = XK_Alt_L;
		break;
	case Qt::Key_Control:
	case Qt::ControlModifier:
		k = XK_Control_L;
		break;
	default:
		kDebug(5011) << "sendKey(): Unhandled Qt::Key value " << key;
		return;
	}

	if (k == XK_Shift_L || k == XK_Control_L || k == XK_Meta_L || k == XK_Alt_L || k == XK_Super_L) {
		if (m_mods.contains(k)) { //release
			m_mods.remove(k);
			vncThread.keyEvent(k, false);
		} else { //press
			m_mods[k] = true;
			vncThread.keyEvent(k, true);
		}
	} else { //normal key
		vncThread.keyEvent(k, true);
		vncThread.keyEvent(k, false);
	}
}

void VncView::sendKeySequence(QKeySequence keys)
{
	Q_ASSERT(keys.count() <= 1); //we can only handle a single combination

	//to get at individual key presses, we split 'keys' into its components
	QList<int> key_list;
	for(int i = 0; ; i++) {
		QString k = keys.toString().section('+', i, i);
		if(k.isEmpty())
			break;
		//kDebug(5011) << "found key: " << k;
		if(k == "Alt") {
			key_list.append(Qt::Key_Alt);
		} else if(k == "Ctrl") {
			key_list.append(Qt::Key_Control);
		} else if(k == "Meta") {
			key_list.append(Qt::Key_Meta);
		} else {
			key_list.append(QKeySequence(k)[0]);
		}
	}
	
	for(int i = 0; i < key_list.count(); i++)
		sendKey(Qt::Key(key_list.at(i)));

	//release modifiers (everything before final key)
	for(int i = key_list.count()-2; i >= 0; i--)
		sendKey(Qt::Key(key_list.at(i)));
}

void VncView::reloadSettings()
{
	QSettings settings;
	left_zoom = settings.value("left_zoom", 0).toInt();
	right_zoom = settings.value("right_zoom", 1).toInt();
	disable_tapping = settings.value("disable_tapping", false).toBool();

	bool always_show_local_cursor = settings.value("always_show_local_cursor", false).toBool();
	if(always_show_local_cursor)
		showDotCursor(CursorOn);

	enableScaling(true);
}

//convert commitString into keyevents
void VncView::inputMethodEvent(QInputMethodEvent *event)
{
	//TODO handle replacements
	//NOTE for the return key to work Qt needs to enable multiline input, which only works for Q(Plain)TextEdit

	//kDebug(5011) << event->commitString() << "|" << event->preeditString() << "|" << event->replacementLength() << "|" << event->replacementStart();
	QString letters = event->commitString();
	for(int i = 0; i < letters.length(); i++) {
		char k = letters.at(i).toLatin1(); //works with all 'normal' keys, not umlauts.
		if(!k) {
			kDebug(5011) << "unhandled key";
			continue;
		}
		vncThread.keyEvent(k, true);
		vncThread.keyEvent(k, false);
	}
}


#include "moc_vncview.cpp"
