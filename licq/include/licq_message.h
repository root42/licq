#ifndef MESSAGE_H
#define MESSAGE_H

#include <ctime>
#include <list>

#include "licq_buffer.h"
#include "licq_constants.h"
#include "licq_color.h"

typedef std::list<const char *> ConstFileList;

#define LICQ_PPID 0x4C696371  // "Licq"
#define EVENT_HEADER_SIZE  80

// Define some event flags, leave the 2 LSB's for the licq version
const unsigned long E_LICQxVER      = 0x0000FFFF;
const unsigned long E_DIRECT        = 0x00010000;
const unsigned long E_MULTIxREC     = 0x00020000;
const unsigned long E_URGENT        = 0x00040000;
const unsigned long E_CANCELLED     = 0x00080000;
const unsigned long E_ENCRYPTED     = 0x00100000;
const unsigned long E_UNKNOWN       = 0x80000000;

class ICQUser;


//=====CUserEvent===============================================================
class CUserEvent
{
public:
   CUserEvent(unsigned short _nSubCommand, unsigned short _nCommand,
              unsigned short _nSequence, time_t _tTime,
              unsigned long _nFlags, unsigned long _nConvoId = 0);
   CUserEvent(const CUserEvent *);
   virtual ~CUserEvent();

   virtual CUserEvent *Copy() = 0;
   const char *Text();
   const char *Description();
   time_t Time()  {  return m_tTime; }
   const char *LicqVersionStr();
   static const char *LicqVersionToString(unsigned long);
   unsigned short Sequence()  { return m_nSequence; }
   unsigned short Command()  { return m_nCommand; }
   unsigned short SubCommand()  { return m_nSubCommand; }
   int Id()  { return m_nId; }
   bool IsDirect()  { return m_nFlags & E_DIRECT; }
   bool IsMultiRec()  { return m_nFlags & E_MULTIxREC; }
   bool IsUrgent()    { return m_nFlags & E_URGENT; }
   bool IsCancelled() { return m_nFlags & E_CANCELLED; }
   bool IsLicq()  { return LicqVersion() != 0; };
   bool IsEncrypted()  { return m_nFlags & E_ENCRYPTED; };
   unsigned short LicqVersion()  { return m_nFlags & E_LICQxVER; }
   direction Direction()  {  return m_eDir; }
   CICQColor *Color() { return &m_sColor; }
   unsigned long ConvoId() { return m_nConvoId; }

   bool Pending() { return m_bPending; }
   void SetPending(bool b)  { m_bPending = b; }

protected:
   virtual void AddToHistory(ICQUser *, unsigned long, direction) = 0;
   int AddToHistory_Header(direction, char *);
   void AddToHistory_Flush(ICQUser *, unsigned long, char *);

   void SetDirection(direction d)  { m_eDir = d; }
   void Cancel() { m_nFlags |= E_CANCELLED; }
   void SetColor(unsigned long fore, unsigned long back)  { m_sColor.Set(fore, back); }
   void SetColor(CICQColor *p)  { m_sColor.Set(p); }

   void CopyBase(CUserEvent *);

   virtual void CreateDescription() = 0;
   static int s_nId;

   char          *m_szText;
   unsigned short m_nCommand;
   unsigned short m_nSubCommand;
   unsigned short m_nSequence;
   int            m_nId;
   time_t         m_tTime;
   unsigned long  m_nFlags;

   direction      m_eDir;
   bool           m_bPending;
   CICQColor      m_sColor;
   unsigned long  m_nConvoId;

friend class CICQDaemon;
friend class CMSN;
friend class CUserHistory;
};



//-----CEventMsg-------------------------------------------------------------
class CEventMsg : public CUserEvent
{
public:
   CEventMsg(const char *_szMessage, unsigned short _nCommand,
             time_t _tTime, unsigned long _nFlags, unsigned long _nConvoId = 0);
   virtual ~CEventMsg();

   virtual CEventMsg *Copy();
   const char *Message()  { return m_szMessage; }
   virtual void AddToHistory(ICQUser *, unsigned long, direction);

   static CEventMsg *Parse(char *sz, unsigned short nCmd, time_t nTime,
                           unsigned long nFlags, unsigned long nConvoId = 0);
protected:
   void CreateDescription();
   char *m_szMessage;
};


//-----CEventFile---------------------------------------------------------------
class CEventFile : public CUserEvent
{
public:
   CEventFile(const char *_szFilename, const char *_szFileDescription,
              unsigned long _nFileSize, ConstFileList &lFileList,
              unsigned short _nSequence, time_t _tTime,
              unsigned long _nFlags, unsigned long _nConovId = 0,
              unsigned long _nMsgID1 = 0, unsigned long _nMsgID2 = 0);
   virtual ~CEventFile();
   virtual CEventFile *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long,  direction);

   const char *Filename()  { return m_szFilename; }
   unsigned long FileSize()  {  return m_nFileSize; }
   const char *FileDescription() { return m_szFileDescription; }
   ConstFileList FileList() { return m_lFileList; }
   unsigned long *MessageID() { return m_nMsgID; }
protected:
   void CreateDescription();
   char *m_szFilename;
   char *m_szFileDescription;
   unsigned long m_nFileSize;
   ConstFileList m_lFileList;
   unsigned long m_nMsgID[2];
};


//-----CEventUrl----------------------------------------------------------------
class CEventUrl : public CUserEvent
{
public:
   CEventUrl(const char *_szUrl, const char *_szUrlDescription,
             unsigned short _nCommand, time_t _tTime,
             unsigned long _nFlags, unsigned long _nConvoId = 0);
   virtual ~CEventUrl();
   virtual CEventUrl *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
   const char *Url()  { return m_szUrl; }
   const char *Description()  { return m_szUrlDescription; }

   static CEventUrl *Parse(char *sz, unsigned short nCmd, time_t nTime,
     unsigned long nFlags, unsigned long nConvoId = 0);
protected:
   void CreateDescription();
   char *m_szUrl;
   char *m_szUrlDescription;
};


//-----CEventChat---------------------------------------------------------------
class CEventChat : public CUserEvent
{
public:
   CEventChat(const char *szReason, unsigned short nSequence, time_t tTime,
      unsigned long nFlags, unsigned long nConvoId = 0, unsigned long nMsgID1 = 0,
      unsigned long nMsgID2 = 0);
   CEventChat(const char *szReason, const char *szClients, unsigned short nPort,
      unsigned short nSequence, time_t tTime, unsigned long nFlags,
      unsigned long _nConvoId = 0, unsigned long nMsgID1 = 0, unsigned long nMsgID2 = 0);
  virtual ~CEventChat();
  virtual CEventChat *Copy();
  virtual void AddToHistory(ICQUser *, unsigned long, direction);
  const char *Reason()  { return m_szReason; }
  const char *Clients()  { return m_szClients; }
  unsigned short Port()   { return m_nPort; }
  unsigned long *MessageID() { return m_nMsgID; }
protected:
  void CreateDescription();
  char *m_szReason;
  char *m_szClients;
  unsigned short m_nPort;
  unsigned long m_nMsgID[2];
};


//-----CEventAdded--------------------------------------------------------------
class CEventAdded : public CUserEvent
{
public:
   CEventAdded(const char *_szId, unsigned long _nPPID, const char *_szAlias,
               const char *_szFirstName, const char *_szLastName, const char *_szEmail,
               unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   CEventAdded(unsigned long _nUin, const char *_szAlias, const char *_szFirstName,
               const char *_szLastName, const char *_szEmail,
               unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventAdded();
   virtual CEventAdded *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
   unsigned long Uin()  { return m_nUin; };
   char *IdString()     { return m_szId; }
   unsigned long PPID() { return m_nPPID; }
protected:
   void CreateDescription();
   unsigned long m_nUin;
   char *m_szId;
   unsigned long m_nPPID;
   char *m_szAlias;
   char *m_szFirstName;
   char *m_szLastName;
   char *m_szEmail;
};



//-----CEventAuthReq---------------------------------------------------------
class CEventAuthRequest : public CUserEvent
{
public:
   CEventAuthRequest(const char *_szId, unsigned long _nPPID, const char *_szAlias,
                     const char *_szFirstName, const char *_szLastName, const char *_szEmail,
                     const char *_szReason, unsigned short _nCommand, time_t _tTime,
                     unsigned long _nFlags);
   CEventAuthRequest(unsigned long _nUin, const char *_szAlias, const char *_szFirstName,
                 const char *_szLastName, const char *_szEmail, const char *_szReason,
                 unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventAuthRequest();
   virtual CEventAuthRequest *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
   unsigned long Uin()  { return m_nUin; };
   char *IdString()     { return m_szId; }
   unsigned long PPID() { return m_nPPID; }
protected:
   void CreateDescription();
   unsigned long m_nUin;
   char *m_szId;
   unsigned long m_nPPID;
   char *m_szAlias;
   char *m_szFirstName;
   char *m_szLastName;
   char *m_szEmail;
   char *m_szReason;
};


//-----CEventAuthGranted-------------------------------------------------------
class CEventAuthGranted : public CUserEvent
{
public:
   CEventAuthGranted(const char *_szId, unsigned long _nPPID, const char *_szMsg,
                     unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   CEventAuthGranted(unsigned long _nUin, const char *_szMessage,
              unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventAuthGranted();
   virtual CEventAuthGranted *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
   unsigned long Uin()  { return m_nUin; };
   char *IdString()     { return m_szId; }
   unsigned long PPID() { return m_nPPID; }
protected:
   void CreateDescription();
   unsigned long m_nUin;
   char *m_szId;
   unsigned long m_nPPID;
   char *m_szMessage;
};


//-----CEventAuthRefused------------------------------------------------------
class CEventAuthRefused : public CUserEvent
{
public:
   CEventAuthRefused(const char *_szId, unsigned long _nPPID, const char *_szMsg,
                     unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   CEventAuthRefused(unsigned long _nUin, const char *_szMessage,
              unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventAuthRefused();
   virtual CEventAuthRefused *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
   unsigned long Uin()  { return m_nUin; };
   char *IdString()     { return m_szId; }
   unsigned long PPID() { return m_nPPID; }
protected:
   void CreateDescription();
   unsigned long m_nUin;
   char *m_szId;
   unsigned long m_nPPID;
   char *m_szMessage;
};


//-----CEventWebPanel----------------------------------------------------------
class CEventWebPanel : public CUserEvent
{
public:
   CEventWebPanel(const char *_szName, char *_szEmail, const char *_szMessage,
                   unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventWebPanel();
   virtual CEventWebPanel *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
protected:
   void CreateDescription();
   char *m_szName;
   char *m_szEmail;
   char *m_szMessage;
};


//-----CEventEmailPager----------------------------------------------------------
class CEventEmailPager : public CUserEvent
{
public:
   CEventEmailPager(const char *_szName, char *_szEmail, const char *_szMessage,
                    unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventEmailPager();
   virtual CEventEmailPager *Copy();
   virtual void AddToHistory(ICQUser *, unsigned long, direction);
protected:
   void CreateDescription();
   char *m_szName;
   char *m_szEmail;
   char *m_szMessage;
};


//-----CEventContactList----------------------------------------------------------
class CContact
{
public:
  CContact(const char *s, unsigned long n, const char *a);
  CContact(unsigned long n, const char *a);
  ~CContact();

  unsigned long Uin() { return m_nUin; }
  const char *Alias() { return m_szAlias; }
  char *IdString()    { return m_szId; }
  unsigned long PPID(){ return m_nPPID; }
protected:
  unsigned long m_nUin;
  char *m_szAlias;
  char *m_szId;
  unsigned long m_nPPID;
};
typedef std::list<CContact *> ContactList;


class CEventContactList : public CUserEvent
{
public:
  CEventContactList(ContactList &cl, bool bDeep, unsigned short nCommand,
     time_t tTime, unsigned long nFlags);
  virtual ~CEventContactList();
  virtual CEventContactList *Copy();
  virtual void AddToHistory(ICQUser *, unsigned long, direction);

  const ContactList &Contacts() { return m_vszFields; }

  static CEventContactList *Parse(char *sz, unsigned short nCmd, time_t nTime, unsigned long nFlags);
protected:
  void CreateDescription();
  ContactList m_vszFields;
};


//-----CEventSms---------------------------------------------------------------
class CEventSms : public CUserEvent
{
public:
   CEventSms(const char *_szNumber, const char *_szMessage,
             unsigned short _nCommand, time_t _tTime, unsigned long _nFlags);
   virtual ~CEventSms();
   virtual CEventSms *Copy();
   const char *Number()  { return m_szNumber; }
   const char *Message()  { return m_szMessage; }
   virtual void AddToHistory(ICQUser *, unsigned long, direction);

   static CEventSms *Parse(char *sz, unsigned short nCmd, time_t nTime, unsigned long nFlags);
protected:
   void CreateDescription();
   char *m_szNumber;
   char *m_szMessage;
};

//-----CEventServerMessage-----------------------------------------------------
class CEventServerMessage : public CUserEvent
{
public:
  CEventServerMessage(const char *_szName, const char *_szEmail,
                      const char *_szMessage, time_t _tTime);
  virtual ~CEventServerMessage();
  virtual CEventServerMessage *Copy();
  virtual void AddToHistory(ICQUser *, unsigned long, direction);

  static CEventServerMessage *Parse(char *, unsigned short, time_t, unsigned long);

protected:
  void CreateDescription();

  char *m_szName,
       *m_szEmail,
       *m_szMessage;
};

//-----CEventEmailAlert-----------------------------------------------------
class CEventEmailAlert : public CUserEvent
{
public:
  CEventEmailAlert(const char *_szName, const char *_szEmail,
                   const char *_szTo, const char *_szSubject, time_t _tTime,
                   const char *_szMSPAuth = 0, const char *_szSID = 0,
                   const char *_szKV = 0, const char *_szId = 0,
                   const char *_szPostURL = 0, const char *_szMsgURL = 0,
                   const char *_szCreds = 0, unsigned long _nSessionLength = 0);
  virtual ~CEventEmailAlert();
  virtual CEventEmailAlert *Copy();
  virtual void AddToHistory(ICQUser *, unsigned long, direction);

  char *From()    { return m_szName; }
  char *To()      { return m_szTo; }
  char *Email()   { return m_szEmail; }
  char *Subject() { return m_szSubject; }

  char *MSPAuth() { return m_szMSPAuth; }
  char *SID()     { return m_szSID; }
  char *KV()      { return m_szKV; }
  char *Id()      { return m_szId; }
  char *PostURL() { return m_szPostURL; }
  char *MsgURL()  { return m_szMsgURL; }
  char *Creds()   { return m_szCreds; }
  unsigned long SessionLength() { return m_nSessionLength; }

protected:
  void CreateDescription();

  // Info
  char *m_szName,
       *m_szTo,
       *m_szEmail,
       *m_szSubject;

  // For Licq to view an MSN email
  char *m_szMSPAuth,
       *m_szSID,
       *m_szKV,
       *m_szId,
       *m_szPostURL,
       *m_szMsgURL,
       *m_szCreds;
  unsigned long m_nSessionLength;
};
//-----CEventPlugin------------------------------------------------------------
class CEventPlugin : public CUserEvent
{
public:
  CEventPlugin(const char *sz, unsigned short nSubCommand,
     time_t tTime, unsigned long nFlags);
  ~CEventPlugin();
  virtual CEventPlugin *Copy();
  virtual void AddToHistory(ICQUser *, unsigned long, direction);
protected:
  void CreateDescription();
  char *m_sz;
};


//-----CEventUnknownSysMsg-----------------------------------------------------
class CEventUnknownSysMsg : public CUserEvent
{
public:
#if 0
  CEventUnknownSysMsg(unsigned short _nSubCommand,
                unsigned short _nCommand, const char *_szId, unsigned long _nPPID,
                const char *_szMsg,
                time_t _tTime, unsigned long _nFlags);
#endif
  CEventUnknownSysMsg(unsigned short _nSubCommand,
                unsigned short _nCommand, unsigned long _nUin,
                const char *_szMsg,
                time_t _tTime, unsigned long _nFlags);
  ~CEventUnknownSysMsg();
  virtual CEventUnknownSysMsg *Copy();
  virtual void AddToHistory(ICQUser *, unsigned long, direction);
protected:
   void CreateDescription();
   unsigned long m_nUin;
   char *m_szMsg;
#if 0
   char *m_szId;
   unsigned long m_nPPID;
#endif
};

#endif
