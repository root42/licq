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

#ifndef LICQDAEMON_PLUGIN_H
#define LICQDAEMON_PLUGIN_H

#include "licq/plugin.h"
#include "licq/thread/mutex.h"
#include "utils/dynamiclibrary.h"
#include "utils/pipe.h"

#include <boost/exception/info.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <string>

class CICQDaemon;

namespace LicqDaemon
{

class Plugin : public virtual Licq::Plugin
{
public:
  typedef boost::shared_ptr<Plugin> Ptr;

  typedef boost::
  error_info<struct tag_errinfo_symbol_name, std::string> errinfo_symbol_name;

  Plugin(boost::shared_ptr<DynamicLibrary> lib, const std::string& prefix,
         bool prefixId = false);
  virtual ~Plugin();

  int getReadPipe() const;

  /**
   * Start the plugin in a new thread.
   */
  void startThread(CICQDaemon* daemon);

  /**
   * Wait for the plugin to stop.
   * @returns The plugins exit code.
   */
  int joinThread();

  void cancelThread();

  /**
   * @return True when called from the plugin's main thread.
   */
  bool isThisThread() const;

  bool isThread(const pthread_t& thread) const;

  void setId(unsigned short id);

  void setSignalMask(unsigned long mask);
  bool wantSignal(unsigned long signal);

  // From Licq::Plugin
  unsigned short getId() const;
  const char* getName() const;
  const char* getVersion() const;
  const std::string& getLibraryName() const;
  void shutdown();

protected:
  boost::shared_ptr<DynamicLibrary> myLib;
  Pipe myPipe;

  template<typename SymbolType>
  void loadSymbol(const std::string& name, SymbolType*& symbol);

private:
  pthread_t myThread;
  unsigned long mySignalMask;

  // Function pointers
  void* (*myMainThreadEntryPoint)(void*);
  const char* (*myName)();
  const char* (*myVersion)();

  // Unique plugin id
  unsigned short* myId;
};

inline int Plugin::getReadPipe() const
{
  return myPipe.getReadFd();
}

inline void Plugin::setId(unsigned short id)
{
  *myId = id;
}

inline void Plugin::setSignalMask(unsigned long mask)
{
  mySignalMask = mask;
}

inline bool Plugin::wantSignal(unsigned long signal)
{
  return (signal & mySignalMask);
}

template<typename SymbolType>
inline void Plugin::loadSymbol(const std::string& name, SymbolType*& symbol)
{
  try
  {
    myLib->getSymbol(name, &symbol);
  }
  catch (DynamicLibrary::Exception& ex)
  {
    ex << errinfo_symbol_name(name);
    throw;
  }
}

} // namespace LicqDaemon

#endif
