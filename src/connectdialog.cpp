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

#ifdef Q_WS_MAEMO_5
#include <QMaemo5ValueButton>
#include <QMaemo5ListPickSelector>
#endif

#include "connectdialog.h"

#include <iostream>


ConnectDialog::ConnectDialog(QWidget *parent):
	QDialog(parent)
{
	setWindowTitle(tr("Connect to VNC Server"));
	QSettings settings;

	//read history
	settings.beginGroup("hosts");
	QStringList hostnames = settings.childGroups();
	QStringList hostnames_sorted = hostnames;
	foreach(QString hostname, hostnames) {
		if(!settings.contains(hostname + "/position")) {
			//can happen when host was given as a command line argument, don't show those
			hostnames_sorted.removeAll(hostname);
			continue;
		}

		int position = settings.value(hostname + "/position").toInt();
		if(position < 0)
			position = 0;
		else if(position >= hostnames_sorted.size())
			position = hostnames_sorted.size()-1;

		hostnames_sorted.replace(position, hostname);
	}
	settings.endGroup();

	//set up combobox
	hosts.addItems(hostnames_sorted);
	hosts.setEditable(true);
#ifdef Q_WS_MAEMO_5
	hosts.lineEdit()->setInputMethodHints(Qt::ImhNoAutoUppercase); //somehow this doesn't work that well here
#endif
	connect(&hosts, SIGNAL(editTextChanged(QString)),
		this, SLOT(hostnameUpdated(QString)));
	layout.addWidget(&hosts);

#ifdef Q_WS_MAEMO_5
	QMaemo5ValueButton *quality = new QMaemo5ValueButton(tr("Quality"), this);
	quality_selector = new QMaemo5ListPickSelector(this);
	QStandardItemModel *model = new QStandardItemModel(0, 1, this);
	model->appendRow(new QStandardItem(tr("High\t\t(LAN)"))); //1
	model->appendRow(new QStandardItem(tr("Medium\t(DSL)"))); //2
	model->appendRow(new QStandardItem(tr("Low\t\t(ISDN)"))); //3
	quality_selector->setModel(model);
	quality->setPickSelector(quality_selector);
	quality->setValueLayout(QMaemo5ValueButton::ValueUnderText);
	quality->setMaximumWidth(120);
	layout.addWidget(quality);

	hostnameUpdated(hosts.lineEdit()->text()); //get saved quality for last host, or 2
#endif

	QPushButton *done = new QPushButton(tr("Connect"));
	done->setMaximumWidth(110);
	connect(done, SIGNAL(clicked()),
		this, SLOT(accept()));
	layout.addWidget(done);

	setLayout(&layout);
}

void ConnectDialog::hostnameUpdated(QString newtext)
{
	//clean up hostname
	newtext.remove(QChar('/'));
	newtext.remove(QChar('\\'));
	hosts.lineEdit()->setText(newtext.toLower());

	//saved quality setting available?
	QSettings settings;
	int quality = settings.value(QString("hosts/%1/quality").arg(hosts.lineEdit()->text()), 2).toInt();
	if(quality < 1 or quality > 3)
		quality = 2;
	quality_selector->setCurrentIndex(quality-1);
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
	settings.beginGroup("hosts");
	bool new_item = !settings.contains(hosts.currentText());
	bool used_old_host = !new_item and hosts.currentIndex() > 0;
	int rearrange_up_to_pos;
	if(new_item) {
		std::cout << "adding new item to history\n";
		rearrange_up_to_pos = hosts.count(); //use free index
	} else if(used_old_host) {
		rearrange_up_to_pos = hosts.currentIndex();
	}

	if(new_item or used_old_host) {
		std::cout << "rearranging history,  last index " << rearrange_up_to_pos << "\n";

		QStringList hostnames = settings.childGroups();
		foreach(QString hostname, hostnames) {
			if(!settings.contains(hostname + "/position"))
				continue; //ignore entries without position

			int position = settings.value(hostname + "/position").toInt();
			if(position < rearrange_up_to_pos)
				settings.setValue(hostname + "/position", position+1);
		}
		//position 0 is now free

		//move selected host to front
		settings.setValue(QString("%1/position").arg(hosts.currentText()), 0);
	}
	int quality = quality_selector->currentIndex() + 1;
	settings.setValue(QString("%1/quality").arg(hosts.currentText()), quality);

	settings.endGroup();
	settings.sync();

	emit connectToHost(QString("vnc://%1").arg(hosts.currentText()), quality);
	deleteLater();
}
