/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2004-2011 Licq developers
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

#include "msn.h"
#include "msnpacket.h"

#include <licq/plugin/pluginmanager.h>
#include <licq/pluginsignal.h>
#include <licq/socket.h>
#include <licq/logging/log.h>

#include <cstring>
#include <string>
#include <list>
#include <vector>

using namespace std;
using Licq::UserId;
using Licq::gLog;

void CMSN::ProcessSSLServerPacket(CMSNBuffer &packet)
{
  // Did we receive the entire packet?
  // I don't like doing this, is there a better way to see
  // if we get the entire packet at the socket level?
  // I couldn't find anything in the HTTP RFC about this packet
  // being broken up without any special HTTP headers
  if (m_pSSLPacket == 0)
    m_pSSLPacket = new CMSNBuffer(packet);

  char *ptr = packet.getDataStart() + packet.getDataSize() - 4;
  int x = memcmp(ptr, "\x0D\x0A\x0D\x0A", 4);
  if (m_pSSLPacket->getDataSize() != packet.getDataSize())
    *m_pSSLPacket += packet;
  
  if (x)  return;
  
  // Now process the packet
  char cTmp = 0;
  string strFirstLine = "";
  
  *m_pSSLPacket >> cTmp;
  while (cTmp != '\r')
  {
    strFirstLine += cTmp;
    *m_pSSLPacket >> cTmp;
  }
  
  *m_pSSLPacket >> cTmp; // skip \n as well
  
  // Authenticated
  if (strFirstLine == "HTTP/1.1 200 OK")
  {
    m_pSSLPacket->ParseHeaders();
    const char* fromPP = strstr(m_pSSLPacket->GetValue("Authentication-Info").c_str(), "from-PP=");
    char *tag;

    if (fromPP == 0)
      tag = m_szCookie;
    else
    {
      fromPP+= 9; // skip to the tag
      const char* endTag = strchr(fromPP, '\'');
      tag = strndup(fromPP, endTag - fromPP); // Thanks, this is all we need
    }

    CMSNPacket *pReply = new CPS_MSNSendTicket(tag);
    SendPacket(pReply);
    free(tag);
    m_szCookie = 0;
  }
  else if (strFirstLine == "HTTP/1.1 302 Found")
  {
    m_pSSLPacket->ParseHeaders();
    string strAuthHeader = m_pSSLPacket->GetValue("WWW-Authenticate");
    string strToSend = strAuthHeader.substr(strAuthHeader.find(" ") + 1, strAuthHeader.size() - strAuthHeader.find(" "));

    string strLocation = m_pSSLPacket->GetValue("Location");
    string::size_type pos = strLocation.find("/", 9);
    if (pos != string::npos)
    {
      string strHost = strLocation.substr(8, pos - 8);
      string strParam = strLocation.substr(pos, strLocation.size() - pos);
      gSocketMan.CloseSocket(m_nSSLSocket, false, true);
      m_nSSLSocket = -1;
      delete m_pSSLPacket;
      m_pSSLPacket = 0;

      gLog.info("Redirecting to %s:443", strHost.c_str());
      MSNAuthenticateRedirect(strHost, strToSend);
      return;
    }
    else
      gLog.error("Malformed location header");
  }
  else if (strFirstLine == "HTTP/1.1 401 Unauthorized")
  {
    gLog.error("Invalid password");
    Licq::gPluginManager.pushPluginSignal(new Licq::PluginSignal(
        Licq::PluginSignal::SignalLogoff,
        Licq::PluginSignal::LogoffPassword, UserId(m_szUserName, MSN_PPID)));
  }
  else
  {
    gLog.error("Unknown sign in error");
  }
  
  gSocketMan.CloseSocket(m_nSSLSocket, false, true);
  m_nSSLSocket = -1;
  delete m_pSSLPacket;
  m_pSSLPacket = 0;
}

void CMSN::ProcessNexusPacket(CMSNBuffer &packet)
{
  bool bNew = false;
  if (m_pNexusBuff == 0)
  {
    m_pNexusBuff = new CMSNBuffer(packet);
    bNew = true;
  }

  char *ptr = packet.getDataStart() + packet.getDataSize() - 4;
  int x = memcmp(ptr, "\x0D\x0A\x0D\x0A", 4);
  if (x) return;
  else if (!bNew) *m_pNexusBuff += packet;

  char cTmp = 0;

  while (cTmp != '\r')
    *m_pNexusBuff >> cTmp;
  *m_pNexusBuff >> cTmp; // skip the \n as well

  m_pNexusBuff->ParseHeaders();

  const char* szLogin = strstr(m_pNexusBuff->GetValue("PassportURLs").c_str(), "DALogin=");
  szLogin += 8; // skip to the tag
  //char *szEndURL = strchr(szLogin, '/');
  //char *szServer = strndup(szLogin, szEndURL - szLogin); // this is all we need
  //char *szEnd = strchr(szLogin, ',');
  //char *szURL = strndup(szEndURL, szEnd - szEndURL);

  MSNAuthenticate(m_szCookie);
}

void CMSN::MSNGetServer()
{
  UserId myOwnerId(m_szUserName, MSN_PPID);
  Licq::TCPSocket* sock = new Licq::TCPSocket(myOwnerId);
  if (!sock->connectTo(string("nexus.passport.com"), 443))
  {
    delete sock;
    return;
  }
  
  if (!sock->SecureConnect())
  {
    delete sock;
    return;
  }

  gSocketMan.AddSocket(sock);
  m_nNexusSocket = sock->Descriptor();
  CMSNPacket *pHello = new CPS_MSNGetServer();
  sock->SSLSend(pHello->getBuffer());
  gSocketMan.DropSocket(sock);
}

void CMSN::MSNAuthenticateRedirect(const string &strHost, const string& /* strParam */)
{
  UserId myOwnerId(m_szUserName, MSN_PPID);
  Licq::TCPSocket* sock = new Licq::TCPSocket(myOwnerId);
  gLog.info("Authenticating to %s:%d", strHost.c_str(), 443);
  if (!sock->connectTo(strHost, 443))
  {
    gLog.error("Connection to %s failed", strHost.c_str());
    delete sock;
    return;
  }

  if (!sock->SecureConnect())
  {
    gLog.error("SSL connection failed");
    delete sock;
    return;
  }

  gSocketMan.AddSocket(sock);
  m_nSSLSocket = sock->Descriptor();
  CMSNPacket *pHello = new CPS_MSNAuthenticate(m_szUserName, myPassword.c_str(), m_szCookie);
  sock->SSLSend(pHello->getBuffer());
  gSocketMan.DropSocket(sock);
}

void CMSN::MSNAuthenticate(char *szCookie)
{
  UserId myOwnerId(m_szUserName, MSN_PPID);
  string server = "loginnet.passport.com";
  Licq::TCPSocket* sock = new Licq::TCPSocket(myOwnerId);
  gLog.info("Authenticating to %s:%d", server.c_str(), 443);
  if (!sock->connectTo(server, 443))
  {
    gLog.error("Connection to %s failed", server.c_str());
    delete sock;
    free(szCookie);
    szCookie = 0;
    return;
  }
  
  if (!sock->SecureConnect())
  {
    gLog.error("SSL connection failed");
    free(szCookie);
    szCookie = 0;
    delete sock;
    return;
  }
  
  gSocketMan.AddSocket(sock);
  m_nSSLSocket = sock->Descriptor();
  CMSNPacket *pHello = new CPS_MSNAuthenticate(m_szUserName, myPassword.c_str(), szCookie);
  sock->SSLSend(pHello->getBuffer());
  gSocketMan.DropSocket(sock);
}

