#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <dlfcn.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "icq-defines.h"
#include "licq.h"
#include "log.h"
#include "countrycodes.h"
#include "utility.h"
#include "support.h"

#include "licq.conf.h"

// Plugin variables
pthread_cond_t LP_IdSignal;
pthread_mutex_t LP_IdMutex;
list<unsigned short> LP_Ids;

CLicq::CLicq(int argc, char **argv)
{
  DEBUG_LEVEL = 0;
  licqException = EXIT_SUCCESS;
  licqDaemon = NULL;
  char *szRedirect = NULL;
  char szFilename[MAX_FILENAME_LEN];
  vector <char *> vszPlugins;
  bool licqHelp = false;
  bool bFork = false;

  // parse command line for arguments
  bool bBaseDir = false;
  bool bForceInit = false;
  bool bSavePlugins = false;
  int i = 0;
  while( (i = getopt(argc, argv, "hd:b:p:iso:f")) > 0)
  {
    switch (i)
    {
    case 'h':  // help
      PrintUsage();
      licqHelp = true;
      licqException = true;
      break;
    case 'b':  // base directory
      sprintf(BASE_DIR, "%s/", optarg);
      bBaseDir = true;
      break;
    case 'd':  // DEBUG_LEVEL
      DEBUG_LEVEL = atol(optarg);
      break;
    case 'i':  // force init
      bForceInit = true;
      break;
    case 'p':  // new plugin
      vszPlugins.push_back(strdup(optarg));
      break;
    case 's':  // save plugin settings
      bSavePlugins = true;
      break;
    case 'o':  // redirect stdout and stderr
      szRedirect = strdup(optarg);
      break;
    case 'f':  // fork
      bFork = true;
      break;
    }
  }

  // Fork into the background
  if (bFork && fork()) exit(0);

  // Initialise the log server for standard output and dump all initial errors
  // and warnings to it regardless of DEBUG_LEVEL
  gLog.AddService(new CLogService_StdOut(DEBUG_LEVEL | L_ERROR | L_WARN));

  // Redirect stdout and stderr if asked to
  if (szRedirect != NULL)
  {
    if (Redirect(szRedirect))
      gLog.Info("%sOutput redirected to \"%s\".\n", L_INITxSTR, szRedirect);
    else
    {
      gLog.Warn("%sRedirection to \"%s\" failed:\n%s%s.\n", L_WARNxSTR,
                szRedirect, L_BLANKxSTR, strerror(errno));
    }
    free (szRedirect);
    szRedirect = NULL;
  }

  // if no base directory set on the command line then get it from HOME
  if (!bBaseDir)
  {
     char *home;
     if ((home = getenv("HOME")) == NULL)
     {
       gLog.Error("%sLicq: $HOME not set, unable to determine config base directory.\n", L_ERRORxSTR);
       licqException = true;
       return;
     }
     sprintf(BASE_DIR, "%s/%s/", home, DEFAULT_HOME_DIR);
  }

  // check if user has conf files installed, install them if not
  if ( (access(BASE_DIR, F_OK) < 0 || bForceInit) && !Install() )
  {
    licqException = EXIT_INSTALLxFAIL;
    return;
  }

  // Define the directory for all the shared data
  sprintf(SHARE_DIR, "%s/%s", INSTALL_DIR, BASE_SHARE_DIR);
  sprintf(LIB_DIR, "%s/%s", INSTALL_DIR, BASE_LIB_DIR);

  // Save the plugins if necessary
  if (bSavePlugins)
  {
    CIniFile licqConf(INI_FxWARN | INI_FxALLOWxCREATE);
    char szConf[MAX_FILENAME_LEN], szKey[32];
    sprintf(szConf, "%slicq.conf", BASE_DIR);
    licqConf.LoadFile(szConf);
    licqConf.SetSection("plugins");
    licqConf.WriteNum("NumPlugins", (unsigned short)vszPlugins.size());
    vector <char *>::iterator iter;
    i = 1;
    for (iter = vszPlugins.begin(); iter != vszPlugins.end(); iter++)
    {
      sprintf(szKey, "Plugin%d", i++);
      licqConf.WriteStr(szKey, *iter);
    }
    licqConf.FlushFile();
    licqException = 255;
    return;
  }

  // Load up the plugins
  m_nNextId = 1;
  vector <char *>::iterator iter;
  for (iter = vszPlugins.begin(); iter != vszPlugins.end(); iter++)
  {
    LoadPlugin(*iter, argc, argv);
    if (licqHelp)
    {
      (*(m_vPluginFunctions.back()).Usage)();
      m_vPluginFunctions.pop_back();
    }
  }

  // Find and load the plugins from the conf file
  if (!licqHelp)
  {
    CIniFile licqConf(INI_FxWARN);
    unsigned short nNumPlugins = 0;
    char szConf[MAX_FILENAME_LEN], szKey[32], szData[MAX_FILENAME_LEN];
    sprintf(szConf, "%slicq.conf", BASE_DIR);
    licqConf.LoadFile(szConf);
    if (licqConf.SetSection("plugins") && licqConf.ReadNum("NumPlugins", nNumPlugins))
    {
      for (int i = 0; i < nNumPlugins; i++)
      {
        sprintf(szKey, "Plugin%d", i + 1);
        if (!licqConf.ReadStr(szKey, szData)) continue;
        LoadPlugin(szData, argc, argv);
      }
    }
  }

  // Start things going
  InitCountryCodes();
  if (!gUserManager.Load())
  {
    licqException = EXIT_LOADxUSERSxFAIL;
    return;
  }
  sprintf(szFilename, "%s%s", SHARE_DIR, UTILITY_DIR);
  gUtilityManager.LoadUtilities(szFilename);

  // Create the daemon
  licqDaemon = new CICQDaemon(this);

}

CLicq::~CLicq(void)
{
  // Close the plugins
  //...
  // Kill the daemon
  if (licqDaemon != NULL) delete licqDaemon;
}


const char *CLicq::Version(void)
{
  static const char version[] = VERSION;
  return version;
}


/*-----------------------------------------------------------------------------
 * LoadPlugin
 *
 * Loads the given plugin using the given command line arguments.
 *---------------------------------------------------------------------------*/
bool CLicq::LoadPlugin(const char *_szName, int argc, char **argv)
{
  void *handle;
  char *error;
  struct SPluginFunctions p;
  char szPlugin[MAX_FILENAME_LEN];

  // First check if the plugin is in the shared location
  if ( _szName[0] != '/' && _szName[0] != '.')
  {
    sprintf(szPlugin, "%slicq_%s.so", LIB_DIR, _szName);
    /*if (access(szPlugin, F_OK) < 0)
      strcpy(szPlugin, _szName);*/
  }
  else
    strcpy(szPlugin, _szName);
  handle = dlopen (szPlugin, DLOPEN_POLICY);
  if (handle == NULL)
  {
    gLog.Error("%sUnable to load plugin (%s): %s.\n ", L_ERRORxSTR, szPlugin, dlerror());
    return false;
  }

  p.Name = (const char * (*)(void))dlsym(handle, "LP_Name");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Name() function in plugin (%s): %s.\n",
               L_ERRORxSTR, _szName, error);
    return false;
  }
  p.Version = (const char * (*)(void))dlsym(handle, "LP_Version");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Version() function in plugin (%s): %s.\n",
               L_ERRORxSTR, (*p.Name)(), error);
    return false;
  }
   p.Init = (bool (*)(int, char **))dlsym(handle, "LP_Init");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Init() function in plugin (%s): %s.\n",
               L_ERRORxSTR, (*p.Name)(), error);
    return false;
  }
  p.Usage = (void (*)(void))dlsym(handle, "LP_Usage");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Usage() function in plugin (%s): %s.\n",
               L_ERRORxSTR, (*p.Name)(), error);
    return false;
  }
  p.Main = (int (*)(CICQDaemon *))dlsym(handle, "LP_Main");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Main() function in plugin (%s): %s.\n",
               L_ERRORxSTR, (*p.Name)(), error);
    return false;
  }
  p.Main_tep = (void * (*)(void *))dlsym(handle, "LP_Main_tep");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Main_tep() function in plugin (%s): %s.\n",
               L_ERRORxSTR, (*p.Name)(), error);
    return false;
  }
  p.Id = (unsigned short *)dlsym(handle, "LP_Id");
  if ((error = dlerror()) != NULL)
  {
    gLog.Error("%sFailed to find LP_Id variable in plugin (%s): %s.\n",
               L_ERRORxSTR, (*p.Name)(), error);
    return false;
  }

  if (!(*p.Init)(argc, argv))
  {
    gLog.Error("%sFailed to initialize plugin (%s).\n", L_ERRORxSTR, (*p.Name)());
    return false;
  }

  *p.Id = m_nNextId++;
  p.dl_handle = handle;
  m_vPluginFunctions.push_back(p);
  return true;
}


int CLicq::Main(void)
{
  int nResult = 0;

  if (m_vPluginFunctions.size() == 0)
  {
    gLog.Warn("%sNo plugins specified on the command-line (-p option).\n%sSee the README for more information.\n",
              L_WARNxSTR, L_BLANKxSTR);
    return nResult;
  }

  nResult = licqDaemon->Start();
  if (nResult != EXIT_SUCCESS) return nResult;

  // Run the plugins
  pthread_cond_init(&LP_IdSignal, NULL);
  vector<struct SPluginFunctions>::iterator iter;
  for (iter = m_vPluginFunctions.begin(); iter != m_vPluginFunctions.end(); iter++)
  {
    gLog.Info("%sStarting plugin %s (version %s).\n", L_INITxSTR, (*(*iter).Name)(),
              (*(*iter).Version)());
    pthread_create( &(*iter).thread_plugin, NULL, (*iter).Main_tep, licqDaemon);
  }
/*  if (m_vPluginFunctions.size() == 0)
  {
    gLog.Warn("%sNo plugins specified on the command-line (-p option).\n%sSee the README for more information.\n",
              L_WARNxSTR, L_BLANKxSTR);
  }*/

  gLog.ModifyService(S_STDOUT, DEBUG_LEVEL);

  unsigned short nId;
  int *nPluginResult;
  bool bDaemonShutdown = false;

  while (m_vPluginFunctions.size() > 0)
  {
    pthread_mutex_lock(&LP_IdMutex);
    while (LP_Ids.size() == 0)
    {
      if (bDaemonShutdown)
      {
        struct timespec abstime;
        abstime.tv_sec = time(TIME_NOW) + MAX_WAIT_PLUGIN;
        abstime.tv_nsec = 0;
        if (pthread_cond_timedwait(&LP_IdSignal, &LP_IdMutex, &abstime) == ETIMEDOUT)
          break;
      }
      else
        pthread_cond_wait(&LP_IdSignal, &LP_IdMutex);
    }
    nId = LP_Ids.front();
    LP_Ids.pop_front();
    pthread_mutex_unlock(&LP_IdMutex);
    if (nId == 0)
    {
      bDaemonShutdown = true;
      continue;
    }

    for (iter = m_vPluginFunctions.begin(); iter != m_vPluginFunctions.end(); iter++)
      if (*(*iter).Id == nId) break;

    if (iter == m_vPluginFunctions.end())
    {
      gLog.Error("%sInvalid plugin id (%d) in exit signal.\n", L_ERRORxSTR, nId);
      continue;
    }

    pthread_join((*iter).thread_plugin, (void **)&nPluginResult);
    gLog.Info("%sPlugin %s exited with code %d.\n", L_ENDxSTR, (*(*iter).Name)(), *nPluginResult);
    free (nPluginResult);
    dlclose((*iter).dl_handle);
    m_vPluginFunctions.erase(iter);
  }

  for (iter = m_vPluginFunctions.begin(); iter != m_vPluginFunctions.end(); iter++)
  {
    gLog.Info("%sPlugin %s failed to exit.\n", L_WARNxSTR, (*(*iter).Name)());
    pthread_cancel( (*iter).thread_plugin);
  }

  pthread_t *t = licqDaemon->Shutdown();
  pthread_join(*t, NULL);

  return m_vPluginFunctions.size();
}


void CLicq::PrintUsage(void)
{
  cerr << PACKAGE << " version " << VERSION << "." << endl
       << "Usage:  Licq [-h] [-d #] [-b configdir] [-i] [-s] [-p plugin] [-p plugin...] [-o file] [ -- <plugin #1 parameters>] [-- <plugin #2 parameters>...]" << endl << endl
       << " -h : this help screen (and any plugin help screens as well)" << endl
       << " -d : set what information is logged to standard output:" << endl
       << "        1  status information" << endl
       << "        2  unknown packets" << endl
       << "        4  errors" << endl
       << "        8  warnings" << endl
       << "       16  all packets" << endl
       << "      add values together for multiple options" << endl
       << " -b : set the base directory for the config and data files (~/.licq by default)" << endl
       << " -i : force initialization of the given base directory" << endl
       << " -p : load the given plugin library" << endl
       << " -s : automatically load the current plugins at next startup" << endl
       << " -o : redirect stdout and stderr to <file>, which can be a device (ie /dev/ttyp4)" << endl
       << endl;
}


bool CLicq::Install(void)
{
  char cmd[MAX_FILENAME_LEN + 128];

  // Create the directory if necessary
  if (mkdir(BASE_DIR, 0700) == -1 && errno != EEXIST)
  {
    cout << "Couldn't mkdir " << BASE_DIR << " - " << strerror(errno) << endl;
    return (false);
  }
  sprintf(cmd, "%s%s", BASE_DIR, HISTORY_DIR);
  if (mkdir(cmd, 0700) == -1 && errno != EEXIST)
  {
    cout << "Couldn't mkdir " << cmd << " - " << strerror(errno) << endl;
    return (false);
  }
  sprintf(cmd, "%s%s", BASE_DIR, USER_DIR);
  if (mkdir(cmd, 0700) == -1 && errno != EEXIST)
  {
    cout << "Couldn't mkdir " << cmd << " - " << strerror(errno) << endl;
    return (false);
  }

  // Create licq.conf
  sprintf(cmd, "%slicq.conf", BASE_DIR);
  FILE *f = fopen(cmd, "w");
  fprintf(f, "%s", LICQ_CONF);
  fclose(f);

  // Create users.conf
  sprintf(cmd, "%susers.conf", BASE_DIR);
  CIniFile usersConf(INI_FxALLOWxCREATE);
  usersConf.LoadFile(cmd);
  usersConf.SetSection("users");
  usersConf.WriteNum("NumOfUsers", 0ul);
  usersConf.FlushFile();

  sprintf (cmd, "%sowner.uin", BASE_DIR);
  CIniFile licqConf(INI_FxALLOWxCREATE);
  licqConf.LoadFile(cmd);
  licqConf.SetSection("user");
  licqConf.WriteStr("Alias", "None");
  licqConf.WriteStr("Password", "");
  licqConf.WriteNum("Uin", 0ul);
  licqConf.WriteBool("WebPresence", false);
  licqConf.WriteBool("HideIP", false);
  licqConf.FlushFile();

  return(true);
}
