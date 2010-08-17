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

#include <iostream>


ConnectDialog::ConnectDialog(QWidget *parent):
	QDialog(parent)
{
	setWindowTitle(tr("Connect to VNC Server"));
	QSettings settings;

	int i = 0;
	for(;;) { //read history
		QString hostname = settings.value(QString("host%1").arg(i), "").toString();
		if(hostname.isEmpty())
			break;

		hosts.addItem(hostname);
		i++;
	}
	hosts.setEditable(true);
	hosts.lineEdit()->setInputMethodHints(Qt::ImhLowercaseOnly); //doesn't work, but I tried.
	connect(&hosts, SIGNAL(editTextChanged(QString)),
		this, SLOT(convertToLowercase(QString)));
	layout.addWidget(&hosts);

	QPushButton *done = new QPushButton(tr("Done"));
	done->setMaximumWidth(100);
	connect(done, SIGNAL(clicked()),
		this, SLOT(accept()));
	layout.addWidget(done);

	setLayout(&layout);
}

void ConnectDialog::convertToLowercase(QString newtext)
{
	hosts.lineEdit()->setText(newtext.toLower());
}

void ConnectDialog::accept()
{
	QDialog::accept();

	if(hosts.currentText().isEmpty()) {
		deleteLater();
		return;
	}

	//save url?
	QSettings settings;
	bool new_item = hosts.itemText(hosts.currentIndex()) != hosts.currentText();
	bool used_old_host = !new_item and hosts.currentIndex() > 0;
	int rearrange_from_idx;
	if(new_item) {
		std::cout << "adding new item to history\n";
		int i = 0;
		for(;;) { //find first unused key
			QString hostname = settings.value(QString("host%1").arg(i), "").toString();
			if(hostname.isEmpty())
				break;
			i++;
		}
		rearrange_from_idx = i;
	} else if(used_old_host) {
		rearrange_from_idx = hosts.currentIndex();
	}

	if(new_item or used_old_host) {
		std::cout << "rearranging history,  last index " << rearrange_from_idx << "\n";

		for(int i = rearrange_from_idx-1; i >= 0; i--) { //increment index for each entry newer than the selected one
			QString hostname = settings.value(QString("host%1").arg(i), "").toString();
			settings.setValue(QString("host%1").arg(i+1), hostname);
		}
		settings.setValue(QString("host0"), hosts.currentText());
	}

	emit connectToHost(QString("vnc://") + hosts.currentText());
	deleteLater();
}
