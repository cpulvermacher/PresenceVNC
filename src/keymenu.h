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

#ifndef KEYMENU_H
#define KEYMENU_H

#include <QtGui>


class KeyMenu : public QDialog
{
    Q_OBJECT
public:
    KeyMenu(QWidget *parent);
    QKeySequence getKeySequence() { return keysequence; }
public slots:
    virtual void accept();
    virtual int exec() { keysequence = QKeySequence(); return QDialog::exec(); }
private:
    QKeySequence keysequence;
};


class ActionTab : public QScrollArea
{
    Q_OBJECT
public:
    ActionTab(KeyMenu *parent);
    void addAction(QAction *action); //adds action and connects it to keymenu->accept()
    void addAction(QString text, QKeySequence keysequence);
private:
    QWidget widget;
    QHBoxLayout layout;
    KeyMenu *keymenu;
};
#endif
