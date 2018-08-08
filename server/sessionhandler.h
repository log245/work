/******************************************************
 *			Copyright 2013, by feifan.
 *				All right reserved.
 *���ܣ������������ӶϿ�
 *���ڣ�2013-3-13 19:53
 *���ߣ�lxy
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
