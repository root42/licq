// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2000-2011 Licq developers
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

#include "usersendchatevent.h"

#include <QAction>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QTextCodec>
#include <QTimer>

#include <licq/contactlist/user.h>
#include <licq/event.h>
#include <licq/icq.h>
#include <licq/icqdefines.h>
#include <licq/protocolmanager.h>
#include <licq/userevents.h>

#include "config/chat.h"

#include "core/gui-defines.h"
#include "core/messagebox.h"

#include "dialogs/chatdlg.h"
#include "dialogs/joinchatdlg.h"

#include "widgets/infofield.h"
#include "widgets/mledit.h"

using Licq::gProtocolManager;
using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::UserSendChatEvent */

UserSendChatEvent::UserSendChatEvent(const Licq::UserId& userId, QWidget* parent)
  : UserSendCommon(ChatEvent, userId, parent, "UserSendChatEvent")
{
  myChatPort = 0;
  myMassMessageCheck->setChecked(false);
  myMassMessageCheck->setEnabled(false);
  myForeColor->setEnabled(false);
  myBackColor->setEnabled(false);

  myMainWidget->addWidget(myViewSplitter);

  if (!Config::Chat::instance()->msgChatView())
    myMessageEdit->setMinimumHeight(150);

  QHBoxLayout* h_lay = new QHBoxLayout();
  myMainWidget->addLayout(h_lay);
  myItemLabel = new QLabel(tr("Multiparty: "));
  h_lay->addWidget(myItemLabel);

  myItemEdit = new InfoField(false);
  h_lay->addWidget(myItemEdit);

  myBrowseButton = new QPushButton(tr("Invite"));
  connect(myBrowseButton, SIGNAL(clicked()), SLOT(inviteUser()));
  h_lay->addWidget(myBrowseButton);

  myBaseTitle += tr(" - Chat Request");

  setWindowTitle(myBaseTitle);
  myEventTypeGroup->actions().at(ChatEvent)->setChecked(true);
}

UserSendChatEvent::~UserSendChatEvent()
{
  // Empty
}

bool UserSendChatEvent::sendDone(const Licq::Event* e)
{
  if (!e->ExtendedAck() || !e->ExtendedAck()->accepted())
  {
    Licq::UserReadGuard u(myUsers.front());
    QString s = !e->ExtendedAck() ?
      tr("No reason provided") :
      myCodec->toUnicode(e->ExtendedAck()->response().c_str());
    QString result = tr("Chat with %1 refused:\n%2")
      .arg(!u.isLocked() ? u->accountId().c_str() : QString::fromUtf8(u->getAlias().c_str()))
      .arg(s);
    u.unlock();
    InformUser(this, result);
  }
  else
  {
    const Licq::EventChat* c = dynamic_cast<const Licq::EventChat*>(e->userEvent());
    if (c->Port() == 0)  // If we requested a join, no need to do anything
    {
      ChatDlg* chatDlg = new ChatDlg(myUsers.front());
      chatDlg->StartAsClient(e->ExtendedAck()->port());
    }
  }

  return true;
}

void UserSendChatEvent::resetSettings()
{
  myMessageEdit->clear();
  myItemEdit->clear();
  myMessageEdit->setFocus();
  massMessageToggled(false);
}

void UserSendChatEvent::inviteUser()
{
  if (myChatPort == 0)
  {
    if (ChatDlg::chatDlgs.size() > 0)
    {
      ChatDlg* chatDlg = NULL;
      JoinChatDlg* j = new JoinChatDlg(true, this);
      if (j->exec() && (chatDlg = j->JoinedChat()) != NULL)
      {
        myItemEdit->setText(j->ChatClients());
        myChatPort = chatDlg->LocalPort();
        myChatClients = chatDlg->ChatName() + ", " + chatDlg->ChatClients();
      }
      delete j;
      myBrowseButton->setText(tr("Clear"));
    }
  }
  else
  {
    myChatPort = 0;
    myChatClients = "";
    myItemEdit->setText("");
    myBrowseButton->setText(tr("Invite"));
  }
}

void UserSendChatEvent::send()
{
  // Take care of typing notification now`
  mySendTypingTimer->stop();
  connect(myMessageEdit, SIGNAL(textChanged()), SLOT(messageTextChanged()));
  gProtocolManager.sendTypingNotification(myUsers.front(), false, myConvoId);

  unsigned long icqEventTag;

  if (myChatPort == 0)
    //TODO in daemon
    icqEventTag = gLicqDaemon->icqChatRequest(
        myUsers.front(),
        myCodec->fromUnicode(myMessageEdit->toPlainText()).data(),
        myUrgentCheck->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL,
        mySendServerCheck->isChecked());
  else
    icqEventTag = gLicqDaemon->icqMultiPartyChatRequest(
        myUsers.front(),
        myCodec->fromUnicode(myMessageEdit->toPlainText()).data(),
        myCodec->fromUnicode(myChatClients).data(),
        myChatPort,
        myUrgentCheck->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL,
        mySendServerCheck->isChecked());

  myEventTag.push_back(icqEventTag);

  UserSendCommon::send();
}
