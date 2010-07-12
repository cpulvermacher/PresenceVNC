/****************************************************************************
**
** Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#include <QApplication>
#include <QInputDialog>
#include <QString>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Presence VNC");
	QCoreApplication::setApplicationName("Presence VNC");

	QApplication app(argc, argv);

	QString url;
	int quality = 2;
	QStringList arguments = QCoreApplication::arguments();
	if (arguments.count() > 1) {
		//Example: vnc://:password@server:1\n"
		//"Optionally, you can define the quality as second argument (1-3, where 1 is the best). Default is 2.");

		url = arguments.at(1);
		if(arguments.count() > 2)
			quality = arguments.at(2).toInt();
	}
	MainWindow main(url, quality);
	main.show();
	return app.exec();
}
