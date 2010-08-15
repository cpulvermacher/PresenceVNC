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
#include <QtGui>

#include "connectdialog.h"


ConnectDialog::ConnectDialog(QWidget *parent):
	QDialog(parent)
{
	setWindowTitle("Connect to Host");
	QSettings settings;

	QHBoxLayout *layout1 = new QHBoxLayout();
	QVBoxLayout *layout2 = new QVBoxLayout();

	hostname = new QLineEdit(settings.value("last_hostname", "").toString(), this);
	hostname->setInputMethodHints(Qt::ImhLowercaseOnly); //doesn't work, but I tried.
	layout2->addWidget(hostname);

	QPushButton *ok = new QPushButton("Done");
	ok->setMaximumWidth(100);

	layout1->addLayout(layout2);
	layout1->addWidget(ok);

	setLayout(layout1);

	connect(ok, SIGNAL(clicked()),
		this, SLOT(accept()));
	connect(hostname, SIGNAL(textEdited()),
		this, SLOT(convertToLowercase()));
}

QString ConnectDialog::getUrl()
{
	QSettings settings;
	settings.setValue("last_hostname", hostname->text());
	settings.sync();

	return QString("vnc://") + hostname->text();
}

void ConnectDialog::convertToLowercase()
{
	hostname->setText(hostname->text().toLower());
}
