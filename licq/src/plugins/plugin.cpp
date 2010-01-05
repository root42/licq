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

#include "plugin.h"

#include "licq_constants.h"
#include "licq/thread/mutexlocker.h"

#include <pthread.h>

using Licq::MutexLocker;
using namespace LicqDaemon;

Plugin::Plugin(boost::shared_ptr<DynamicLibrary> lib,
               const std::string& prefix, bool prefixId) :
  myLib(lib),
  mySignalMask(0)
{
  loadSymbol(prefix + "_Main_tep", myMainThreadEntryPoint);
  loadSymbol(prefix + "_Name", myName);
  loadSymbol(prefix + "_Version", myVersion);

  if (prefixId)
    loadSymbol(prefix + "_Id", myId);
  else
    loadSymbol("LP_Id", myId);
  *myId = -1;
}

Plugin::~Plugin()
{
  // Empty
}

void Plugin::startThread(CICQDaemon* daemon)
{
  ::pthread_create(&myThread, NULL, myMainThreadEntryPoint, daemon);
}

int Plugin::joinThread()
{
  void* result;
  if (::pthread_join(myThread, &result) == 0)
  {
    int* retval = reinterpret_cast<int*>(result);
    int value = *retval;
    ::free(retval);
    return value;
  }

  return -1;
}

void Plugin::cancelThread()
{
  ::pthread_cancel(myThread);
}

bool Plugin::isThisThread() const
{
  return isThread(::pthread_self());
}

bool Plugin::isThread(const pthread_t& thread) const
{
  return ::pthread_equal(myThread, thread) != 0;
}

const char* Plugin::getName() const
{
  return (*myName)();
}

const char* Plugin::getVersion() const
{
  return (*myVersion)();
}

unsigned short Plugin::getId() const
{
  return *myId;
}

const std::string& Plugin::getLibraryName() const
{
  return myLib->getName();
}

void Plugin::shutdown()
{
  myPipe.putChar(PLUGIN_SHUTDOWN);
}
