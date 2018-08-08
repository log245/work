#include "serverhandler.h"
#include <business/bussinesscommon.h>
#include "programdllmgr.h"
extern CProgramDllMgrPtr g_pDllMgr;

CServerHandler::CServerHandler()
{

}

CServerHandler::~CServerHandler()
{

}

void CServerHandler::onServerStop()
{
	g_pDllMgr->onServerStop();
}

void CServerHandler::incRef()
{
    CRefShare::incRef();
}

bool CServerHandler::decRef()
{
    return CRefShare::decRef();
}

void CServerHandler::onProfileUpdate( CDAServer& Server )
{
	g_pDllMgr->onProfileUpdate(Server);
}
