// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2000-2006 Licq developers
 *
 * Licq is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Licq is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Licq; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mmuserview.h"

#include "config.h"

#include <ctype.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>

#include <licq_icqd.h>
#include <licq_user.h>
extern char* PPIDSTRING(unsigned long);

#include "config/contactlist.h"

#include "contactlist/contactlist.h"
#include "contactlist/multicontactproxy.h"

#include "core/gui-defines.h"
#include "core/messagebox.h"

#include "helpers/usercodec.h"

using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::MMUserView */

MMUserView::MMUserView(QString id, unsigned long ppid, ContactListModel* contactList, QWidget* parent)
  : UserViewBase(contactList, parent),
    myId(id),
    myPpid(ppid)
{
  // Use a proxy model for sorting and filtering
  myListProxy = new MultiContactProxy(myContactList, this);
  setModel(myListProxy);
  setRootIndex(dynamic_cast<MultiContactProxy*>(myListProxy)->rootIndex());

  // Setup popup menu
  myMenu = new QMenu();
  myMenu->addAction(tr("Remove"), this, SLOT(remove()));
  myMenu->addAction(tr("Crop"), this, SLOT(crop()));
  myMenu->addAction(tr("Clear"), this, SLOT(clear()));
  myMenu->addSeparator();
  myMenu->addAction(tr("Add Group"), this, SLOT(addCurrentGroup()));
  myMenu->addAction(tr("Add All"), this, SLOT(addAll()));

  setSelectionMode(ExtendedSelection);
  dynamic_cast<SortedContactListProxy*>(myListProxy)->sort(0);
  header()->setVisible(Config::ContactList::instance()->showHeader());

  for (unsigned short i = 0; i < Config::ContactList::instance()->columnCount(); i++)
    setColumnWidth(i, Config::ContactList::instance()->columnWidth(i));
}

MMUserView::~MMUserView()
{
  // Empty
}

void MMUserView::add(QString id, unsigned long ppid)
{
  if (id == myId && ppid == myPpid)
    return;
  dynamic_cast<MultiContactProxy*>(myListProxy)->add(id, ppid);
}

void MMUserView::removeFirst()
{
  QPair<QString, unsigned long> contact = *contacts().begin();
  dynamic_cast<MultiContactProxy*>(myListProxy)->remove(contact.first, contact.second);
}

const QSet<QPair<QString, unsigned long> >& MMUserView::contacts() const
{
  return dynamic_cast<MultiContactProxy*>(myListProxy)->contacts();
}

void MMUserView::remove()
{
  dynamic_cast<MultiContactProxy*>(myListProxy)->remove(selectedIndexes());
}

void MMUserView::crop()
{
  dynamic_cast<MultiContactProxy*>(myListProxy)->crop(selectedIndexes());
}

void MMUserView::clear()
{
  dynamic_cast<MultiContactProxy*>(myListProxy)->clear();
}

void MMUserView::addCurrentGroup()
{
  GroupType groupType = Config::ContactList::instance()->groupType();
  unsigned long groupId = Config::ContactList::instance()->groupId();

  // User group 0 in mainwin means all users but that is system group 0 in model
  if (groupType == GROUPS_USER && groupId == GROUP_ALL_USERS)
    groupType = GROUPS_SYSTEM;

  dynamic_cast<MultiContactProxy*>(myListProxy)->addGroup(groupType, groupId);

  // Make sure current user isn't added
  dynamic_cast<MultiContactProxy*>(myListProxy)->remove(myId, myPpid);
}

void MMUserView::addAll()
{
  // Add all contacts from "All users" group
  dynamic_cast<MultiContactProxy*>(myListProxy)->addGroup(GROUPS_SYSTEM, GROUP_ALL_USERS);

  // Make sure current user isn't added
  dynamic_cast<MultiContactProxy*>(myListProxy)->remove(myId, myPpid);
}

void MMUserView::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasText())
    event->acceptProposedAction();
}

void MMUserView::dropEvent(QDropEvent* event)
{
  // We ignore the event per default and then accept it if we
  // get to the end of this function.
  event->ignore();

  if (event->mimeData()->hasText() && event->mimeData()->text().length() > 4)
  {
    QString text = event->mimeData()->text();

    unsigned long ppid = 0;
    FOR_EACH_PROTO_PLUGIN_START(gLicqDaemon)
    {
      if (text.startsWith(PPIDSTRING((*_ppit)->PPID())))
      {
        ppid = (*_ppit)->PPID();
        break;
      }
    }
    FOR_EACH_PROTO_PLUGIN_END;

    if (ppid == 0)
      return;

    QString id = text.mid(4);

    if (id.isEmpty())
      return;

    add(id, ppid);
  }
  else
    return; // Not accepted

  event->acceptProposedAction();
}

void MMUserView::mousePressEvent(QMouseEvent* event)
{
  UserViewBase::mousePressEvent(event);

  if (event->button() == Qt::LeftButton)
  {
    if (!indexAt(event->pos()).isValid())
    {
      // Clicking outiside list will clear selection
      selectionModel()->clearSelection();
      setCurrentIndex(QModelIndex());
    }
  }
}

void MMUserView::contextMenuEvent(QContextMenuEvent* event)
{
  myMenu->popup(viewport()->mapToGlobal(event->pos()));
}

void MMUserView::keyPressEvent(QKeyEvent* event)
{
  if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))
  {
    event->ignore();
    UserViewBase::keyPressEvent(event);
    return;
  }

  switch (event->key())
  {
    case Qt::Key_Space:
      myMenu->popup(viewport()->mapToGlobal(QPoint(40, visualRect(currentIndex()).y())));
      return;

    default:
      UserViewBase::keyPressEvent(event);
  }
}