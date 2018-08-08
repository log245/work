#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <common/util/refshare.h>
#include <framework/daserver.h>
#include <framework/daexception.h>
#include <framework/servant.h>
#include <net/netdefines.h>
#include <business/bussinesscommon.h>
#include "programdllmgr.h"
#include "sessionhandler.h"
#include "serverhandler.h"
#include <business/util/systemlogstream.h>
#include <common/util/userexception.h>
#include <common/db/connection.h>
#ifndef WIN32
#include <malloc.h>
#endif
CDAServerPtr g_DaServer;
CProgramDllMgrPtr g_pDllMgr = NULL;
IConsummerPtr g_pConsummer = NULL;
CSystemLogOutputStreamPtr g_pSystemLogOut = NULL;

static void sig_int(int sigi)
{
	g_DaServer->stop();
	log_info("sig_int|stop server now....");
}
static void sig_int_nothing(int sigi)
{
	log_info("sig_int|abort....");
	if (!g_DaServer->isInited())
	{
		exit(-1);
	}
}

int main( int argc, char** argv )
{
#ifdef WIN32
	if( (signal(SIGINT,sig_int) == SIG_ERR) || (signal(SIGTERM,sig_int) == SIG_ERR) )
		exit(-1);
#else
	if( (signal(SIGINT,sig_int) == SIG_ERR) || (signal(SIGTERM,sig_int) == SIG_ERR) || (signal(SIGABRT,sig_int_nothing) == SIG_ERR))
		exit(-1);

	if(signal(SIGTERM, sig_int) == SIG_ERR)
		exit(-1);
#endif

	try
	{
#ifndef WIN32
		//频繁的内存申请/释放导致大量缺页中断
		mallopt(M_MMAP_MAX, 0); // 禁止malloc调用mmap分配内存
		mallopt(M_TRIM_THRESHOLD, -1); // 禁止内存紧缩
#endif
		CThreadFactory::initialize();
		CCException::initStackTrace();
		parseCmdArgs(argc,argv);
		if ( !initFrameWorkLib(argv[0]) )
			ThrowException<CCException>("init FrameWorkLib Error!");

		g_DaServer = CDAServerPtr::createInstance();
		if( !g_DaServer->init(getFrameWorkProf()) )
		{
			log_error("Failed to call DAServer::init() \n");
			return -1;
		}
		g_DaServer->setArg(argc, argv);

		g_pSystemLog = new CSystemLog();
		const char* logdir = NULL;
		if ( !getCmdArgs('l',logdir) )
			logdir = "./log";
		//写本地文件
		g_pSystemLogOut = new CSystemLogOutputStream(logdir);
		if ( !g_pSystemLogOut->init(g_DaServer) )
			ThrowException<CCException>("init CLocalLogOutputStream Error!");
		g_pSystemLog->addFileLogOutput(g_pSystemLogOut.get());
		g_pSystemLog->setLogPath(logdir);
		g_pSystemLogOut->start();

		g_pDllMgr = new CProgramDllMgr();
	
		const char* szdll = NULL;
		if ( !getCmdArgs('d',szdll) )
			ThrowException<CCException>("init dll Error!");
		g_pDllMgr->loadDll(szdll);

		g_DaServer->registeTimeTask(g_pDllMgr.get(),0,0,100 * 1000);

		g_DaServer->addSessionHandler(new CSessionHandler());

		g_DaServer->setServerHandler(new CServerHandler());
		
		if (!g_DaServer->initService())
		{
			log_error("Failed to call DAServer::initService() \n");
			return -1;
		}

		//记录加载完成标志
		#ifndef WINDOWS
			uint16 npid = getpid();
			char buffer[255] ={ 0 };
			sprintf(buffer, "%s/%d.loadfinish", logdir, npid);
			FILE *logfile = NULL;
			if ((logfile = fopen(buffer, "a")) != NULL)
			{
				fclose(logfile);
			}
		#endif

		g_DaServer->run();
	}
	catch(CDAException& e)
	{
		log_warning("Catch CDAException %s", e.printStackTrace());
		if ( g_DaServer != NULL )
			g_DaServer->stop();
		if (g_pDllMgr != NULL)
		{
			g_pDllMgr->stopServer();
			g_pDllMgr->onServerStop();
		}
		if (g_pSystemLogOut != NULL)
			g_pSystemLogOut->stop();
		uninitFrameWorkLib();
		CCException::cleanupStackTrace();
		return -1;
	}    
	catch (CCException& e)
	{
		log_warning("catch ccexception[%s][%s]",e.what(), e.printStackTrace());
		if ( g_DaServer != NULL )
			g_DaServer->stop();
		if (g_pDllMgr != NULL)
		{
			g_pDllMgr->stopServer();
			g_pDllMgr->onServerStop();
		}
		if (g_pSystemLogOut != NULL)
			g_pSystemLogOut->stop();
		uninitFrameWorkLib();
		CCException::cleanupStackTrace();
		return -1;
	}
	catch (exception &e)
	{
		log_warning("catch exception[%s]",e.what());
		if ( g_DaServer != NULL )
			g_DaServer->stop();
		if (g_pDllMgr != NULL)
		{
			g_pDllMgr->stopServer();
			g_pDllMgr->onServerStop();
		}
		if (g_pSystemLogOut != NULL)
			g_pSystemLogOut->stop();
		uninitFrameWorkLib();
		CCException::cleanupStackTrace();
		return -1;
	}
	catch(...)
	{
		log_warning("catch ...");

		if(g_DaServer != NULL)
			g_DaServer->stop();
		if (g_pDllMgr != NULL)
		{
			g_pDllMgr->stopServer();
			g_pDllMgr->onServerStop();
		}
		if (g_pSystemLogOut != NULL)
			g_pSystemLogOut->stop();
		uninitFrameWorkLib();
		CCException::cleanupStackTrace();
		return -1;
	}

	if (g_pDllMgr != NULL)
	{
		g_pDllMgr->stopServer();
	}
	log_info("Program End!");
	if (g_pSystemLogOut != NULL)
		g_pSystemLogOut->stop();
	uninitFrameWorkLib();
	CCException::cleanupStackTrace();
	return 0;
}
