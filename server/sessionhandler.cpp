#include "sessionhandler.h"
#include <net/netdefines.h>
#include <framework/daserver.h>
#include "programdllmgr.h"
extern CProgramDllMgrPtr g_pDllMgr;

extern CDAServerPtr g_DaServer;

CSessionHandler::CSessionHandler()
{

}

CSessionHandler::~CSessionHandler()
{

}

void CSessionHandler::onSessionClose(CSessionPtr pSession)
{
	g_pDllMgr->onSessionClose(pSession);
}

void CSessionHandler::incRef()
{
    CRefShare::incRef();
}

bool CSessionHandler::decRef()
{
    return CRefShare::decRef();
}
