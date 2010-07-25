#include "msn.h"
#include "msnevent.h"

#include <licq/logging/log.h>

#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <licq/contactlist/user.h>
#include <licq/pluginsignal.h>

using namespace std;
using Licq::UserId;
using Licq::gLog;

CMSNDataEvent::CMSNDataEvent(CMSN *p)
{
  m_pMSN = p;
  m_nSocketDesc = -1;
  m_nEvent = 0;
  m_strId = "";
  m_eState = STATE_WAITING_ACK;
  m_nFileDesc = -1;
  m_strFileName = "";
  m_nFilePos = 0;
  m_nBytesTransferred = 0;
  m_nStartTime = 0;
  m_nSessionId = 0;
  m_nBaseId = 0;
  m_nDataSize[0] = 0;
  m_nDataSize[1] = 0;
  m_strFromId = "";
  m_strCallId = "";
}

CMSNDataEvent::CMSNDataEvent(unsigned long _nEvent, unsigned long _nSessionId,
    unsigned long _nBaseId, const Licq::UserId& userId,
			     const string &_strFromId, const string &_strCallId,
                             CMSN *p)
{
  m_pMSN = p;
  m_nSocketDesc = -1;
  m_nEvent = _nEvent;
  m_strId = userId.accountId();
  m_eState = STATE_WAITING_ACK;
  m_nFileDesc = -1;
  {
    Licq::UserReadGuard u(userId);
    m_strFileName = u->pictureFileName();
  }
  m_nFilePos = 0;
  m_nBytesTransferred = 0;
  m_nStartTime = 0;
  m_nSessionId = _nSessionId;
  m_nBaseId = _nBaseId;
  m_nDataSize[0] = 0;
  m_nDataSize[1] = 0;
  m_strFromId = _strFromId;
  m_strCallId = _strCallId;
}

CMSNDataEvent::~CMSNDataEvent()
{
  if (m_nSocketDesc)
  {
    Licq::INetSocket* s = gSocketMan.FetchSocket(m_nSocketDesc);
    gSocketMan.DropSocket(s);
    gSocketMan.CloseSocket(m_nSocketDesc);
  }

  if (m_nFileDesc)
    close(m_nFileDesc);
}

int CMSNDataEvent::ProcessPacket(CMSNBuffer *p)
{
  unsigned long nSessionId,
    nIdentifier,
    nOffset[2],
    nDataSize[2],
    nLen,
    nFlag,
    nAckId,
    nAckUniqueId,
    nAckSize[2];

  (*p) >> nSessionId >> nIdentifier >> nOffset[0] >> nOffset[1]
    >> nDataSize[0] >> nDataSize[1] >> nLen >> nFlag >> nAckId
    >> nAckUniqueId >> nAckSize[0] >> nAckSize[1];

//  printf("%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
//	 nSessionId, nIdentifier, nOffset[0], nOffset[1],
//	 nDataSize[0], nDataSize[1], nLen, nFlag, nAckId,
//	 nAckUniqueId, nAckSize[0], nAckSize[1]);

  switch (m_eState)
  {
    case STATE_WAITING_ACK:
    {
      if (m_nSessionId == 0)
      {
	if (nFlag == 0x00000002)
	{
	  gLog.info("%sDisplay Picture: Ack received\n", L_MSNxSTR);
	}
	else if (nFlag == 0)
	{
	  if (nSessionId)
	    m_nSessionId = nSessionId;
	  else
	  {
	    // Get it from the body
	    char szStatusLine[128];
	    int nToRead = strstr(p->getDataPosRead(), "\r\n")+2-p->getDataPosRead();
	    if (nToRead > 128)
	    {
	      gLog.warning("%sDisplay Picture: Received unusually long status line, aborting\n", L_WARNxSTR);
	      // close connection
	      return -1;
	    }
	    p->UnpackRaw(szStatusLine, nToRead);
	    string strStatus(szStatusLine);
	    if (strStatus != "MSNSLP/1.0 200 OK\r\n")
	    {
	      gLog.error("%sDisplay Picture: Encountered an error before the session id was received: %s", L_ERRORxSTR, szStatusLine);
	      // close connection
	      return -1;
	    }
	    
	    p->ParseHeaders();
	    string strLen = p->GetValue("Content-Length");
	    int nConLen = atoi(strLen.c_str());
	    if (nConLen)
	    {
	      p->SkipRN();
	      p->ParseHeaders();
	      string strSessId = p->GetValue("SessionID");
	      m_nSessionId = strtoul(strSessId.c_str(), (char **)NULL, 10);
	    }
	  }

	  gLog.info("%sDisplay Picture: Session Id received (%ld)\n",
		    L_MSNxSTR, m_nSessionId);
	  CMSNPacket *pAck = new CPS_MSNP2PAck(m_strId.c_str(), m_nSessionId,
					       m_nBaseId-3, nIdentifier, nAckId,
					       nDataSize[1], nDataSize[0]);
          m_pMSN->Send_SB_Packet(UserId(m_strId, MSN_PPID), pAck, m_nSocketDesc);
	  m_eState = STATE_GOT_SID;
	}
      }


      break;
    }

    case STATE_GOT_SID:
    {
      CMSNPacket *pAck = new CPS_MSNP2PAck(m_strId.c_str(), m_nSessionId,
					   m_nBaseId-2, nIdentifier, nAckId,
					   nDataSize[1], nDataSize[0]);
      m_pMSN->Send_SB_Packet(UserId(m_strId, MSN_PPID), pAck, m_nSocketDesc);
      m_eState = STATE_RECV_DATA;

      gLog.info("%sDisplay Picture: Got data start message (%ld)\n",
		L_MSNxSTR, m_nSessionId);

      m_nFileDesc = open(m_strFileName.c_str(), O_WRONLY | O_CREAT, 00600);
      if (!m_nFileDesc)
      {
	gLog.error("%sUnable to create a file in your licq directory, check disk space.\n",
		   L_ERRORxSTR);
	return -1;
      }

      break;
    }

    case STATE_RECV_DATA:
    {
      // Picture data has the 0x20 Flag, so only set data when we get the first picture data packet
      if (m_nDataSize[0] == 0 && nFlag == 0x00000020)
      {
	m_nDataSize[0] = nDataSize[0];
	m_nDataSize[1] = nDataSize[1];
	gLog.info("%sDisplay Picture: Expecting file of size %ld (Id: %ld).\n",
		  L_MSNxSTR, m_nDataSize[0], m_nSessionId);
      }

      if (nFlag != 0x00000020)
      {
        gLog.info("%sDisplay Picture: Skipping packet without 0x20 flag.\n", L_MSNxSTR);
        break;
      }

      ssize_t nWrote = write(m_nFileDesc, p->getDataPosRead(), nLen);
      if (nWrote != (ssize_t)nLen)
      {
	gLog.error("%sDisplay Picture: Tried to write %ld, but wrote %ld (Id: %ld).\n",
		   L_MSNxSTR, nLen, (long)nWrote, m_nSessionId);
      }

      m_nBytesTransferred += nLen;

      gLog.info("%sDisplay Picture: Wrote %ld of %ld bytes.\n",
          L_MSNxSTR, m_nBytesTransferred, m_nDataSize[0]);

      if (m_nBytesTransferred >= m_nDataSize[0])
      {
	if (m_nBytesTransferred == m_nDataSize[0])
	{
	  gLog.info("%sDisplay Picture: Successfully completed (%s).\n",
		    L_MSNxSTR, m_strFileName.c_str());
	}
	else
	{
	  gLog.error("%sDisplay Picture: Too much data received, ending transfer.\n",
		     L_MSNxSTR);
	}
	close(m_nFileDesc);
	m_nFileDesc = -1;
	m_eState = STATE_FINISHED;

        {
          Licq::UserWriteGuard u(UserId(m_strId, MSN_PPID));
          if (u.isLocked())
          {
            u->SetPicturePresent(true);
            m_pMSN->pushPluginSignal(new Licq::PluginSignal(Licq::PluginSignal::SignalUser,
                Licq::PluginSignal::UserPicture, u->id()));
          }
        }

	// Ack that we got the data
	CMSNPacket *pAck = new CPS_MSNP2PAck(m_strId.c_str(), m_nSessionId,
					     m_nBaseId-1, nIdentifier, nAckId,
					     nDataSize[1], nDataSize[0]);
        m_pMSN->Send_SB_Packet(UserId(m_strId, MSN_PPID), pAck, m_nSocketDesc);

        // Send a bye command
        CMSNPacket *pBye = new CPS_MSNP2PBye(m_strId.c_str(),
					     m_strFromId.c_str(),
					     m_strCallId.c_str(),
	  				     m_nBaseId, nAckId,
					     nDataSize[1], nDataSize[0]);
        m_pMSN->Send_SB_Packet(UserId(m_strId, MSN_PPID), pBye, m_nSocketDesc);
	return 0;
      }

      break;
    }

    case STATE_FINISHED:
    {
      // Don't have to send anything back, just return and close the socket.
      gLog.info("%s Display Picture: closing connection with %s\n", L_MSNxSTR,
                m_strId.c_str());
      return 10;
      break;
    }

    case STATE_CLOSED:
    {
      break;
    }
  }

  return 0;
}
