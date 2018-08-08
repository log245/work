/******************************************************
 *			Copyright 2013, by feifan.
 *				All right reserved.
 *功能：监听进程关闭消息
 *日期：2013-3-13 19:52
 *作者：lxy
 ******************************************************/
#ifndef _SERVERHANDLER_H_
#define _SERVERHANDLER_H_

#include <util/refshare.h>
#include <framework/response.h>
#include <framework/current.h>
#include <framework/endpoint.h>
#include <framework/responsehandler.h>
#include <framework/daserver.h>

class CServerHandler : public IDAServerHandler, public CRefShare
{
public:
    CServerHandler();
    virtual ~CServerHandler();

public :
    /**
     * 服务器停止
     */
   void onServerStop();
   /**
    *
    */
   void onProfileUpdate(CDAServer& Server);

    void incRef();
    bool decRef();
};

typedef CRefObject<CServerHandler> CServerHandlerPtr;

#endif
