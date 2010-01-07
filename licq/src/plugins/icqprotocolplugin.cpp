/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2010 Erik Johansson <erijo@licq.org>
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

#include "licq_icqd.h"
#include "licq/licqversion.h"
#include "licq/pluginmanager.h"

#define LProto_Name LProto_icq_Name
#define LProto_Version LProto_icq_Version
#define LProto_PPID LProto_icq_PPID
#define LProto_Init LProto_icq_Init
#define LProto_SendFuncs LProto_icq_SendFuncs
#define LProto_Main LProto_icq_Main
#define LP_Id LProto_icq_Id
#define LPproto_Exit LProto_icq_Exit
#define LProto_Main_tep LProto_icq_Main_tep

#include "licq_protoplugin.h"

char* LProto_icq_Name()
{
  static char name[] = "ICQ";
  return name;
}

char* LProto_icq_Version()
{
  static char version[] = LICQ_VERSION_STRING;
  return version;
}

char* LProto_icq_PPID()
{
  static char ppid[] = "Licq";
  return ppid;
}

bool LProto_icq_Init()
{
  return true;
}

unsigned long LProto_icq_SendFuncs()
{
  return 0;
}

int LProto_icq_Main(CICQDaemon* daemon)
{
  int fd = daemon->getPluginManager().registerProtocolPlugin();
  if (fd == -1)
    return -1;

  bool run = true;
  while (run)
  {
    fd_set readFd;
    FD_ZERO(&readFd);
    FD_SET(fd, &readFd);
    int ret = ::select(fd + 1, &readFd, NULL, NULL, NULL);
    if (ret > 0 && FD_ISSET(fd, &readFd))
    {
      char ch;
      ::read(fd, &ch, sizeof(ch));
      switch (ch)
      {
      case PLUGIN_SHUTDOWN:
        run = false;
        break;
      default:
        assert(false);
        break;
      }
    }
  }

  daemon->getPluginManager().unregisterProtocolPlugin();
  return 0;
}