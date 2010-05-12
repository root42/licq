// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2000-2009 Licq developers
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

#include "usersendcontactevent.h"

#include <QAction>
#include <QLabel>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

#include <licq_icq.h>
#include <licq_icqd.h>
#include <licq_user.h>
#include <licq/protocolmanager.h>

#include "config/chat.h"

#include "contactlist/contactlist.h"

#include "core/gui-defines.h"

#include "dialogs/mmsenddlg.h"
#include "dialogs/showawaymsgdlg.h"

#include "views/mmuserview.h"

#include "widgets/mledit.h"

using Licq::StringList;
using Licq::gProtocolManager;
using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::UserSendContactEvent */

UserSendContactEvent::UserSendContactEvent(const UserId& userId, QWidget* parent)
  : UserSendCommon(ContactEvent, userId, parent, "UserSendContactEvent")
{
  myMassMessageCheck->setChecked(false);
  myMassMessageCheck->setEnabled(false);
  myForeColor->setEnabled(false);
  myBackColor->setEnabled(false);
  myEmoticon->setEnabled(false);

  myMainWidget->addWidget(myViewSplitter);

  QSplitter* bottom = dynamic_cast<QSplitter*>(myMessageEdit->parentWidget());
  int ind = bottom->indexOf(myMessageEdit);
  delete myMessageEdit;
  myMessageEdit = NULL;

  QWidget* w = new QWidget();
  bottom->insertWidget(ind, w);
  QVBoxLayout* lay = new QVBoxLayout(w);
  lay->setContentsMargins(0, 0, 0, 0);

  w->setToolTip(tr("Drag Users Here - Right Click for Options"));

  myContactsList = new MMUserView(myUsers.front(), gGuiContactList);
  lay->addWidget(myContactsList);

  myBaseTitle += tr(" - Contact List");

  setWindowTitle(myBaseTitle);
  myEventTypeGroup->actions().at(ContactEvent)->setChecked(true);
}

UserSendContactEvent::~UserSendContactEvent()
{
  // Empty
}

void UserSendContactEvent::setContact(const UserId& userId)
{
  const LicqUser* u = gUserManager.fetchUser(userId);

  if (u != NULL)
  {
    myContactsList->add(u->id());
    gUserManager.DropUser(u);
  }
}

bool UserSendContactEvent::sendDone(const LicqEvent* e)
{
  if (e->Command() != ICQ_CMDxTCP_START)
    return true;

  bool showAwayDlg = false;
  const LicqUser* u = gUserManager.fetchUser(myUsers.front());
  if (u != NULL)
  {
    showAwayDlg = u->Away() && u->ShowAwayMsg();
    gUserManager.DropUser(u);
  }

  if (showAwayDlg && Config::Chat::instance()->popupAutoResponse())
    new ShowAwayMsgDlg(myUsers.front());

  return true;
}

void UserSendContactEvent::resetSettings()
{
  myContactsList->clear();
  massMessageToggled(false);
}

//TODO Fix this for new protocol plugin
void UserSendContactEvent::send()
{
  // Take care of typing notification now
  mySendTypingTimer->stop();

  const LicqUser* user = gUserManager.fetchUser(myUsers.front());
  QString accountId = user->accountId().c_str();
  gUserManager.DropUser(user);
  gProtocolManager.sendTypingNotification(myUsers.front(), false, myConvoId);

  StringList users;

  UserId userId;
  foreach (userId, myContactsList->contacts())
  {
    const LicqUser* user = gUserManager.fetchUser(userId, LOCK_R);
    if (user == NULL)
      continue;
    QString accountId = user->accountId().c_str();
    gUserManager.DropUser(user);
    users.push_back(accountId.toLatin1().data());
  }

  if (users.size() == 0)
    return;

  if (!checkSecure())
    return;

  if (myMassMessageCheck->isChecked())
  {
    MMSendDlg* m = new MMSendDlg(myMassMessageList, this);
    connect(m, SIGNAL(eventSent(const LicqEvent*)), SIGNAL(eventSent(const LicqEvent*)));
    int r = m->go_contact(users);
    delete m;
    if (r != QDialog::Accepted) return;
  }

  unsigned long icqEventTag;
  icqEventTag = gLicqDaemon->icqSendContactList(
      accountId.toLatin1(),
      users,
      mySendServerCheck->isChecked() ? false : true,
      myUrgentCheck->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL,
      myMassMessageCheck->isChecked(),
      &myIcqColor);
  myEventTag.push_back(icqEventTag);

  UserSendCommon::send();
}
