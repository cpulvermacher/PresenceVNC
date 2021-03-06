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

#include "vncclientthread.h"

#include <QMutexLocker>
#include <QTimer>

static QString outputErrorMessageString;

#ifdef Q_WS_MAEMO_5
//N900 display has 16bit depth (R/G/B with 5/6/5 bpp)
const int MAX_COLOR_DEPTH = 16;
#else
const int MAX_COLOR_DEPTH = 32;
#endif


rfbBool VncClientThread::newclient(rfbClient *cl)
{
    VncClientThread *t = static_cast<VncClientThread*>(rfbClientGetClientData(cl, 0));
    Q_ASSERT(t);

    switch (t->quality()) {
    case RemoteView::High:
        cl->format.bitsPerPixel = MAX_COLOR_DEPTH;
        cl->appData.useBGR233 = 0;
        cl->appData.encodingsString = "copyrect hextile raw";
        cl->appData.compressLevel = 0;
        cl->appData.qualityLevel = 9;
        break;
    case RemoteView::Medium:
        cl->format.bitsPerPixel = 16;
        cl->appData.useBGR233 = 0;
        cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
        cl->appData.compressLevel = 5;
        cl->appData.qualityLevel = 7;
        break;
    case RemoteView::Low:
    case RemoteView::Unknown:
    default:
        cl->format.bitsPerPixel = 16; //TODO: add support for 8bit (needs color map)
        cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
        cl->appData.compressLevel = 9;
        cl->appData.qualityLevel = 1;
    }

    if(cl->format.bitsPerPixel == 16) {
        cl->format.depth = 16; //number of useful bits in the pixel value
        cl->format.redShift = 11;
        cl->format.greenShift = 5;
        cl->format.blueShift = 0;
        cl->format.redMax = 0x1f;
        cl->format.greenMax = 0x3f;
        cl->format.blueMax = 0x1f;
    } else {
        cl->format.depth = 24; //number of useful bits in the pixel value
        cl->format.redShift = 16;
        cl->format.greenShift = 8;
        cl->format.blueShift = 0;
        cl->format.redMax = 0xff;
        cl->format.greenMax = 0xff;
        cl->format.blueMax = 0xff;
    }

    delete [] t->m_frameBuffer; // do not leak if we get a new framebuffer size
    const int size = cl->width * cl->height * (cl->format.bitsPerPixel / 8);
    t->m_frameBuffer = new uint8_t[size];
    cl->frameBuffer = t->m_frameBuffer;
    memset(cl->frameBuffer, '\0', size);


    SetFormatAndEncodings(cl);

    return true;
}

void VncClientThread::updatefb(rfbClient* cl, int x, int y, int w, int h)
{
    //kDebug(5011) << "updated client: x: " << x << ", y: " << y << ", w: " << w << ", h: " << h;

    const QImage img(
            cl->frameBuffer,
            cl->width,
            cl->height,
            (cl->format.bitsPerPixel==16)?QImage::Format_RGB16:QImage::Format_RGB32
    );

    if (img.isNull()) {
        kDebug(5011) << "image not loaded";
    }

    VncClientThread *t = static_cast<VncClientThread*>(rfbClientGetClientData(cl, 0));
    Q_ASSERT(t);

    t->setImage(img);

    t->emitUpdated(x, y, w, h);
}

void VncClientThread::cuttext(rfbClient* cl, const char *text, int textlen)
{
    const QString cutText = QString::fromUtf8(text, textlen);
    kDebug(5011) << "cuttext: " << cutText;

    if (!cutText.isEmpty()) {
        VncClientThread *t = static_cast<VncClientThread*>(rfbClientGetClientData(cl, 0));
        Q_ASSERT(t);

        t->emitGotCut(cutText);
    }
}

char *VncClientThread::passwdHandler(rfbClient *cl)
{
    kDebug(5011) << "password request" << kBacktrace();

    VncClientThread *t = static_cast<VncClientThread*>(rfbClientGetClientData(cl, 0));
    Q_ASSERT(t);

    t->m_passwordError = true;
    t->passwordRequest();

    return strdup(t->password().toLocal8Bit());
}

void VncClientThread::setPassword(const QString &password)
{
    if(password.isNull()) //cancelled, don't retry
        m_passwordError = false;

    m_password = password;
}

void VncClientThread::outputHandler(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    QString message;
    message.vsprintf(format, args);

    va_end(args);

    message = message.trimmed();

    kDebug(5011) << message;

    if ((message.contains("Couldn't convert ")) ||
            (message.contains("Unable to connect to VNC server")))
        outputErrorMessageString = i18n("Server not found.");
    else if ((message.contains("VNC connection failed: Authentication failed, too many tries")) ||
            (message.contains("VNC connection failed: Too many authentication failures")))
        outputErrorMessageString = i18n("VNC authentication failed because of too many authentication tries.");
    else if (message.contains("VNC connection failed: Authentication failed"))
        outputErrorMessageString = i18n("VNC authentication failed.");
    else if (message.contains("VNC server closed connection"))
        outputErrorMessageString = i18n("VNC server closed connection.");
    else if (message.contains("VNC server supports protocol version 3.889")) // see http://bugs.kde.org/162640
        outputErrorMessageString = "INTERNAL:APPLE_VNC_COMPATIBILTY"; // internal messages, not displayed to user
}

VncClientThread::VncClientThread(QObject *parent)
    : QThread(parent)
    , m_frameBuffer(0)
{
    QMutexLocker locker(&m_mutex);
    m_stopped = false;

    QTimer *outputErrorMessagesCheckTimer = new QTimer(this);
    outputErrorMessagesCheckTimer->setInterval(500);
    connect(outputErrorMessagesCheckTimer, SIGNAL(timeout()), this, SLOT(checkOutputErrorMessage()));
    outputErrorMessagesCheckTimer->start();
}

VncClientThread::~VncClientThread()
{
    stop();

    const bool quitSuccess = wait(4000);

    if(!quitSuccess)
        kDebug(5011) << "~VncClientThread(): Quit failed";

    delete [] m_frameBuffer;
    //m_cl is free()d when event loop exits.
}

void VncClientThread::checkOutputErrorMessage()
{
    if (!outputErrorMessageString.isEmpty()) {
        QString errorMessage = outputErrorMessageString;
        outputErrorMessageString.clear();
        // show authentication failure error only after the 3rd unsuccessful try
        if ((errorMessage != i18n("VNC authentication failed.")) || m_passwordError)
            emit outputErrorMessage(errorMessage);
    }
}

void VncClientThread::setHost(const QString &host)
{
    QMutexLocker locker(&m_mutex);
    m_host = host;
}

void VncClientThread::setPort(int port)
{
    QMutexLocker locker(&m_mutex);
    m_port = port;
}

void VncClientThread::setQuality(RemoteView::Quality quality)
{
    m_quality = quality;
}

RemoteView::Quality VncClientThread::quality() const
{
    return m_quality;
}

void VncClientThread::setImage(const QImage &img)
{
    QMutexLocker locker(&m_mutex);
    m_image = img;
}

const QImage VncClientThread::image(int x, int y, int w, int h)
{
    QMutexLocker locker(&m_mutex);

    if (w == 0) // full image requested
        return m_image;
    else
        return m_image.copy(x, y, w, h);
}

void VncClientThread::emitUpdated(int x, int y, int w, int h)
{
    emit imageUpdated(x, y, w, h);
}

void VncClientThread::emitGotCut(const QString &text)
{
    emit gotCut(text);
}

void VncClientThread::stop()
{
    if(m_stopped)
        return;

    //also abort listening for connections, should be safe without locking
    if(m_listen_port)
        m_cl->listenSpecified = false;

    QMutexLocker locker(&m_mutex);
    m_stopped = true;
}

void VncClientThread::run()
{
    QMutexLocker locker(&m_mutex);

    int passwd_failures = 0;
    while (!m_stopped) { // try to connect as long as the server allows
        m_passwordError = false;
        outputErrorMessageString.clear(); //don't deliver error messages of old instances...

        rfbClientLog = outputHandler;
        rfbClientErr = outputHandler;
        m_cl = rfbGetClient(8, 3, 4); // bitsPerSample, samplesPerPixel, bytesPerPixel
        m_cl->MallocFrameBuffer = newclient;
        m_cl->canHandleNewFBSize = true;
        m_cl->GetPassword = passwdHandler;
        m_cl->GotFrameBufferUpdate = updatefb;
        m_cl->GotXCutText = cuttext;
        rfbClientSetClientData(m_cl, 0, this);

        m_cl->serverHost = strdup(m_host.toUtf8().constData());

        if (m_port < 0 || !m_port) // port is invalid or empty...
            m_port = 5900; // fallback: try an often used VNC port

        if (m_port >= 0 && m_port < 100) // the user most likely used the short form (e.g. :1)
            m_port += 5900;
        m_cl->serverPort = m_port;

        m_cl->listenSpecified = rfbBool(m_listen_port > 0);
        m_cl->listenPort = m_listen_port;

        kDebug(5011) << "--------------------- trying init ---------------------";

        if (rfbInitClient(m_cl, 0, 0))
            break;

        //init failed...
        if (m_passwordError) {
            passwd_failures++;
            if(passwd_failures < 3)
                continue; //that's ok, try again
        }

        //stop connecting
        m_stopped = true;
        return; //no cleanup necessary, m_cl was free()d by rfbInitClient()
    }

    locker.unlock();

    // Main VNC event loop
    while (!m_stopped) {
        const int i = WaitForMessage(m_cl, 500);
        if(m_stopped or i < 0)
            break;

        if (i)
            if (!HandleRFBServerMessage(m_cl))
                break;

        locker.relock();

        while (!m_eventQueue.isEmpty()) {
            ClientEvent* clientEvent = m_eventQueue.dequeue();
            clientEvent->fire(m_cl);
            delete clientEvent;
        }

        locker.unlock();
    }

    // Cleanup allocated resources
    locker.relock();
    rfbClientCleanup(m_cl);
    m_stopped = true;
}

ClientEvent::~ClientEvent()
{
}

void PointerClientEvent::fire(rfbClient* cl)
{
    SendPointerEvent(cl, m_x, m_y, m_buttonMask);
}

void KeyClientEvent::fire(rfbClient* cl)
{
    SendKeyEvent(cl, m_key, m_pressed);
}

void ClientCutEvent::fire(rfbClient* cl)
{
    SendClientCutText(cl, text.toUtf8().data(), text.size());
}

void VncClientThread::mouseEvent(int x, int y, int buttonMask)
{
    QMutexLocker lock(&m_mutex);
    if (m_stopped)
        return;

    m_eventQueue.enqueue(new PointerClientEvent(x, y, buttonMask));
}

void VncClientThread::keyEvent(int key, bool pressed)
{
    QMutexLocker lock(&m_mutex);
    if (m_stopped)
        return;

    m_eventQueue.enqueue(new KeyClientEvent(key, pressed));
}

void VncClientThread::clientCut(const QString &text)
{
    QMutexLocker lock(&m_mutex);
    if (m_stopped)
        return;

    m_eventQueue.enqueue(new ClientCutEvent(text));
}
