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

#ifndef USEREVENTDLG_H
#define USEREVENTDLG_H

#include <qdialog.h>

class QVGroupBox;
class QBoxLayout;
class QCheckBox;
class QPushButton;
class QTimer;
class QSplitter;
class QListViewItem;

class MLEditWrap;
class MsgView;
class CInfoField;
class CICQDaemon;
class CICQEventTag;
class CSignalManager;
class CMainWindow;
class CUserEvent;
class CMMUserView;

/* ----------------------------------------------------------------------------- */

class UserEventCommon : public QDialog
{
  Q_OBJECT
public:

  UserEventCommon(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0, const char* name =0);
  virtual ~UserEventCommon();

protected:
  bool m_bOwner;
  unsigned long m_nUin;
  CICQDaemon *server;
  CMainWindow *mainwin;
  CSignalManager *sigman;
  CICQEventTag *icqEventTag;
  QVGroupBox *mainWidget;

  CInfoField *nfoStatus, *nfoTimezone;
  time_t m_nRemoteTimeOffset;
  QTimer *tmrTime;

protected slots:
  void slot_updatetime();

signals:
  void finished(unsigned long);
};


/* ----------------------------------------------------------------------------- */

class UserViewEvent : public UserEventCommon
{
  Q_OBJECT
public:

  UserViewEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
  virtual ~UserViewEvent();

protected:
  QSplitter *splRead;
  MLEditWrap *mleRead;
  MsgView *msgView;
  CUserEvent *m_xCurrentReadEvent;
  QPushButton *btnRead1, *btnRead2, *btnRead3, *btnRead4, *btnReadNext;

  void generateReply();
  void sendMsg(QString txt);

protected slots:
  void slot_btnRead1();
  void slot_btnRead2();
  void slot_btnRead3();
  void slot_btnRead4();
  void slot_btnReadNext();
  void slot_printMessage(QListViewItem*);
};


/* ----------------------------------------------------------------------------- */

class UserSendCommon : public UserEventCommon
{
  Q_OBJECT
public:

  UserSendCommon(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                 unsigned long _nUin, QWidget* parent = 0, const char* name=0);
  virtual ~UserSendCommon();

protected:
  MLEditWrap *mleSend;
  QCheckBox *chkSendServer, *chkSpoof, *chkUrgent, *chkMass;
  QPushButton *btnSend;
  QLineEdit *edtSpoof;
  QVGroupBox *grpMR;
  QButtonGroup *grpCmd;
  QString m_szMPChatClients;
  unsigned short m_nMPChatPort;
  CMMUserView *lstMultipleRecipients;

  void RetrySend(ICQEvent *e, bool bOnline, unsigned short nLevel);

protected slots:
  virtual void sendButton();
  virtual void sendDone(ICQEvent*);
};


/* ----------------------------------------------------------------------------- */

class UserSendMsgEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendMsgEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
  virtual ~UserSendMsgEvent();

  void setText(QString txt);
protected:

protected slots:
  virtual void sendButton();
  virtual void sendDone(ICQEvent*);
};


/* ----------------------------------------------------------------------------- */

class UserSendUrlEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendUrlEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
  virtual ~UserSendUrlEvent();

protected:
  CInfoField *edtItem;

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendFileEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendFileEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
  virtual ~UserSendFileEvent();

protected:
  CInfoField *edtItem;

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendChatEvent : public UserSendCommon
{
  Q_OBJECT
public:

    UserSendChatEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
    virtual ~UserSendChatEvent();

protected:

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendContactEvent : public UserSendCommon
{
  Q_OBJECT
public:

    UserSendContactEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
    virtual ~UserSendContactEvent();

protected:

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

#endif
