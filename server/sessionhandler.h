/******************************************************
 *			Copyright 2013, by feifan.
 *				All right reserved.
 *功能：监听网络连接断开
 *日期：2013-3-13 19:53
 *作者：lxy
 ******************************************************/
#ifndef _CSESSIONHANDLER_H_
#define _CSESSIONHANDLER_H_

#include <framework/sessionhandler.h>

class CSessionHandler : public ISessionHandler, public CRefShare
{
public:
    CSessionHandler();
    virtual ~CSessionHandler();

    virtual void onSessionClose(CSessionPtr pSession);

    void incRef();
    bool decRef();
};

typedef CRefObject<CSessionHandler> CSessionHandlerPtr;

#endif
