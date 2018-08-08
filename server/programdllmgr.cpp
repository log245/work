#include "programdllmgr.h"
#include <net/netdefines.h>
#include <framework/commonfun.h>
#include <common/util/dir.h>
#include <common/file/fileutil.h>

extern CDAServerPtr g_DaServer;
extern CLogPtr g_Log;
extern CSystemLogPtr g_pSystemLog;
extern IConsummerPtr g_pConsummer;

typedef void (*pInitDllServant)(CDAServerPtr, CLogPtr, CSystemLogPtr, IConsummerPtr, std::map<uint32, IServantPtr>& );
typedef void (*pDllGetConsummer)(IConsummerPtr& );

CProgramDllMgr::CProgramDllMgr()
{
	m_mapDll.clear();
	m_strSession.clear();
	m_nProfileModifiedTime = 0;
	m_strProfile.clear();
	m_mapDllUpdateNum.clear();
}

CProgramDllMgr::~CProgramDllMgr()
{
	g_pConsummer = NULL;
	m_mapDll.clear();
}

bool CProgramDllMgr::loadDll(const char* strSettings)
{
	IF (g_DaServer == NULL)
	{
		return false;
	}
	m_strSession = strSettings;

	const char* workpath = NULL;
	if ( !getCmdArgs('w',workpath) )
		workpath = "./";
	std::string strworkpath = workpath;
#ifdef WIN32
	m_strProfile = strworkpath + "\\..\\..\\conf\\processmodule.conf";	
#else
	m_strProfile = strworkpath + "/../program/processmodule.conf";	
#endif
	if( !m_Profile.open( m_strProfile.c_str() ) )
	{
		log_error("cprogramdllmgr|load config file[%s] error", m_strProfile.c_str());
		return false;
	}
	CFileUtil::getLastModifiedTime(m_strProfile.c_str(), m_nProfileModifiedTime);
	m_tCheckProfile.Start(30);

	//dll加载信息
	std::vector<std::string> vecsuccessmodule;
	std::vector<std::string> vecignoremodule;
	std::vector<std::string> vecfailedmodule;
	vecsuccessmodule.clear();
	vecignoremodule.clear();
	vecfailedmodule.clear();

	std::map<std::string, std::string> keypairs;
	keypairs.clear();
	m_Profile.getKeyPairs(strSettings, keypairs);
	std::map<std::string, std::string>::iterator iter = keypairs.begin();
	std::map<uint32, std::string> mapmodule;
	mapmodule.clear();
	DEADLOOP_BREAK_BEGIN(t1);
	for (; iter != keypairs.end(); iter++)
	{
		DEADLOOP_BREAK(t1,10000);
		char szname[100] = {0};
		int nmoduleid = 0;
		if (sscanf(iter->first.c_str(), "%d,%s", &nmoduleid, szname) != 2)
		{
			continue;
		}
		m_mapDllUpdateNum[nmoduleid] = atoi(iter->second.c_str());
		if (atoi(iter->second.c_str()) == 0)
		{
			vecignoremodule.push_back(szname);
			continue;
		}		
		mapmodule[nmoduleid] = szname;
	}

	DEADLOOP_BREAK_BEGIN(t2);
	for (std::map<uint32, std::string>::iterator itermodule=mapmodule.begin(); itermodule!=mapmodule.end(); ++itermodule)
	{
		DEADLOOP_BREAK(t2,10000);
		if (IS_MAIN_DLL(itermodule->first))
		{
			try
			{
				if (!this->loadDll(itermodule->second, itermodule->first))
				{
					ThrowException<CCException>("init main dll[%s] failed", itermodule->second.c_str());
				}
				vecsuccessmodule.push_back(itermodule->second);
			}
			catch (CDAException& e)
			{
				log_warning("CProgramDllMgr::loadDll Catch CDAException[%s][%s]", e.what(), e.printStackTrace());
				ThrowException<CCException>("init main dll[%s] failed", itermodule->second.c_str());
			}
			catch (CCException& e)
			{
				log_warning("CProgramDllMgr::loadDll catch ccexception[%s][%s]", e.what(), e.printStackTrace());
				ThrowException<CCException>("init main dll[%s] failed", itermodule->second.c_str());
			}
			catch (exception &e)
			{
				log_warning("CProgramDllMgr::loadDll catch exception[%s]", e.what());
				ThrowException<CCException>("init main dll[%s] failed", itermodule->second.c_str());
			}
			catch (...)
			{
				log_warning("CProgramDllMgr::loadDll catch ...");
				ThrowException<CCException>("init main dll[%s] failed", itermodule->second.c_str());
			}
		}
	}

	DEADLOOP_BREAK_BEGIN(t3);
	for (std::map<uint32, std::string>::iterator itermodule=mapmodule.begin(); itermodule!=mapmodule.end(); ++itermodule)
	{
		DEADLOOP_BREAK(t3,10000);
		if (!IS_MAIN_DLL(itermodule->first))
		{
			try
			{
				this->loadDll(itermodule->second, itermodule->first);
				vecsuccessmodule.push_back(itermodule->second);
				CThread::sleep(10);
				continue;
			}
			catch(CDAException& e)
			{
				log_warning("CProgramDllMgr::loadDll Catch CDAException %s", e.printStackTrace());
			}    
			catch (CCException& e)
			{
				log_warning("CProgramDllMgr::loadDll catch ccexception[%s]",e.printStackTrace());
			}
			catch (exception &e)
			{
				log_warning("CProgramDllMgr::loadDll catch exception[%s]",e.what());
			}
			catch(...)
			{
				log_warning("CProgramDllMgr::loadDll catch ...");
			}
			vecfailedmodule.push_back(itermodule->second);
		}
	}	

	this->initGlobalConsummer();

	//打印dll模块加载情况
	log_info("=====================load module  (%d)=========================", vecsuccessmodule.size());
	for (uint32 i=0; i<vecsuccessmodule.size(); ++i)
	{
		log_info("%s", vecsuccessmodule[i].c_str());
	}
	log_info("=====================ignore module (%d)========================", vecignoremodule.size());
	for (uint32 i=0; i<vecignoremodule.size(); ++i)
	{
		log_info("%s", vecignoremodule[i].c_str());
	}
	log_info("=====================failed module (%d)========================", vecfailedmodule.size());
	for (uint32 i=0; i<vecfailedmodule.size(); ++i)
	{
		log_info("%s", vecfailedmodule[i].c_str());
	}
	log_info("===========================end================================");

	return true;
}

void CProgramDllMgr::initGlobalConsummer()
{
	std::map<uint32, IConsummerPtr> mapconsummer;
	mapconsummer.clear();
	this->getConsummer(mapconsummer);
	if (g_pConsummer != NULL)
	{
		g_pConsummer->initConsummer(mapconsummer);
	}
}

void CProgramDllMgr::getConsummer(std::map<uint32, IConsummerPtr>& mapConsummer)
{
	std::map<uint32, CDllInfoPtr>::iterator iter = m_mapDll.begin();
	DEADLOOP_BREAK_BEGIN(t1);
	for (; iter != m_mapDll.end(); iter++)
	{
		DEADLOOP_BREAK(t1,10000);
		if (iter->second == NULL)
		{
			continue;
		}
		if (iter->second->m_pConsummer != NULL)
		{
			mapConsummer[iter->first] = iter->second->m_pConsummer;
		}
	}
}

void CProgramDllMgr::onServerStop()
{
	if (g_pConsummer == NULL)
	{
		return;
	}
	if (g_pConsummer != NULL)
	{
		g_pConsummer->onServerStop();
	}
}

void CProgramDllMgr::stopServer() 
{
	std::map<uint32, CDllInfoPtr>::iterator iter = m_mapDll.begin();
	DEADLOOP_BREAK_BEGIN(t1);
	for (; iter != m_mapDll.end(); iter++)
	{
		DEADLOOP_BREAK(t1, 10000);
		if (IS_MAIN_DLL(iter->first))
		{
			continue;
		}
		if (iter->second != NULL)
		{
			this->unregistServant(iter->second->m_mapServant);
		}
		if (iter->second->m_pConsummer != NULL)
		{
			iter->second->m_pConsummer->onServerStop();
		}
		if (g_pConsummer != NULL)
		{
			g_pConsummer->removeConsummer(iter->first);
		}
	}
}

void CProgramDllMgr::registServant(std::map<uint32, IServantPtr>& mapServant)
{
	IF (g_DaServer == NULL)
	{
		return;
	}
	for (std::map<uint32, IServantPtr>::iterator iter=mapServant.begin(); iter!=mapServant.end(); ++iter)
	{
		if (iter->second != NULL)
		{
			//iter->second->incRef();
			g_DaServer->addServant(iter->first, iter->second.get());
		}
	}	
}

void CProgramDllMgr::unregistServant(std::map<uint32, IServantPtr>& mapServant)
{
	IF (g_DaServer == NULL)
	{
		return;
	}
	for (std::map<uint32, IServantPtr>::iterator iter=mapServant.begin(); iter!=mapServant.end(); ++iter)
	{
		if (iter->second != NULL)
		{
			g_DaServer->removeServant(iter->first);
		}
	}
}

void CProgramDllMgr::onProfileUpdate( CDAServer& Server )
{	
	std::map<uint32, CDllInfoPtr>::iterator iterdll = m_mapDll.begin();
	DEADLOOP_BREAK_BEGIN(t2);
	for (; iterdll!=m_mapDll.end(); ++iterdll)
	{
		DEADLOOP_BREAK(t2,10000);
		if (iterdll->second != NULL)
		{
			if (iterdll->second->m_pConsummer != NULL)
			{
				iterdll->second->m_pConsummer->onProfileUpdate(Server);
			}
		}
	}
}

bool CProgramDllMgr::loadDll(std::string strName, uint32 nModule)
{
	const char* workpath = NULL;
	if ( !getCmdArgs('w',workpath) )
		workpath = "./";

	std::string strworkpath = workpath;
	std::string strextname = "";
	int nindex = strworkpath.find_last_of('/');
	if (nindex != -1)
	{
		strextname.assign(strworkpath, nindex + 1, strworkpath.length());
	}
#ifdef WIN32
	std::string strsorcename = strworkpath + "\\" + strName + ".dll";
	std::string strdestname = strsorcename;
#else
	std::string strsorcename = strworkpath + "/../program/lib/lib" + strName;	
	std::string strdestname = strworkpath + "/../program/tmp/";

	CDir dir;
	if (!dir.OpenDirectory(strdestname.c_str()))
	{
		CDir::MakeDirectory(strdestname.c_str());
	}
	else
	{
		dir.Close();
	}
	strdestname = strdestname + "lib" + strName + CDateTime().asString("YYYYMMDDhhmmss") + "_" + strextname + ".so";
	strsorcename = strsorcename + ".so";
	copyFile(strsorcename.c_str(), strdestname.c_str());
#endif

	CDllInfoPtr pdllinfo = new CDllInfo();
	CDynamicFile* pdll = new CDynamicFile();

	CDAServerPtr pserver = g_DaServer;
	CLogPtr plog = g_Log;
	CSystemLogPtr psystemlog = g_pSystemLog;
	IConsummerPtr pconsummer = g_pConsummer;

	if ( ! pdll->open(strdestname.c_str()))
	{
		log_error("open dyanmic file[%s] error [%s]", strdestname.c_str(), pdll->getError());
		ThrowException<CCException>("open dyanmic file[%s] error [%s]", strdestname.c_str(), pdll->getError());
		return false;
	}
	g_DaServer = pserver;
	g_Log = plog;
	g_pSystemLog = psystemlog;
	g_pConsummer = pconsummer;

	g_Log->info("load dyanmic file[%s] success [%X]", strdestname.c_str(), &pdll);
	pInitDllServant pinitdllservant = (pInitDllServant)pdll->getFuncAddr("initDllServant");
	if ( pinitdllservant == NULL )
	{
		ThrowException<CCException>("can't found function registServant in dll[%s]",pdll->getError());
		return false;
	}
	pDllGetConsummer pgetconsummer = (pDllGetConsummer)pdll->getFuncAddr("getConsummer");
	if ( pgetconsummer == NULL )
	{
		ThrowException<CCException>("can't found function getConsummer in dll[%s]",pdll->getError());
		return false;
	}

	pgetconsummer(pdllinfo->m_pConsummer);
	if (IS_MAIN_DLL(nModule))
	{
		if (pdllinfo->m_pConsummer != NULL)
			g_pConsummer = pdllinfo->m_pConsummer.get();
	}

	pinitdllservant(g_DaServer, g_Log, g_pSystemLog, g_pConsummer, pdllinfo->m_mapServant);
	this->registServant(pdllinfo->m_mapServant);
	pdllinfo->m_pFile = pdll;
	m_mapDll[nModule] = pdllinfo;

	return true;
}

void CProgramDllMgr::unLoadDll(uint32 nMoudle)
{
	std::map<uint32, CDllInfoPtr>::iterator iterold = m_mapDll.find(nMoudle);
	if (iterold == m_mapDll.end())
	{
		return;
	}
	if (IS_MAIN_DLL(nMoudle))
	{
		return;
	}
	if (iterold->second != NULL)
	{
		this->unregistServant(iterold->second->m_mapServant);
	}
	if (iterold->second->m_pConsummer != NULL)
	{
		iterold->second->m_pConsummer->onServerStop();
	}
	if (g_pConsummer != NULL)
	{
		g_pConsummer->removeConsummer(nMoudle);
		if (IS_MAIN_DLL(nMoudle))
		{
			g_pConsummer = NULL;
		}
	}

	m_mapDll.erase(nMoudle);
}

void CProgramDllMgr::execute(void)
{
	long interval = 100;
	std::map<uint32, CDllInfoPtr>::iterator iterdll = m_mapDll.begin();
	DEADLOOP_BREAK_BEGIN(t1);
	for (; iterdll!=m_mapDll.end(); ++iterdll)
	{
		DEADLOOP_BREAK(t1,10000);
		if (iterdll->second != NULL)
		{
			if (iterdll->second->m_pConsummer != NULL && IS_MAIN_DLL(iterdll->first))
			{
				iterdll->second->m_pConsummer->onTimer(interval);
			}
		}
	}
	if (m_tCheckProfile.ToNextTime())
	{
		time_t filetm;
		CFileUtil::getLastModifiedTime(m_strProfile.c_str(), filetm);
		if (filetm != m_nProfileModifiedTime)
		{
			m_nProfileModifiedTime = filetm;
			if (!m_Profile.open(m_strProfile.c_str()))
			{
				log_error("cprogramdllmgr execute|load config file[%s] error", m_strProfile.c_str());
			}
			else
			{
				std::map<std::string, std::string> keypairs;
				keypairs.clear();
				m_Profile.getKeyPairs(m_strSession.c_str(), keypairs);
				std::map<std::string, std::string>::iterator iter = keypairs.begin();
				bool badd = false;
				DEADLOOP_BREAK_BEGIN(t1);
				for (; iter != keypairs.end(); iter++)
				{
					DEADLOOP_BREAK(t1,10000);
					char szname[100] = {0};
					int nmoduleid = 0;
					if (sscanf(iter->first.c_str(), "%d,%s", &nmoduleid, szname) != 2)
					{
						continue;
					}
					if (atoi(iter->second.c_str()) == 0)
					{
						std::map<uint32, CDllInfoPtr>::iterator iterold = m_mapDll.find(nmoduleid);
						if (iterold != m_mapDll.end())
						{
							this->unLoadDll(nmoduleid);
							if (IS_MAIN_DLL(nmoduleid))
							{
								ThrowException<CCException>("main dll[%d] unloaded", nmoduleid);
								return;
							}
						}
						continue;
					}
					else
					{
						std::map<uint32, CDllInfoPtr>::iterator iterold = m_mapDll.find(nmoduleid);
						if (iterold != m_mapDll.end())
						{
							if (m_mapDllUpdateNum[nmoduleid] == atoi(iter->second.c_str()))
							{
								continue;
							}
							else
							{
								this->unLoadDll(nmoduleid);
								if (IS_MAIN_DLL(nmoduleid))
								{
									ThrowException<CCException>("main dll[%d] unloaded", nmoduleid);
									return;
								}
							}
						}
					}

					try
					{
						badd = true;
						m_mapDllUpdateNum[nmoduleid] = atoi(iter->second.c_str());
						this->loadDll(szname, nmoduleid);
					}
					catch(CDAException& e)
					{
						log_warning("CProgramDllMgr::loadDll Catch CDAException %s", e.printStackTrace());
					}    
					catch (CCException& e)
					{
						log_warning("CProgramDllMgr::loadDll catch ccexception[%s]",e.printStackTrace());
					}
					catch (exception &e)
					{
						log_warning("CProgramDllMgr::loadDll catch exception[%s]",e.what());
					}
					catch(...)
					{
						log_warning("CProgramDllMgr::loadDll catch ...");
					}
				}

				if (badd)
				{
					this->initGlobalConsummer();
				}
			}
		}
	}
	return ;
}

void CProgramDllMgr::onSessionClose(CSessionPtr pSession)
{
	std::map<uint32, CDllInfoPtr>::iterator iterdll = m_mapDll.begin();
	DEADLOOP_BREAK_BEGIN(t1);
	for (; iterdll!=m_mapDll.end(); ++iterdll)
	{
		DEADLOOP_BREAK(t1,10000);
		if (iterdll->second != NULL)
		{
			if (iterdll->second->m_pConsummer != NULL)
			{
				iterdll->second->m_pConsummer->onSessionClose(pSession);
			}
		}
	}
}