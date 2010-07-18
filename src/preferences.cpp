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
	//rotation->setValueText(settings.value("sound_filename", SOUND_FILE).toString());
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

	settings.sync();
}
