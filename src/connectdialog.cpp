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

#include "connectdialog.h"
#include "rfb/rfbclient.h"

#include <QtGui>

#ifdef Q_WS_MAEMO_5
#include <QMaemo5ValueButton>
#include <QMaemo5ListPickSelector>
#endif


const QString LISTEN_FOR_INCOMING_CONNECTIONS_STRING = QObject::tr("Listen for incoming connections");

ConnectDialog::ConnectDialog(QWidget *parent):
	QDialog(parent),
	done(new QPushButton)
{
	setWindowTitle(tr("Connect to VNC Server"));
	QSettings settings;

	//read history
	settings.beginGroup("hosts");
	QStringList hostnames = settings.childGroups();
	QMap<int, QString> hosts_map; //use position as key
	foreach(QString hostname, hostnames) {
		if(!settings.contains(hostname + "/position")) {
			continue; //can happen when host was given as a command line argument, don't show those
		}

		int position = settings.value(hostname + "/position").toInt();
		hosts_map.insert(position, hostname);
	}
	hostnames_sorted = hosts_map.values(); //sorted by ascending position
	settings.endGroup();

	//set up combobox
	hosts.addItems(hostnames_sorted);
	hosts.insertSeparator(hosts.count());
	hosts.addItem(QIcon("/usr/share/icons/hicolor/48x48/hildon/general_received.png"), LISTEN_FOR_INCOMING_CONNECTIONS_STRING);
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
#else
	//combobox numbering starts from 0, so currentIndex() == quality-1
	quality_combobox.addItem(tr("High quality (LAN)")); //1
	quality_combobox.addItem(tr("Medium quality (DSL)")); //2
	quality_combobox.addItem(tr("Low quality (ISDN)")); //3
	//quality_combobox.setMaximumWidth(120);
	layout.addWidget(&quality_combobox);
#endif

	hostnameUpdated(hosts.lineEdit()->text()); //get saved quality for last host, or 2

	done->setText(tr("Connect"));
	done->setMaximumWidth(110);
	connect(done, SIGNAL(clicked()),
		this, SLOT(accept()));
	layout.addWidget(done);

	setLayout(&layout);

	connect(this, SIGNAL(finished(int)),
		this, SLOT(deleteLater()));
}

void ConnectDialog::hostnameUpdated(QString newtext)
{
	const int cursorpos = hosts.lineEdit()->cursorPosition();

	const bool normal_entry = hosts.itemIcon(hosts.currentIndex()).isNull();
	done->setText(normal_entry ? tr("Connect") : tr("Listen"));

	//unselect 'listen ...' entry if edited
	if(!normal_entry) {
		if(newtext != LISTEN_FOR_INCOMING_CONNECTIONS_STRING) {
			hosts.setCurrentIndex(-1);
		} else { 
			return;
		}
	}

	//clean up hostname (we don't want / or \ in saved hostnames)
	newtext.remove(QChar('/'));
	newtext.remove(QChar('\\'));
	hosts.lineEdit()->setText(newtext);
	hosts.lineEdit()->setCursorPosition(cursorpos);

	//saved quality setting available?
	QSettings settings;
	int quality = settings.value(QString("hosts/%1/quality").arg(hosts.lineEdit()->text()), 2).toInt();
	if(quality < 1 or quality > 3)
		quality = 2;
#ifdef Q_WS_MAEMO_5
	quality_selector->setCurrentIndex(quality-1);
#else
	quality_combobox.setCurrentIndex(quality-1);
#endif
}

void ConnectDialog::accept()
{
	QDialog::accept();

	QString selected_host = hosts.currentText();
	if(selected_host.isEmpty()) {
		return;
	}

#ifdef Q_WS_MAEMO_5
	int quality = quality_selector->currentIndex() + 1;
#else
	int quality = quality_combobox.currentIndex() + 1;
#endif

	QSettings settings;
	if(!hosts.itemIcon(hosts.currentIndex()).isNull()) {
		int listen_port = settings.value("listen_port", LISTEN_PORT_OFFSET).toInt();

#if QT_VERSION >= 0x040500
		//ask user if listen_port is correct
		bool ok;
		listen_port = QInputDialog::getInt(this,
			tr("Listen for Incoming Connections"),
			tr("Listen on Port:"),
			listen_port, 1, 65535, //value, min, max
			1, &ok);
#else
		bool ok = true;
#endif
		if(ok) {
			settings.setValue("listen_port", listen_port);
			emit connectToHost("", quality, listen_port);
		}
		return;
	}

	settings.beginGroup("hosts");
	const bool new_item = !hostnames_sorted.contains(selected_host);
	const bool used_old_host = !new_item and hosts.currentIndex() > 0;
	//if both are false, we don't need to mess with positions

	if(new_item or used_old_host) {
		//put selected_host at the top
		settings.setValue(QString("%1/position").arg(selected_host), 0);

		//don't create duplicates
		if(used_old_host)
			hostnames_sorted.removeAll(selected_host);

		//now rebuild list for positions >= 1
		for(int i = 0; i < hostnames_sorted.size(); i++)
			settings.setValue(QString("%1/position").arg(hostnames_sorted.at(i)), i+1);
	}

#ifdef Q_WS_MAEMO_5
	settings.setValue(QString("%1/quality").arg(selected_host), quality);
#endif

	settings.endGroup();
	settings.sync();

	emit connectToHost(QString("vnc://%1").arg(selected_host), quality, 0);
}
