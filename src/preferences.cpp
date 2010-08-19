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
#include <QMaemo5ValueButton>
#include <QMaemo5ListPickSelector>

#include "preferences.h"

#include <iostream>

void migrateConfiguration()
{
	QSettings settings;
	int config_ver = settings.value("config_version", 0).toInt();
	if(config_ver == 2) //config file up-to-date
		return;

	std::cout << "Migrating from configuration ver. " << config_ver << "\n";

	if(config_ver == 0) {
		int left_zoom = settings.value("left_zoom", 0).toInt();
		int right_zoom = settings.value("left_zoom", 0).toInt();
		if(left_zoom >= 2)
			settings.setValue("left_zoom", left_zoom+1);
		if(right_zoom >= 2)
			settings.setValue("left_zoom", right_zoom+1);
		config_ver = 1;
	}
	if(config_ver == 1) {
		QString last_hostname = settings.value("last_hostname", "").toString();
		settings.remove("last_hostname");
		if(!last_hostname.isEmpty()) {
			//make sure hostname is sane
			last_hostname.remove(QChar('/'));
			last_hostname.remove(QChar('\\'));
			last_hostname = last_hostname.toLower();

			settings.setValue(QString("hosts/%1/position").arg(last_hostname), 0);
		}
		
		config_ver = 2;
	}
	settings.setValue("config_version", config_ver);
	settings.sync();
}


Preferences::Preferences(QWidget *parent):
	QDialog(parent)
{
	setWindowTitle(tr("Preferences"));

	QHBoxLayout *layout1 = new QHBoxLayout();
	QVBoxLayout *layout2 = new QVBoxLayout();

	QMaemo5ValueButton *rotation = new QMaemo5ValueButton(tr("Screen Rotation"), this);
	rotation_selector = new QMaemo5ListPickSelector(this);
	QStandardItemModel *model = new QStandardItemModel(0, 1, this);
	model->appendRow(new QStandardItem(tr("Automatic")));
	model->appendRow(new QStandardItem(tr("Landscape")));
	model->appendRow(new QStandardItem(tr("Portrait")));
	rotation_selector->setModel(model);
	rotation_selector->setCurrentIndex(settings.value("screen_rotation", 0).toInt());
	rotation->setPickSelector(rotation_selector);
	rotation->setValueLayout(QMaemo5ValueButton::ValueBesideText);
	layout2->addWidget(rotation);

	QMaemo5ValueButton *leftzoom = new QMaemo5ValueButton(tr("Left Zoom Button"), this);
	leftzoom_selector = new QMaemo5ListPickSelector(this);
	QStandardItemModel *key_model = new QStandardItemModel(0, 1, this);
	key_model->insertRow(0, new QStandardItem(tr("Left Click")));
	key_model->insertRow(1, new QStandardItem(tr("Right Click")));
	key_model->insertRow(2, new QStandardItem(tr("Middle Click")));
	key_model->insertRow(3, new QStandardItem(tr("Wheel Up")));
	key_model->insertRow(4, new QStandardItem(tr("Wheel Down")));
	key_model->insertRow(5, new QStandardItem(tr("Page Up")));
	key_model->insertRow(6, new QStandardItem(tr("Page Down")));
	leftzoom_selector->setModel(key_model);
	leftzoom_selector->setCurrentIndex(settings.value("left_zoom", 0).toInt());
	leftzoom->setPickSelector(leftzoom_selector);
	leftzoom->setValueLayout(QMaemo5ValueButton::ValueBesideText);
	layout2->addWidget(leftzoom);

	QMaemo5ValueButton *rightzoom = new QMaemo5ValueButton(tr("Right Zoom Button"), this);
	rightzoom_selector = new QMaemo5ListPickSelector(this);
	rightzoom_selector->setModel(key_model);
	rightzoom_selector->setCurrentIndex(settings.value("right_zoom", 1).toInt());
	rightzoom->setPickSelector(rightzoom_selector);
	rightzoom->setValueLayout(QMaemo5ValueButton::ValueBesideText);
	layout2->addWidget(rightzoom);

	disable_tapping = new QCheckBox(tr("Disable Tapping"), this);
	disable_tapping->setChecked(settings.value("disable_tapping", false).toBool());
	layout2->addWidget(disable_tapping);

	QPushButton *ok = new QPushButton(tr("Done"));
	ok->setMaximumWidth(100);

	layout1->addLayout(layout2);
	layout1->addWidget(ok);

	setLayout(layout1);

	connect(ok, SIGNAL(clicked()),
		this, SLOT(accept()));
	connect(this, SIGNAL(accepted()),
		this, SLOT(save()));
}

Preferences::~Preferences() { }

void Preferences::save()
{
	settings.setValue("screen_rotation", rotation_selector->currentIndex());
	settings.setValue("left_zoom", leftzoom_selector->currentIndex());
	settings.setValue("right_zoom", rightzoom_selector->currentIndex());
	settings.setValue("disable_tapping", disable_tapping->isChecked());

	settings.sync();
}
