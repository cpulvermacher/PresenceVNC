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

Preferences::Preferences(QWidget *parent):
	QDialog(parent)
{
	setWindowTitle("Preferences");

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
	key_model->appendRow(new QStandardItem(tr("Left Click"))); //0
	key_model->appendRow(new QStandardItem(tr("Right Click")));//1
	key_model->appendRow(new QStandardItem(tr("Wheel Up")));//2
	key_model->appendRow(new QStandardItem(tr("Wheel Down")));//3
	key_model->appendRow(new QStandardItem(tr("Page Up")));//4
	key_model->appendRow(new QStandardItem(tr("Page Down")));//5
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

	QPushButton *ok = new QPushButton("OK");

	layout1->addLayout(layout2);
	layout1->addWidget(ok);

	setLayout(layout1);

	connect(ok, SIGNAL(clicked()),
		this, SLOT(accept()));
	connect(this, SIGNAL(accepted()),
		this, SLOT(save()));
}

Preferences::~Preferences()
{

}


void Preferences::save()
{
	settings.setValue("screen_rotation", rotation_selector->currentIndex());
	//TODO: save zoom stuf

	settings.sync();
}
