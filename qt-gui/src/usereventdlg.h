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

#include <qwidget.h>
#include <list>

#include "licq_color.h"
#include "licq_filetransfer.h"

class QTabWidget;
class QBoxLayout;
class QGroupBox;
class QVGroupBox;
class QComboBox;
class QBoxLayout;
class QCheckBox;
class QPushButton;
class QTimer;
class QSplitter;
class QLineEdit;
class QGroupBox;
class QButtonGroup;
class QListViewItem;
class QLabel;
class QTextCodec;

class MLEditWrap;
class MsgView;
class CInfoField;
class CICQDaemon;
class CSignalManager;
class CMainWindow;
class CUserEvent;
class CMMUserView;
class CEButton;
class CMessageViewWidget;
class MLView;

/* ----------------------------------------------------------------------------- */
class UserEventTabDlg : public QWidget
{
  Q_OBJECT
public:
  UserEventTabDlg(QWidget *parent = 0, const char *name = 0);
  ~UserEventTabDlg();

  void addTab(UserEventCommon *tab, int index = -1);
  void removeTab(QWidget *tab);
  void selectTab(QWidget *tab);
  void replaceTab(QWidget *oldTab, UserEventCommon *newTab);
  bool tabIsSelected(QWidget *tab);
  bool tabExists(QWidget *tab);
  void updateTabLabel(ICQUser *u);

private:
  QTabWidget *tabw;
  void updateTitle(QWidget *tab);
  void clearEvents(QWidget *tab);

public slots:
  void slot_currentChanged(QWidget *tab);
  void moveLeft();
  void moveRight();

signals:
  void signal_done();
};
/* ----------------------------------------------------------------------------- */

class UserEventCommon : public QWidget
{
  Q_OBJECT
public:
  UserEventCommon(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0,
                  const char *name = 0);
  virtual ~UserEventCommon();

  char *Id()  { return m_szId; }
  unsigned long PPID()  { return m_nPPID; }

  void AddEventTag(unsigned long n)  { if (n) m_lnEventTag.push_back(n); }
  
  enum type {
  	UC_MESSAGE,
  	UC_URL,
  	UC_CHAT,
  	UC_FILE,
  	UC_CONTACT,
  	UC_SMS
  };
protected:
  QTextCodec *codec;
  bool m_bOwner;
  char *m_szId;
  unsigned long m_nPPID;
  QBoxLayout* top_lay, *top_hlay;
  CICQDaemon *server;
  CMainWindow *mainwin;
  CSignalManager *sigman;
  std::list<unsigned long> m_lnEventTag;
  QWidget *mainWidget;
  QPushButton *btnHistory, *btnInfo, *btnEncoding, *btnSecure, *btnForeColor, *btnBackColor;
  QPopupMenu * popupEncoding;

  CInfoField *nfoStatus, *nfoTimezone;
  time_t m_nRemoteTimeOffset;
  QTimer *tmrTime;
  bool m_bDeleteUser;
  QString m_sBaseTitle, m_sProgressMsg;

  // ID of the higest event we've processed. Helps determine
  // which events we already processed in the ctor.
  int m_highestEventId;

  virtual void UserUpdated(CICQSignal *, ICQUser *) = 0;
  void SetGeneralInfo(ICQUser *);

protected slots:
  void slot_connectsignal();
  void slot_userupdated(CICQSignal *);
  void slot_updatetime();
  void showHistory();
  void showUserInfo();
  void slot_usermenu() { gMainWindow->SetUserMenuUser(m_szId, m_nPPID); }
  void slot_security();
  void slot_setEncoding(int encodingMib);

signals:
  void finished(const char *, unsigned long);
  void encodingChanged();
  void viewurl(QWidget*, QString);
};


/* ----------------------------------------------------------------------------- */

class UserViewEvent : public UserEventCommon
{
  Q_OBJECT
public:

  UserViewEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserViewEvent();

protected:
  // widgets

  QSplitter *splRead;
  MLView *mlvRead;
  MsgView *msgView;
  CUserEvent *m_xCurrentReadEvent;
  QCheckBox* chkAutoClose;
  QPushButton *btnRead2, *btnRead3, *btnRead4, *btnReadNext;
  CEButton *btnRead1, *btnClose;

  // The currently displayed message in decoded (Unicode) form.
  QString m_messageText;

  void generateReply();
  void sendMsg(QString txt);
  void updateNextButton();
  virtual void UserUpdated(CICQSignal *, ICQUser *);

protected slots:
  void slot_close();
  void slot_autoClose();
  void slot_msgtypechanged(UserSendCommon *, UserSendCommon *);
  void slot_btnRead1();
  void slot_btnRead2();
  void slot_btnRead3();
  void slot_btnRead4();
  void slot_btnReadNext();
  void slot_printMessage(QListViewItem*);
  void slot_clearEvent();
  void slot_sentevent(ICQEvent *);
  void slot_setEncoding();
};


/* ----------------------------------------------------------------------------- */

class UserSendCommon : public UserEventCommon
{
  Q_OBJECT
public:

  UserSendCommon(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                 const char *_szId, unsigned long _nPPID, QWidget *parent = 0,
                 const char *name = 0);
  virtual ~UserSendCommon();

  void setText(const QString& txt);
#if QT_VERSION >= 300
  virtual void windowActivationChange(bool oldActive);
#endif
  int clearDelay;

signals:
  void autoCloseNotify();
  void updateUser(CICQSignal*);
  void signal_msgtypechanged(UserSendCommon *, UserSendCommon *);

protected:
  CMessageViewWidget *mleHistory;
  QSplitter * splView;
  QCheckBox *chkSendServer, *chkUrgent, *chkMass;
  QPushButton *btnSend, *btnCancel;
  QGroupBox *grpMR;
  QButtonGroup *grpCmd;
  QComboBox* cmbSendType;
  CMMUserView *lstMultipleRecipients;
  MLEditWrap *mleSend;
  CICQColor icqColor;

  void RetrySend(ICQEvent *e, bool bOnline, unsigned short nLevel);
  virtual void UserUpdated(CICQSignal *, ICQUser *);
  virtual bool sendDone(ICQEvent *) = 0;
  bool checkSecure();

  virtual void resetSettings() = 0;
  virtual bool isType(int) = 0;

protected slots:
  virtual void sendButton();
  virtual void sendDone_common(ICQEvent *);

  void cancelSend();
  void massMessageToggled(bool);
  void slot_resettitle();
  void slot_SetForegroundICQColor();
  void slot_SetBackgroundICQColor();
  void trySecure();
  void slot_ClearNewEvents();

public slots:
  void changeEventType(int);

private:
  int tmpWidgetWidth;
  // In QT >= 3.1 we can use QWidget::isShown(),
  // but for now we implement it ourselves:
  bool m_bGrpMRIsVisible; // Remembers visibility of grpMR Widget
};


/* ----------------------------------------------------------------------------- */

class UserSendMsgEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendMsgEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  unsigned long _nUin, QWidget* parent = 0);
  UserSendMsgEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserSendMsgEvent();

protected:
  virtual bool sendDone(ICQEvent *);
  virtual void resetSettings();
  virtual bool isType(int id) { return (id == UC_MESSAGE); }

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendUrlEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendUrlEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserSendUrlEvent();

  void setUrl(const QString& url, const QString& description);

protected:
  QLabel *lblItem;
  CInfoField *edtItem;
  virtual bool sendDone(ICQEvent *);
  virtual void resetSettings();
  virtual bool isType(int id) { return (id == UC_URL); }

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendFileEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendFileEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserSendFileEvent();

  void setFile(const QString& file, const QString& description);

protected:
  QLabel *lblItem;
  CInfoField *edtItem;
  QPushButton *btnBrowse, *btnEdit;
  ConstFileList m_lFileList;
  virtual bool sendDone(ICQEvent*);
  virtual void resetSettings();
  virtual bool isType(int id) { return (id == UC_FILE); }

protected slots:
  void browseFile();
  void editFileList();
  void slot_filedel(unsigned);
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendChatEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendChatEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserSendChatEvent();

protected:
  QLabel *lblItem;
  CInfoField *edtItem;
  QPushButton *btnBrowse;
  QString m_szMPChatClients;
  unsigned short m_nMPChatPort;
  virtual bool sendDone(ICQEvent *);
  virtual void resetSettings();
  virtual bool isType(int id) { return (id == UC_CHAT); }

protected slots:
  virtual void sendButton();

  void InviteUser();
};


/* ----------------------------------------------------------------------------- */

class UserSendContactEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendContactEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                       unsigned long _nUin, QWidget* parent = 0);
  UserSendContactEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserSendContactEvent();

  void setContact(const char *, unsigned long, const QString& alias);

protected:
  CMMUserView *lstContacts;

  virtual bool sendDone(ICQEvent *);
  virtual void resetSettings();
  virtual bool isType(int id) { return (id == UC_CONTACT); }

protected slots:
  virtual void sendButton();
};


/* ----------------------------------------------------------------------------- */

class UserSendSmsEvent : public UserSendCommon
{
  Q_OBJECT
public:

  UserSendSmsEvent(CICQDaemon *s, CSignalManager *theSigMan, CMainWindow *m,
                  const char *_szId, unsigned long _nPPID, QWidget *parent = 0);
  virtual ~UserSendSmsEvent();

protected:
  QLabel *lblNumber;
  CInfoField *nfoNumber;
  QLabel *lblCount;
  CInfoField *nfoCount;

  virtual bool sendDone(ICQEvent *);
  virtual void resetSettings();
  virtual bool isType(int id) { return (id == UC_SMS); }

protected slots:
  virtual void sendButton();

  void slot_count();
};

/* ----------------------------------------------------------------------------- */

#endif
