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

// written by Jon Keating <jon@licq.org>

#include "licq_icqd.h"
#include "licq_user.h"
#include "licq_protoplugin.h"

#include "msn.h"

char *LProto_Name()
{
  static char szName[] = "MSN";
  return szName;
}

char *LProto_Version()
{
  static char szVersion[] = "0.1";
  return szVersion;
}

const char *LProto_Description()
{
  static char szDesc[] = "MSN Protocol Plugin";
  return szDesc;
}

unsigned long LProto_Capabilities()
{
  return 0;
}

void LProto_Main(CICQDaemon *_pDaemon)
{
  int nPipe = _pDaemon->RegisterProtoPlugin();  

  CMSN *pMSN = new CMSN(_pDaemon, nPipe);
  pMSN->Run();

  delete pMSN;
}

char *LProto_Id()
{
  static char szId[] = "MSN_";
  return szId;
}

bool LProto_Init()
{
  return true;
}
