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

#include "keymenu.h"


KeyMenu::KeyMenu(QWidget *parent):
    QDialog(parent)
{
    setWindowTitle(tr("Additional Keys"));
    QTabWidget *tabwidget = new QTabWidget(this);

    //modifiers
    ActionTab *mod_tab = new ActionTab(this);
    win = new QAction(tr("Win"), this);
    win->setShortcut(Qt::META);
    win->setCheckable(true);
    mod_tab->addAction(win);
    alt = new QAction(tr("Alt"), this);
    alt->setShortcut(Qt::ALT);
    alt->setCheckable(true);
    mod_tab->addAction(alt);
    tabwidget->addTab(mod_tab, tr("Modifiers"));

    //movement/text editing keys
    ActionTab *other_tab = new ActionTab(this);
    other_tab->addAction(tr("Insert"), Qt::Key_Insert);
    other_tab->addAction(tr("Delete"), Qt::Key_Delete);
    other_tab->addAction(tr("Backspace"), Qt::Key_Backspace);
    other_tab->addAction(tr("Home"), Qt::Key_Home);
    other_tab->addAction(tr("End"), Qt::Key_End);
    tabwidget->addTab(other_tab, tr("Editing"));

    //F1-F12
    ActionTab *fx_tab = new ActionTab(this);
    for(int i = 1; i<=12; i++)
        fx_tab->addAction(tr("F%1").arg(i), QString("F%1").arg(i));
    tabwidget->addTab(fx_tab, tr("F1-F12"));

    //Misc
    ActionTab *misc_tab = new ActionTab(this);
    misc_tab->addAction(tr("Pause"), QString("Pause"));
    misc_tab->addAction(tr("Print"), QString("print"));
    misc_tab->addAction(tr("Menu"), QString("Menu"));
    misc_tab->addAction(tr("Ctrl+Alt+Del"), QString("Ctrl+Alt+Delete"));
    misc_tab->addAction(tr("Ctrl+Alt+Backspace"), QString("Ctrl+Alt+Backspace"));
    tabwidget->addTab(misc_tab, tr("Misc"));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(tabwidget);
    setLayout(layout);
}

void KeyMenu::accept()
{
    QAction* selected_action = qobject_cast<QAction* >(sender());
    if(!selected_action) {
        keysequence = QKeySequence();
    } else {
        keysequence = selected_action->shortcut();
    }

    QDialog::accept();
}

ActionTab::ActionTab(KeyMenu *parent):
    QScrollArea(parent),
    keymenu(parent)
{
    setWidgetResizable(true);
    QWidget *widget = new QWidget(this);
    setWidget(widget);
    widget->setLayout(&layout);
}

void ActionTab::addAction(QString text, QKeySequence keysequence)
{
    QAction *action = new QAction(text, this);
    action->setShortcut(keysequence);

    addAction(action);
}

void ActionTab::addAction(QAction *action)
{
    connect(action, SIGNAL(triggered()),
            keymenu, SLOT(accept()));

    QToolButton *button = new QToolButton();
    button->setDefaultAction(action);
    layout.addWidget(button);
}
