// -*- c-basic-offset: 2 -*-
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// written by Graham Roff <graham@licq.org>
// contributions by Dirk A. Mueller <dirk@licq.org>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qcheckbox.h>
#include <qdatetime.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qsplitter.h>

#ifdef USE_KDE
#include <kfiledialog.h>
#else
#include <qfiledialog.h>
#endif

#include "licq_message.h"
#include "licq_icqd.h"
#include "licq_log.h"

#include "authuserdlg.h"
#include "ewidgets.h"
#include "mainwin.h"
#include "messagebox.h"
#include "mmlistview.h"
#include "mmsenddlg.h"
#include "chatdlg.h"
#include "chatjoin.h"
#include "eventdesc.h"
#include "filedlg.h"
#include "forwarddlg.h"
#include "usereventdlg.h"
#include "refusedlg.h"
#include "sigman.h"
#include "showawaymsgdlg.h"


// -----------------------------------------------------------------------------

UserEventCommon::UserEventCommon(CICQDaemon *s, CSignalManager *theSigMan,
                                 CMainWindow *m, unsigned long _nUin,
                                 QWidget* parent, const char* name)
  : QWidget(parent, name, WDestructiveClose)
{
  server = s;
  mainwin = m;
  sigman = theSigMan;
  icqEventTag = NULL;
  m_nUin = _nUin;
  m_bOwner = (m_nUin == gUserManager.OwnerUin());
  m_bDeleteUser = false;

  top_hlay = new QHBoxLayout(this, 6);
  top_lay = new QVBoxLayout(top_hlay);

  QBoxLayout *layt = new QHBoxLayout(top_lay, 8);
  layt->addWidget(new QLabel(tr("Status:"), this));
  nfoStatus = new CInfoField(this, true);
  layt->addWidget(nfoStatus);
  layt->addWidget(new QLabel(tr("Time:"), this));
  nfoTimezone = new CInfoField(this, true);
  layt->addWidget(nfoTimezone);
  // these strings are not translated because they're replaced with an
  // icon anyway
  btnHistory = new QPushButton("Hist", this);
  connect(btnHistory, SIGNAL(clicked()), this, SLOT(showHistory()));
  layt->addWidget(btnHistory);
  btnInfo = new QPushButton("Info", this);
  connect(btnInfo, SIGNAL(clicked()), this, SLOT(showUserInfo()));
  layt->addWidget(btnInfo);

  tmrTime = NULL;
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
  if (u != NULL)
  {
    nfoStatus->setData(u->StatusStr());
    SetGeneralInfo(u);
    gUserManager.DropUser(u);
  }

  connect (sigman, SIGNAL(signal_updatedUser(CICQSignal *)),
           this, SLOT(slot_userupdated(CICQSignal *)));

  mainWidget = new QWidget(this);
  top_lay->addWidget(mainWidget);
}


//-----UserEventCommon::SetGeneralInfo---------------------------------------
void UserEventCommon::SetGeneralInfo(ICQUser *u)
{
  if (u->GetTimezone() == TIMEZONE_UNKNOWN)
    nfoTimezone->setText(tr("Unknown"));
  else
  {
    m_nRemoteTimeOffset = u->LocalTimeOffset();
    QDateTime t;
    t.setTime_t(u->LocalTime());
    nfoTimezone->setText(t.time().toString());
    /*nfoTimezone->setText(tr("%1 (GMT%1%1%1)")
                         .arg(t.time().toString())
                         .arg(u->GetTimezone() > 0 ? "-" : "+")
                         .arg(abs(u->GetTimezone() / 2))
                         .arg(u->GetTimezone() % 2 ? "30" : "00") );*/
    if (tmrTime == NULL)
    {
      tmrTime = new QTimer(this);
      connect(tmrTime, SIGNAL(timeout()), this, SLOT(slot_updatetime()));
      tmrTime->start(3000);
    }
  }

  m_sBaseTitle = QString::fromLocal8Bit(u->GetAlias()) + " (" +
             QString::fromLocal8Bit(u->GetFirstName()) + " " +
             QString::fromLocal8Bit(u->GetLastName())+ ")";
  setCaption(m_sBaseTitle);
  setIconText(u->GetAlias());
}


void UserEventCommon::slot_updatetime()
{
  QDateTime t;
  t.setTime_t(time(NULL) + m_nRemoteTimeOffset);
  nfoTimezone->setText(t.time().toString());
  //nfoTimezone->setText(nfoTimezone->text().replace(0, t.time().toString().length(), t.time().toString()));
}


UserEventCommon::~UserEventCommon()
{
  emit finished(m_nUin);

  if (m_bDeleteUser && !m_bOwner)
    mainwin->RemoveUserFromList(m_nUin, this);
}


//-----UserEventCommon::slot_userupdated-------------------------------------
void UserEventCommon::slot_userupdated(CICQSignal *sig)
{
  if (m_nUin != sig->Uin()) return;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u == NULL) return;

  switch (sig->SubSignal())
  {
    case USER_STATUS:
    {
      nfoStatus->setData(u->StatusStr());
      break;
    }
    case USER_GENERAL:
    {
      SetGeneralInfo(u);
      break;
    }
  }
  // Call the event specific function now
  UserUpdated(sig, u);

  gUserManager.DropUser(u);
}


void UserEventCommon::showHistory()
{
  mainwin->callInfoTab(mnuUserHistory, m_nUin);
}


void UserEventCommon::showUserInfo()
{
  mainwin->callInfoTab(mnuUserGeneral, m_nUin);
}


//=====UserViewEvent=========================================================
UserViewEvent::UserViewEvent(CICQDaemon *s, CSignalManager *theSigMan,
                             CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserEventCommon(s, theSigMan, m, _nUin, parent, "UserViewEvent")
{
  QBoxLayout* lay = new QVBoxLayout(mainWidget);
  splRead = new QSplitter(Vertical, mainWidget);
  lay->addWidget(splRead);
  splRead->setOpaqueResize();

  msgView = new MsgView(splRead);
  mleRead = new MLEditWrap(true, splRead, true);
  mleRead->setReadOnly(true);

  connect (msgView, SIGNAL(clicked(QListViewItem *)), this, SLOT(slot_printMessage(QListViewItem *)));

  QHGroupBox *h_action = new QHGroupBox(mainWidget);
  lay->addSpacing(10);
  lay->addWidget(h_action);
  btnRead1 = new CEButton(h_action);
  btnRead2 = new QPushButton(h_action);
  btnRead3 = new QPushButton(h_action);
  btnRead4 = new QPushButton(h_action);

  btnRead1->setEnabled(false);
  btnRead2->setEnabled(false);
  btnRead3->setEnabled(false);
  btnRead4->setEnabled(false);

  connect(btnRead1, SIGNAL(clicked()), this, SLOT(slot_btnRead1()));
  connect(btnRead2, SIGNAL(clicked()), this, SLOT(slot_btnRead2()));
  connect(btnRead3, SIGNAL(clicked()), this, SLOT(slot_btnRead3()));
  connect(btnRead4, SIGNAL(clicked()), this, SLOT(slot_btnRead4()));

  QBoxLayout *h_lay = new QHBoxLayout(top_lay, 4);
  if (!m_bOwner)
  {
    QPushButton *btnMenu = new QPushButton(tr("&Menu"), this);
    h_lay->addWidget(btnMenu);
    connect(btnMenu, SIGNAL(pressed()), this, SLOT(slot_usermenu()));
    btnMenu->setPopup(gMainWindow->UserMenu());
    chkAutoClose = new QCheckBox(tr("Aut&o Close"), this);
    chkAutoClose->setChecked(gMainWindow->m_bAutoClose);
    h_lay->addWidget(chkAutoClose);
  }
  h_lay->addStretch(1);
  int bw = 75;
  btnReadNext = new QPushButton(tr("Nex&t"), this);
  setTabOrder(btnRead4, btnReadNext);
  btnClose = new CEButton(tr("&Close"), this);
  setTabOrder(btnReadNext, btnClose);
  bw = QMAX(bw, btnReadNext->sizeHint().width());
  bw = QMAX(bw, btnClose->sizeHint().width());
  btnReadNext->setFixedWidth(bw);
  btnClose->setFixedWidth(bw);
  h_lay->addWidget(btnReadNext);
  btnReadNext->setEnabled(false);
  connect(btnReadNext, SIGNAL(clicked()), this, SLOT(slot_btnReadNext()));
  connect(btnClose, SIGNAL(clicked()), SLOT(slot_close()));
  h_lay->addWidget(btnClose);

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u != NULL && u->NewMessages() > 0)
  {
    MsgViewItem *e = new MsgViewItem(u->EventPeek(0), msgView);
    for (unsigned short i = 1; i < u->NewMessages(); i++)
    {
      (void) new MsgViewItem(u->EventPeek(i), msgView);
    }
    gUserManager.DropUser(u);
    slot_printMessage(e);
    msgView->setSelected(e, true);
    msgView->ensureItemVisible(e);
  }
  else
    gUserManager.DropUser(u);
}


UserViewEvent::~UserViewEvent()
{
}


void UserViewEvent::slot_close()
{
  m_bDeleteUser = btnClose->stateWhenPressed() & ControlButton;
  close();
}


//-----UserViewEvent::slot_autoClose-----------------------------------------
void UserViewEvent::slot_autoClose()
{
  if(!chkAutoClose->isChecked()) return;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  bool doclose = (u->NewMessages() == 0);
  gUserManager.DropUser(u);

  if(doclose)
    close();
}

//-----UserViewEvent::updateNextButton---------------------------------------
void UserViewEvent::updateNextButton()
{
  int num = 0;

  MsgViewItem* it = static_cast<MsgViewItem*>(msgView->firstChild());
  MsgViewItem* e = NULL;

  while(it) {
    if(it->m_nEventId != -1) {
      e = it;
      num++;
    }
    it = static_cast<MsgViewItem*>(it->nextSibling());
  }

  btnReadNext->setEnabled(num > 0);

  if(num > 1)
    btnReadNext->setText(tr("Nex&t (%1)").arg(num));
  else if (num == 1)
    btnReadNext->setText(tr("Nex&t"));

  if(e && e->msg)
    btnReadNext->setIconSet(CMainWindow::iconForEvent(e->msg->SubCommand()));
}


//-----UserViewEvent::slot_printMessage--------------------------------------
void UserViewEvent::slot_printMessage(QListViewItem *eq)
{
  if (eq == NULL) return;

  MsgViewItem *e = (MsgViewItem *)eq;

  btnRead1->setText("");
  btnRead2->setText("");
  btnRead3->setText("");
  btnRead4->setText("");
  btnRead1->setEnabled(false);
  btnRead2->setEnabled(false);
  btnRead3->setEnabled(false);
  btnRead4->setEnabled(false);

  CUserEvent *m = e->msg;
  m_xCurrentReadEvent = m;
  mleRead->setText(QString::fromLocal8Bit(m->Text()));
  if (m->Direction() == D_RECEIVER && (m->Command() == ICQ_CMDxTCP_START || m->Command() == ICQ_CMDxRCV_SYSxMSGxONLINE))
  {
    switch (m->SubCommand())
    {
      case ICQ_CMDxSUB_CHAT:  // accept or refuse a chat request
      case ICQ_CMDxSUB_FILE:  // accept or refuse a file transfer
        btnRead1->setText(tr("&Reply"));
        if (m->IsCancelled())
        {
          mleRead->append(tr("\n--------------------\nRequest was cancelled."));
        }
        else
        {
          btnRead2->setText(tr("A&ccept"));
          btnRead3->setText(tr("&Refuse"));
          // If this is a chat, and we already have chats going, and this is
          // not a join request, then we can join
          if (m->SubCommand() == ICQ_CMDxSUB_CHAT &&
              ChatDlg::chatDlgs.size() > 0 &&
              ((CEventChat *)m)->Port() == 0)
            btnRead4->setText(tr("&Join"));
        }
        break;

      case ICQ_CMDxSUB_MSG:
        btnRead1->setText(tr("&Reply"));
        btnRead2->setText(tr("&Quote"));
        btnRead3->setText(tr("&Forward"));
        break;

      case ICQ_CMDxSUB_URL:   // view a url
        btnRead1->setText(tr("&Reply"));
        btnRead2->setText(tr("&Quote"));
        btnRead3->setText(tr("&Forward"));
        if (server->getUrlViewer() != NULL)
          btnRead4->setText(tr("&View"));
        break;

      case ICQ_CMDxSUB_AUTHxREQUEST:
      {
        btnRead1->setText(tr("A&uthorize"));
        btnRead2->setText(tr("&Refuse"));
        ICQUser *u = gUserManager.FetchUser( ((CEventAuthRequest *)m)->Uin(), LOCK_R);
        if (u == NULL)
          btnRead3->setText(tr("A&dd User"));
        else
          gUserManager.DropUser(u);
        break;
      }
      case ICQ_CMDxSUB_AUTHxGRANTED:
      {
        ICQUser *u = gUserManager.FetchUser( ((CEventAuthGranted *)m)->Uin(), LOCK_R);
        if (u == NULL)
          btnRead1->setText(tr("A&dd User"));
        else
          gUserManager.DropUser(u);
        break;
      }
      case ICQ_CMDxSUB_ADDEDxTOxLIST:
      {
        ICQUser *u = gUserManager.FetchUser( ((CEventAdded *)m)->Uin(), LOCK_R);
        if (u == NULL)
          btnRead1->setText(tr("A&dd User"));
        else
          gUserManager.DropUser(u);
        break;
      }
      case ICQ_CMDxSUB_CONTACTxLIST:
      {
        int s = static_cast<CEventContactList*>(m)->Contacts().size();
        if(s > 1)
          btnRead1->setText(tr("A&dd %1 Users").arg(s));
        else if(s == 1)
          btnRead1->setText(tr("A&dd User"));
        break;
      }
    } // switch
  }  // if

  if (!btnRead1->text().isEmpty()) btnRead1->setEnabled(true);
  if (!btnRead2->text().isEmpty()) btnRead2->setEnabled(true);
  if (!btnRead3->text().isEmpty()) btnRead3->setEnabled(true);
  if (!btnRead4->text().isEmpty()) btnRead4->setEnabled(true);

  btnRead1->setFocus();

  if (e->m_nEventId != -1 && e->msg->Direction() == D_RECEIVER)
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_W);
    u->EventClearId(e->m_nEventId);
    gUserManager.DropUser(u);
    e->MarkRead();
  }
}


void UserViewEvent::generateReply()
{
  QString s;
  for (int i = 0; i < mleRead->numLines(); i++)
    if(!mleRead->textLine(i).stripWhiteSpace().isEmpty())
      s += QString("> ") + mleRead->textLine(i) + " \n";
    else
      s += "\n";

  s = s.stripWhiteSpace()+ " \n";

  sendMsg(s);
}


void UserViewEvent::sendMsg(QString txt)
{
  UserSendMsgEvent *e = new UserSendMsgEvent(server, sigman, mainwin, m_nUin);
  e->setText(txt);
  e->show();

  connect(e, SIGNAL(autoCloseNotify()), this, SLOT(slot_autoClose()));
}


void UserViewEvent::slot_btnRead1()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:  // reply/quote
    case ICQ_CMDxSUB_URL:
    case ICQ_CMDxSUB_CHAT:
    case ICQ_CMDxSUB_FILE:
      if (btnRead1->stateWhenPressed() & ControlButton)
        generateReply();
      else
        sendMsg("");
      break;

    case ICQ_CMDxSUB_AUTHxREQUEST:
      (void) new AuthUserDlg(server, ((CEventAuthRequest *)m_xCurrentReadEvent)->Uin(), true);
      break;

    case ICQ_CMDxSUB_AUTHxGRANTED:
      server->AddUserToList( ((CEventAuthGranted *)m_xCurrentReadEvent)->Uin());
      break;

    case ICQ_CMDxSUB_ADDEDxTOxLIST:
      server->AddUserToList( ((CEventAdded *)m_xCurrentReadEvent)->Uin());
      break;
    case ICQ_CMDxSUB_CONTACTxLIST:
    {
      const ContactList& cl = static_cast<CEventContactList*>(m_xCurrentReadEvent)->Contacts();

      ContactList::const_iterator it;
      for(it = cl.begin(); it != cl.end(); ++it) {
        ICQUser* u = gUserManager.FetchUser((*it)->Uin(), LOCK_R);
        if(u == NULL)
          server->AddUserToList((*it)->Uin());
        gUserManager.DropUser(u);
      }
      btnRead1->setEnabled(false);
    }
  } // switch
}

void UserViewEvent::slot_btnRead2()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:  // quote
    case ICQ_CMDxSUB_URL:
      generateReply();
      break;

    case ICQ_CMDxSUB_CHAT:  // accept a chat request
    {
      btnRead1->setEnabled(false);
      btnRead2->setEnabled(false);
      CEventChat *c = (CEventChat *)m_xCurrentReadEvent;
      ChatDlg *chatDlg = new ChatDlg(m_nUin, server);
      if (c->Port() != 0)  // Joining a multiparty chat (we connect to them)
      {
        if (chatDlg->StartAsClient(c->Port()))
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
      }
      else  // single party (other side connects to us)
      {
        if (chatDlg->StartAsServer())
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
      }
      break;
    }

    case ICQ_CMDxSUB_FILE:  // accept a file transfer
    {
      btnRead1->setEnabled(false);
      btnRead2->setEnabled(false);
      CEventFile *f = (CEventFile *)m_xCurrentReadEvent;
      CFileDlg *fileDlg = new CFileDlg(m_nUin, server);
      if (fileDlg->ReceiveFiles())
        server->icqFileTransferAccept(m_nUin, fileDlg->LocalPort(), f->Sequence());
      break;
    }

    case ICQ_CMDxSUB_AUTHxREQUEST:
    {
      (void) new AuthUserDlg(server, ((CEventAuthRequest *)m_xCurrentReadEvent)->Uin(), false);
      break;
    }
  } // switch

}


void UserViewEvent::slot_btnRead3()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:  // Forward
    case ICQ_CMDxSUB_URL:
    {
      CForwardDlg *f = new CForwardDlg(server, sigman, mainwin, m_xCurrentReadEvent);
      f->move(x() + width() / 2 - f->width() / 2, y() + height() / 2 - f->height() / 2);
      f->show();
      break;
    }

    case ICQ_CMDxSUB_CHAT:  // refuse a chat request
    {
      CRefuseDlg *r = new CRefuseDlg(m_nUin, tr("Chat"), this);
      if (r->exec())
      {
        btnRead1->setEnabled(false);
        btnRead2->setEnabled(false);
        server->icqChatRequestRefuse(m_nUin, r->RefuseMessage().local8Bit(),
           m_xCurrentReadEvent->Sequence());
      }
      delete r;
      break;
    }

    case ICQ_CMDxSUB_FILE:  // refuse a file transfer
    {
      CRefuseDlg *r = new CRefuseDlg(m_nUin, tr("File Transfer"), this);
      if (r->exec())
      {
        btnRead1->setEnabled(false);
        btnRead2->setEnabled(false);
        server->icqFileTransferRefuse(m_nUin, r->RefuseMessage().local8Bit(),
           m_xCurrentReadEvent->Sequence());
      }
      delete r;
      break;
    }

    case ICQ_CMDxSUB_AUTHxREQUEST:
      server->AddUserToList( ((CEventAuthRequest *)m_xCurrentReadEvent)->Uin());
      break;

  }
}


void UserViewEvent::slot_btnRead4()
{
  if (m_xCurrentReadEvent == NULL) return;

  switch (m_xCurrentReadEvent->SubCommand())
  {
    case ICQ_CMDxSUB_CHAT:  // join to current chat
    {
      CEventChat *c = (CEventChat *)m_xCurrentReadEvent;
      if (c->Port() != 0)  // Joining a multiparty chat (we connect to them)
      {
        ChatDlg *chatDlg = new ChatDlg(m_nUin, server);
        if (chatDlg->StartAsClient(c->Port()))
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
      }
      else  // single party (other side connects to us)
      {
        ChatDlg *chatDlg = NULL;
        CJoinChatDlg *j = new CJoinChatDlg(this);
        if (j->exec() && (chatDlg = j->JoinedChat()) != NULL)
          server->icqChatRequestAccept(m_nUin, chatDlg->LocalPort(), c->Sequence());
        delete j;
      }
      break;
    }
    case ICQ_CMDxSUB_URL:   // view a url
      if (!server->ViewUrl(((CEventUrl *)m_xCurrentReadEvent)->Url()))
        WarnUser(this, tr("View URL failed"));
      break;
  }
}


void UserViewEvent::slot_btnReadNext()
{
  MsgViewItem* it = static_cast<MsgViewItem*>(msgView->firstChild());
  MsgViewItem* e = NULL;

  while(it) {
    if(it->m_nEventId != -1) {
      e = it;
    }
    it = static_cast<MsgViewItem*>(it->nextSibling());
  }

  updateNextButton();

  if(e != NULL) {
    msgView->setSelected(e, true);
    msgView->ensureItemVisible(e);
    slot_printMessage(e);
  }
}


void UserViewEvent::UserUpdated(CICQSignal *sig, ICQUser *u)
{
  if(sig->SubSignal() == USER_EVENTS)
  {
    CUserEvent* e = NULL;

    if (sig->Argument() > 0)
    {
      e = u->EventPeekId(sig->Argument());
      if (e != NULL)
      {
        MsgViewItem *m = new MsgViewItem(e, msgView);
        msgView->ensureItemVisible(m);
      }
    }

    updateNextButton();
  }
}


//=====UserSendCommon========================================================
UserSendCommon::UserSendCommon(CICQDaemon *s, CSignalManager *theSigMan,
                               CMainWindow *m, unsigned long _nUin, QWidget* parent, const char* name)
  : UserEventCommon(s, theSigMan, m, _nUin, parent, name)
{
  grpMR = NULL;

  QGroupBox *box = new QGroupBox(this);
  top_lay->addWidget(box);
  QBoxLayout *vlay = new QVBoxLayout(box, 10, 5);
  QBoxLayout *hlay = new QHBoxLayout(vlay);
  chkSendServer = new QCheckBox(tr("Se&nd through server"), box);
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  chkSendServer->setChecked(u->SendServer() || (u->StatusOffline() && u->SocketDesc() == -1));
  if (u->Ip() == 0)
  {
    chkSendServer->setChecked(true);
    chkSendServer->setEnabled(false);
  }
  gUserManager.DropUser(u);
  hlay->addWidget(chkSendServer);
  chkUrgent = new QCheckBox(tr("U&rgent"), box);
  hlay->addWidget(chkUrgent);
  chkMass = new QCheckBox(tr("M&ultiple recipients"), box);
  hlay->addWidget(chkMass);
  connect(chkMass, SIGNAL(toggled(bool)), this, SLOT(massMessageToggled(bool)));

#ifdef USE_SPOOFING
  hlay = new QHBoxLayout(vlay);
  chkSpoof = new QCheckBox(tr("S&poof UIN:"), box);
  hlay->addWidget(chkSpoof);
  edtSpoof = new QLineEdit(box);
  hlay->addWidget(edtSpoof);
  edtSpoof->setEnabled(false);
  edtSpoof->setValidator(new QIntValidator(10000, 2000000000, edtSpoof));
  connect(chkSpoof, SIGNAL(toggled(bool)), edtSpoof, SLOT(setEnabled(bool)));
#else
  edtSpoof = NULL;
  chkSpoof = NULL;
#endif

  QBoxLayout* h_lay = new QHBoxLayout(top_lay);
  if (!m_bOwner)
  {
    QPushButton *btnMenu = new QPushButton(tr("&Menu"), this);
    h_lay->addWidget(btnMenu);
    connect(btnMenu, SIGNAL(pressed()), this, SLOT(slot_usermenu()));
    btnMenu->setPopup(gMainWindow->UserMenu());
  }
  cmbSendType = new QComboBox(this);
  cmbSendType->insertItem(tr("Message"));
  cmbSendType->insertItem(tr("URL"));
  cmbSendType->insertItem(tr("Chat Request"));
  cmbSendType->insertItem(tr("File Transfer"));
  cmbSendType->insertItem(tr("Contact List"));
  connect(cmbSendType, SIGNAL(activated(int)), this, SLOT(changeEventType(int)));
  h_lay->addWidget(cmbSendType);
  h_lay->addStretch(1);
  btnSend = new QPushButton(tr("&Send"), this);
  h_lay->addWidget(btnSend);
  connect(btnSend, SIGNAL(clicked()), this, SLOT(sendButton()));
  btnCancel = new QPushButton(tr("&Close"), this);
  h_lay->addWidget(btnCancel);
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancelSend()));
  mleSend = new MLEditWrap(true, mainWidget, true);
  setTabOrder(mleSend, btnSend);
  setTabOrder(btnSend, btnCancel);
  connect (mleSend, SIGNAL(signal_CtrlEnterPressed()), btnSend, SIGNAL(clicked()));
}


UserSendCommon::~UserSendCommon()
{
}

//-----UserSendCommon::changeEventType---------------------------------------
void UserSendCommon::changeEventType(int id)
{
  UserSendCommon* e = NULL;
  switch(id)
  {
  case 0:
    e = static_cast<UserSendCommon*>(gMainWindow->callFunction(mnuUserSendMsg, m_nUin));
    break;
  case 1:
    e = static_cast<UserSendCommon*>(gMainWindow->callFunction(mnuUserSendUrl, m_nUin));
    break;
  case 2:
    e = static_cast<UserSendCommon*>(gMainWindow->callFunction(mnuUserSendChat, m_nUin));
    break;
  case 3:
    e = static_cast<UserSendCommon*>(gMainWindow->callFunction(mnuUserSendFile, m_nUin));
    break;
  case 4:
    e = static_cast<UserSendCommon*>(gMainWindow->callFunction(mnuUserSendContact, m_nUin));
    break;
  }

  if (e != NULL)
  {
    QPoint p = topLevelWidget()->pos();
    if (e->mleSend && mleSend)
      e->mleSend->setText(mleSend->text());
    e->move(p);
    e->show();

    close();
  }
}

//-----UserSendCommon::massMessageToggled------------------------------------
void UserSendCommon::massMessageToggled(bool b)
{
  if(grpMR == NULL) {
    grpMR = new QVGroupBox(this);
    top_hlay->addWidget(grpMR);

    (void) new QLabel(tr("Drag Users Here\nRight Click for Options"), grpMR);
    lstMultipleRecipients = new CMMUserView(gMainWindow->colInfo, mainwin->m_bShowHeader,
                                            m_nUin, mainwin, grpMR);
    lstMultipleRecipients->setFixedWidth(mainwin->UserView()->width());
  }

  if(b)
    grpMR->show();
  else {
    // doesn't work right TODO investigate why
    int w = grpMR->width();
    grpMR->hide();
    top_hlay->setGeometry(QRect(x(), y(), width()-w, height()));
  }
}


//-----UserSendCommon::sendButton--------------------------------------------
void UserSendCommon::sendButton()
{
  if (icqEventTag != NULL)
  {
    QString title = m_sBaseTitle + " [" + m_sProgressMsg + "]";
    setCaption(title);
    setCursor(waitCursor);
    btnSend->setEnabled(false);
    btnCancel->setText(tr("&Cancel"));
    connect (sigman, SIGNAL(signal_doneUserFcn(ICQEvent *)), this, SLOT(sendDone_common(ICQEvent *)));
  }
}


//-----UserSendCommon::setText-----------------------------------------------
void UserSendCommon::setText(const QString& txt)
{
  if(!mleSend) return;
  mleSend->setText(txt);
  mleSend->GotoEnd();
  mleSend->setEdited(false);
}


//-----UserSendCommon::sendDone_common---------------------------------------
void UserSendCommon::sendDone_common(ICQEvent *e)
{
  if ( !icqEventTag->Equals(e) )
    return;

  QString title, result;
  if (e == NULL)
  {
    result = tr("error");
  }
  else
  {
    switch (e->Result())
    {
    case EVENT_ACKED:
    case EVENT_SUCCESS:
      result = tr("done");
      QTimer::singleShot(5000, this, SLOT(slot_resettitle()));
      break;
    case EVENT_FAILED:
      result = tr("failed");
      break;
    case EVENT_TIMEDOUT:
      result = tr("timed out");
      break;
    case EVENT_ERROR:
      result = tr("error");
      break;
    default:
      break;
    }
  }
  title = m_sBaseTitle + " [" + m_sProgressMsg + result + "]";
  setCaption(title);

  setCursor(arrowCursor);
  btnSend->setEnabled(true);
  btnCancel->setText(tr("&Close"));
  delete icqEventTag;
  icqEventTag = NULL;
  disconnect (sigman, SIGNAL(signal_doneUserFcn(ICQEvent *)), this, SLOT(sendDone_common(ICQEvent *)));

  if (e == NULL || e->Result() != EVENT_ACKED)
  {
    if (e->Command() == ICQ_CMDxTCP_START &&
        (e->SubCommand() != ICQ_CMDxSUB_CHAT &&
         e->SubCommand() != ICQ_CMDxSUB_FILE) &&
        QueryUser(this, tr("Direct send failed,\nsend through server?"), tr("Yes"), tr("No")) )
    {
      RetrySend(e, false, ICQ_TCPxMSG_NORMAL);
    }
    return;
  }

  ICQUser *u = NULL;
  CUserEvent *ue = e->UserEvent();
  QString msg;
  if (e->SubResult() == ICQ_TCPxACK_RETURN)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_R);
    msg = tr("%1 is in %2 mode:\n%3\n")
             .arg(u->GetAlias()).arg(u->StatusStr())
             .arg(QString::fromLocal8Bit(u->AutoResponse()));
    gUserManager.DropUser(u);
    switch (QueryUser(this, msg, tr("Send\nUrgent"), tr("Send to\nContact List"), tr("Cancel")))
    {
      case 0:
        RetrySend(e, true, ICQ_TCPxMSG_URGENT);
        break;
      case 1:
        RetrySend(e, true, ICQ_TCPxMSG_LIST);
        break;
      case 2:
        break;
    }
    return;
  }
  else if (e->SubResult() == ICQ_TCPxACK_REFUSE)
  {
    u = gUserManager.FetchUser(m_nUin, LOCK_R);
    msg = tr("%1 refused %2, send through server.")
          .arg(u->GetAlias()).arg(EventDescription(ue));
    InformUser(this, msg);
    gUserManager.DropUser(u);
    return;
  }
  else
  {
    emit autoCloseNotify();
    if (sendDone(e)) close();
  }
}

//-----UserSendCommon::RetrySend---------------------------------------------
void UserSendCommon::RetrySend(ICQEvent *e, bool bOnline, unsigned short nLevel)
{
  chkSendServer->setChecked(!bOnline);
  chkUrgent->setChecked(nLevel == ICQ_TCPxMSG_URGENT);

  switch(e->SubCommand())
  {
    case ICQ_CMDxSUB_MSG:
    {
      CEventMsg *ue = (CEventMsg *)e->UserEvent();
      m_sProgressMsg = tr("Sending ");
      m_sProgressMsg += bOnline ? tr("direct") : tr("through server");
      m_sProgressMsg += "...";
      icqEventTag = server->icqSendMessage(m_nUin, ue->Message(), bOnline,
         nLevel, 0);
      break;
    }
    case ICQ_CMDxSUB_URL:
    {
      CEventUrl *ue = (CEventUrl *)e->UserEvent();
      m_sProgressMsg = tr("Sending ");
      m_sProgressMsg += bOnline ? tr("direct") : tr("through server");
      m_sProgressMsg += "...";
      icqEventTag = server->icqSendUrl(m_nUin, ue->Url(), ue->Description(),
         bOnline, nLevel, 0);
      break;
    }
    case ICQ_CMDxSUB_CHAT:
    {
      CEventChat *ue = (CEventChat *)e->UserEvent();
      m_sProgressMsg = tr("Sending...");
      icqEventTag = server->icqChatRequest(m_nUin, ue->Reason(), nLevel);
      break;
    }
    case ICQ_CMDxSUB_FILE:
    {
      CEventFile *ue = (CEventFile *)e->UserEvent();
      m_sProgressMsg = tr("Sending...");
      icqEventTag = server->icqFileTransfer(m_nUin, ue->Filename(),
         ue->FileDescription(), nLevel);
      break;
    }
    default:
    {
      gLog.Warn("%sInternal error: UserSendCommon::RetrySend()\n"
         "%sUnknown sub-command %d.\n", L_WARNxSTR, L_BLANKxSTR, e->SubCommand());
      break;
    }
  }

  UserSendCommon::sendButton();
}


//-----UserSendCommon::cancelSend--------------------------------------------
void UserSendCommon::cancelSend()
{
  if (!icqEventTag)
  {
    close();
    return;
  }

  setCaption(m_sBaseTitle);
  server->CancelEvent(icqEventTag);
  delete icqEventTag;
  icqEventTag = NULL;
  btnSend->setEnabled(true);
  btnCancel->setText(tr("&Close"));
  setCursor(arrowCursor);
}


//-----UserSendCommon::UserUpdated-------------------------------------------
void UserSendCommon::UserUpdated(CICQSignal *sig, ICQUser *u)
{
  switch (sig->SubSignal())
  {
    case USER_STATUS:
    {
      if (u->Ip() == 0)
      {
        chkSendServer->setChecked(true);
        chkSendServer->setEnabled(false);
      }
      else
      {
        chkSendServer->setEnabled(true);
      }
      if (u->StatusOffline())
        chkSendServer->setChecked(true);
      break;
    }
  }
}


//=====UserSendMsgEvent======================================================
UserSendMsgEvent::UserSendMsgEvent(CICQDaemon *s, CSignalManager *theSigMan,
  CMainWindow *m, unsigned long nUin, QWidget *parent)
  : UserSendCommon(s, theSigMan, m, nUin, parent, "UserSendMsgEvent")
{
  QBoxLayout* lay = new QVBoxLayout(mainWidget);
  lay->addWidget(mleSend);
  mleSend->setMinimumHeight(150);
  mleSend->setFocus ();

  m_sBaseTitle += tr(" - Message");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(0);
}


UserSendMsgEvent::~UserSendMsgEvent()
{
}


//-----UserSendMsgEvent::sendButton------------------------------------------
void UserSendMsgEvent::sendButton()
{
  // do nothing if a command is already being processed
  if (icqEventTag != NULL) return;

  if(!mleSend->edited() &&
     !QueryUser(this, tr("You didn't edit the message.\n"
                         "Do you really want to send it?"), tr("&Yes"), tr("&No")))
    return;

  unsigned short nMsgLen = mleSend->text().length();
  if (nMsgLen > MAX_MESSAGE_SIZE && chkSendServer->isChecked()
      && !QueryUser(this, tr("Message is %1 characters, over the ICQ server limit of %2.\n"
                             "The message will be truncated if sent through the server.")
                    .arg(nMsgLen).arg(MAX_MESSAGE_SIZE),
                    tr("C&ontinue"), tr("&Cancel")))
    return;

  unsigned long uin = (chkSpoof && chkSpoof->isChecked() ?
                       edtSpoof->text().toULong() : 0);
  // don't let the user send empty messages
  if (mleSend->text().stripWhiteSpace().isEmpty()) return;

  if (chkMass->isChecked())
  {
    CMMSendDlg *m = new CMMSendDlg(server, sigman, lstMultipleRecipients, this);
    int r = m->go_message(mleSend->text());
    delete m;
    if (r != QDialog::Accepted) return;
  }

  m_sProgressMsg = tr("Sending ");
  m_sProgressMsg += chkSendServer->isChecked() ? tr("through server") : tr("direct");
  m_sProgressMsg += "...";
  icqEventTag = server->icqSendMessage(m_nUin, mleSend->text().local8Bit(),
     chkSendServer->isChecked() ? false : true,
     chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL, uin);

  UserSendCommon::sendButton();
}


//-----UserSendMsgEvent::sendDone--------------------------------------------
bool UserSendMsgEvent::sendDone(ICQEvent *e)
{
  if (e->Command() != ICQ_CMDxTCP_START) return true;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u->Away() && u->ShowAwayMsg())
  {
    gUserManager.DropUser(u);
    (void) new ShowAwayMsgDlg(NULL, NULL, m_nUin);
  }
  else
    gUserManager.DropUser(u);

  return true;
}


//=====UserSendUrlEvent======================================================
UserSendUrlEvent::UserSendUrlEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                   CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendUrlEvent")
{
  QBoxLayout* lay = new QVBoxLayout(mainWidget, 4);

  lay->addWidget(mleSend);

  QBoxLayout* h_lay = new QHBoxLayout(lay);
  lblItem = new QLabel(tr("URL : "), mainWidget);
  h_lay->addWidget(lblItem);
  edtItem = new CInfoField(mainWidget, false);
  h_lay->addWidget(edtItem);

  m_sBaseTitle += tr(" - URL");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(1);
}


UserSendUrlEvent::~UserSendUrlEvent()
{
}

void UserSendUrlEvent::setUrl(const QString& url, const QString& description)
{
  edtItem->setText(url);
  setText(description);
}

//-----UserSendUrlEvent::sendButton------------------------------------------
void UserSendUrlEvent::sendButton()
{
  unsigned long uin = (chkSpoof && chkSpoof->isChecked() ?
                       edtSpoof->text().toULong() : 0);

  if (edtItem->text().stripWhiteSpace().isEmpty())
    return;

  if (chkMass->isChecked())
  {
    CMMSendDlg *m = new CMMSendDlg(server, sigman, lstMultipleRecipients, this);
    int r = m->go_url(edtItem->text(), mleSend->text());
    delete m;
    if (r != QDialog::Accepted) return;
  }

  m_sProgressMsg = tr("Sending ");
  m_sProgressMsg += chkSendServer->isChecked() ? tr("through server") : tr("direct");
  m_sProgressMsg += "...";
  icqEventTag = server->icqSendUrl(m_nUin, edtItem->text().latin1(), mleSend->text().local8Bit(),
                                   chkSendServer->isChecked() ? false : true,
                                   chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL, uin);

  UserSendCommon::sendButton();
}


//-----UserSendUrlEvent::sendDone--------------------------------------------
bool UserSendUrlEvent::sendDone(ICQEvent *e)
{
  if (e->Command() != ICQ_CMDxTCP_START) return true;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u->Away() && u->ShowAwayMsg())
  {
    gUserManager.DropUser(u);
    (void) new ShowAwayMsgDlg(NULL, NULL, m_nUin);
  }
  else
    gUserManager.DropUser(u);

  return true;
}


//=====UserSendFileEvent=====================================================
UserSendFileEvent::UserSendFileEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                     CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendFileEvent")
{
  chkSendServer->setChecked(false);
  chkSendServer->setEnabled(false);
  chkMass->setChecked(false);
  chkMass->setEnabled(false);

  QBoxLayout* lay = new QVBoxLayout(mainWidget, 4);
  lay->addWidget(mleSend);

  QBoxLayout* h_lay = new QHBoxLayout(lay);
  lblItem = new QLabel(tr("File(s): "), mainWidget);
  h_lay->addWidget(lblItem);

  edtItem = new CInfoField(mainWidget, false);
  h_lay->addWidget(edtItem);

  btnBrowse = new QPushButton(tr("Browse"), mainWidget);
  connect(btnBrowse, SIGNAL(clicked()), this, SLOT(browseFile()));
  h_lay->addWidget(btnBrowse);

  m_sBaseTitle += tr(" - File Transfer");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(3);
}


void UserSendFileEvent::browseFile()
{
#ifdef USE_KDE
    QStringList fl = KFileDialog::getOpenFileNames(NULL, NULL, this);
#else
    QStringList fl = QFileDialog::getOpenFileNames(NULL, NULL, this);
#endif
    if (fl.isEmpty()) return;
    QStringList::ConstIterator it;
    QString f;
    for( it = fl.begin(); it != fl.end(); it++ )
    {
      if (it != fl.begin())
        f += ", ";
      f += (*it);
    }
    edtItem->setText(f);
}


UserSendFileEvent::~UserSendFileEvent()
{
}


void UserSendFileEvent::sendButton()
{
  if (edtItem->text().stripWhiteSpace().isEmpty())
  {
    WarnUser(this, tr("You must specify a file to transfer!"));
    return;
  }
  m_sProgressMsg = tr("Sending...");
  icqEventTag = server->icqFileTransfer(m_nUin, edtItem->text().local8Bit(),
                                        mleSend->text().local8Bit(),
                                        chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);

  UserSendCommon::sendButton();
}

void UserSendFileEvent::setFile(const QString& file, const QString& description)
{
  edtItem->setText(file);
  setText(description);
}

//-----UserSendFileEvent::sendDone-------------------------------------------
bool UserSendFileEvent::sendDone(ICQEvent *e)
{
  if (!e->ExtendedAck()->Accepted())
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
    QString result = tr("File transfer with %2 refused:\n%3").arg(u->GetAlias()).arg(e->ExtendedAck()->Response());
    gUserManager.DropUser(u);
    InformUser(this, result);
  }
  else
  {
    CEventFile *f = (CEventFile *)e->UserEvent();
    CFileDlg *fileDlg = new CFileDlg(m_nUin, server);
    fileDlg->SendFiles(f->Filename(), e->ExtendedAck()->Port());
  }

  return true;
}


//=====UserSendChatEvent=====================================================
UserSendChatEvent::UserSendChatEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                     CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendChatEvent")
{
  chkSendServer->setChecked(false);
  chkSendServer->setEnabled(false);
  chkMass->setChecked(false);
  chkMass->setEnabled(false);

  QBoxLayout *lay = new QVBoxLayout(mainWidget);
  lay->addWidget(mleSend);
  mleSend->setMinimumHeight(150);

  m_sBaseTitle += tr(" - Chat Request");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(2);
}


UserSendChatEvent::~UserSendChatEvent()
{
}


//-----UserSendChatEvent::sendButton-----------------------------------------
void UserSendChatEvent::sendButton()
{
  m_sProgressMsg = tr("Sending...");
  if (m_nMPChatPort == 0)
    icqEventTag = server->icqChatRequest(m_nUin,
                                         mleSend->text().local8Bit(),
                                         chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);
  else
    icqEventTag = server->icqMultiPartyChatRequest(m_nUin,
                                                   mleSend->text().local8Bit(), m_szMPChatClients.local8Bit(),
                                                   m_nMPChatPort,
                                                   chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);
  UserSendCommon::sendButton();
}


//-----UserSendChatEvent::sendDone-------------------------------------------
bool UserSendChatEvent::sendDone(ICQEvent *e)
{
  if (!e->ExtendedAck()->Accepted())
  {
    ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
    QString result = tr("Chat with %2 refused:\n%3").arg(u->GetAlias())
                     .arg(e->ExtendedAck()->Response());
    gUserManager.DropUser(u);
    InformUser(this, result);

  }
  else
  {
    CEventChat *c = (CEventChat *)e->UserEvent();
    if (c->Port() == 0)  // If we requested a join, no need to do anything
    {
      ChatDlg *chatDlg = new ChatDlg(m_nUin, server);
      chatDlg->StartAsClient(e->ExtendedAck()->Port());
    }
  }

  return true;
}


//=====UserSendContactEvent==================================================
UserSendContactEvent::UserSendContactEvent(CICQDaemon *s, CSignalManager *theSigMan,
                                           CMainWindow *m, unsigned long _nUin, QWidget* parent)
  : UserSendCommon(s, theSigMan, m, _nUin, parent, "UserSendContactEvent")
{
  delete mleSend; mleSend = NULL;

  QBoxLayout* lay = new QVBoxLayout(mainWidget);
  QLabel* lblContact =  new QLabel(tr("Drag Users Here - Right Click for Options"), mainWidget);
  lay->addWidget(lblContact);

  lstContacts = new CMMUserView(gMainWindow->colInfo, mainwin->m_bShowHeader,
                                m_nUin, mainwin, mainWidget);
  lay->addWidget(lstContacts);

  m_sBaseTitle += tr(" - Contact List");
  setCaption(m_sBaseTitle);
  cmbSendType->setCurrentItem(4);
}


UserSendContactEvent::~UserSendContactEvent()
{
}


void UserSendContactEvent::sendButton()
{
  CMMUserViewItem *i = static_cast<CMMUserViewItem*>(lstContacts->firstChild());
  UinList uins;

  while(i) {

    uins.push_back(i->Uin());

    i = static_cast<CMMUserViewItem *>(i->nextSibling());
  }

  icqEventTag = server->icqSendContactList(m_nUin, uins,
    chkSendServer->isChecked() ? false : true,
    chkUrgent->isChecked() ? ICQ_TCPxMSG_URGENT : ICQ_TCPxMSG_NORMAL);

  UserSendCommon::sendButton();
}


//-----UserSendMsgEvent::sendDone--------------------------------------------
bool UserSendContactEvent::sendDone(ICQEvent *e)
{
  if (e->Command() != ICQ_CMDxTCP_START) return true;

  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  if (u->Away() && u->ShowAwayMsg())
  {
    gUserManager.DropUser(u);
    (void) new ShowAwayMsgDlg(NULL, NULL, m_nUin);
  }
  else
    gUserManager.DropUser(u);

  return true;
}


//-----UserSendContactEvent::setContact--------------------------------------
void UserSendContactEvent::setContact(unsigned long Uin, const QString&)
{
  ICQUser* u = gUserManager.FetchUser(Uin, LOCK_R);

  if(u != NULL)
  {
    (void) new CMMUserViewItem(u, lstContacts);

    gUserManager.DropUser(u);
  }
}


// -----------------------------------------------------------------------------

#include "usereventdlg.moc"
