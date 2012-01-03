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
#include "mainwindow.h"
#include "rfb/rfbclient.h"

#include <QApplication>
#include <QString>

#include <iostream>


const QString APPNAME("Presence VNC");

void printHelp() {
    std::cout << "Usage: " << qPrintable(QCoreApplication::arguments().at(0)) << " [options] [URL [quality]]\n"

        << "\nOptions:\n"
        << " --help\t\t\t Print this text and exit.\n"
        << " --listen [port]\t Listen for incoming connections on given port, or 5500 if omitted. Cannot be used with an URL.\n"
        << " --viewonly\t\t Don't send mouse/keyboard input to remote desktop. This is only useful if you also supply a URL, or --listen.\n"

        << "\nURLs:\n"
        << " vnc://:password@server:display\n\n"
        << " Password and display can be omitted, e.g. vnc://server is a valid URL.\n"
        << " Optionally, you can define the quality as a second argument (1-3, where 1 is the best). Default is 2.\n";
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(APPNAME);
    QCoreApplication::setApplicationName(APPNAME);

    QApplication app(argc, argv);

    QString url;
    int quality = 2;
    int listen_port = 0; //only used if >0
    bool view_only = false;

    QStringList arguments = QCoreApplication::arguments();
    for(int i = 1; i < arguments.count(); i++) {
        if(arguments.at(i) == "--help") {
            printHelp();

            return 0;
        } else if(arguments.at(i) == "--listen") {
            if(arguments.count() <= i+1 or arguments.at(i+1).startsWith("--")) {
                //no argument found, use default
                listen_port = LISTEN_PORT_OFFSET;
            } else {
                //next argument should be a port number
                listen_port = arguments.at(i+1).toInt();
                if(listen_port < 1 or listen_port > 65535) {
                    std::cerr << "\"" << qPrintable(arguments.at(i+1)) << "\" is not a valid port number!\n\n";
                    return 1;
                }

                //everything ok
                i++;
            }
        } else if(arguments.at(i) == "--viewonly") {
            view_only = true;
        } else { //not a valid command line option, should be the url
            url = arguments.at(i);

            //check url
            if(!url.startsWith("vnc://")) {
                std::cerr << "\"" << qPrintable(url) << "\" is not a valid command line option!\n\n";
                printHelp();

                return 1;
            }

            if(arguments.count() > i+1) { //TODO: having a --quality option would make more sense.
                int arg = arguments.at(i+1).toInt();
                if(1 <= arg and arg <= 3) { //check if arg is valid, might also be another option
                    quality = arg;
                    i++;
                }
            }
        }
    }

    if(!url.isNull() and listen_port > 0) {
        std::cerr << "--listen cannot be used with an URL!\n";
        return 1;
    }

    MainWindow main(url, quality, listen_port, view_only);
    main.show();
    return app.exec();
}
