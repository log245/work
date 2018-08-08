/******************************************************
 *			Copyright 2013, by feifan.
 *				All right reserved.
 *功能：dll加载控制逻辑，管理该进程下所有dll的加载和卸载
 *日期：2013-3-13 19:34
 *作者：lxy
 ******************************************************/
#ifndef _PROGRM_DLL_MGR_H_
#define _PROGRM_DLL_MGR_H_
#include <public.h>
#include <framework/daserver.h>
#include <file/dynamicfile.h>
#include <business/consummer/iconsummer.h>
#include <common/util/tinytimer.h>
#include <business/bussinesscommon.h>
#include <event/timereactor.h>

//dll的基本信息类
class CDllInfo : public CRefShare
{
protected:
	friend class CRefObject<CDllInfo>;
public:
	CDynamicFile* m_pFile;//dll文件指针
	IConsummerPtr m_pConsummer;//dll提供的接口
	std::map<uint32, IServantPtr> m_mapServant;//dll注册的servant
	CDllInfo()
	{
		m_pFile = NULL;
		m_pConsummer = NULL;
		m_mapServant.clear();
	}
	~CDllInfo()
	{
		m_pConsummer = NULL;
		m_mapServant.clear();
		if (m_pFile != NULL)
		{
			delete m_pFile;
		}
		m_pFile = NULL;
	}
};
typedef CRefObject<CDllInfo> CDllInfoPtr;

//dll管理器
class CProgramDllMgr : public TimeEventHandle
{
protected:
	friend class CRefObject<CProgramDllMgr> ;
public:
	CProgramDllMgr();
	virtual ~CProgramDllMgr();

	/**
	 * @功能：进程启动，加载所有dll
	 * @输入：const char*进程启动参数，配置读取该进程所配置dll的节点
	 * @输出：无
	 * @返回：加载所有dll是否成功
	 */
	bool loadDll(const char* strSettings);

	/**
	 * @功能：加载指定dll
	 * @输入：std::string为dll的名字，uint32为dll的编号
	 * @输出：无
	 * @返回：加载dll是否成功
	 */
	bool loadDll(std::string strName, uint32 nModule);

	/**
	 * @功能：卸载指定的dll，并且保存原dll指针
	 * @输入：uint32为dll的编号
	 * @输出：无
	 * @返回：无
	 */
	void unLoadDll(uint32 nMoudle);

	/**
	 * @功能：获取所有dll的接口指针
	 * @输入：无
	 * @输出：std::map<uint32, IConsummerPtr>&，保存所有dll接口指针
	 * @返回：无
	 */
	void getConsummer(std::map<uint32, IConsummerPtr>& mapConsummer);

	void initGlobalConsummer();

	/**
	 * @功能：注册servant到daserver里面
	 * @输入：std::map<uint32, IServantPtr>&从加载成功的dll中获取的servant指针
	 * @输出：无
	 * @返回：无
	 */
	void registServant(std::map<uint32, IServantPtr>& mapServant);

	/**
	 * @功能：在daserver取消servant注册
	 * @输入：std::map<uint32, IServantPtr>&从卸载成功dll中获取的servant指针
	 * @输出：无
	 * @返回：无
	 */
	void unregistServant(std::map<uint32, IServantPtr>& mapServant);

	/**
	 * @功能：配置文件更新(由daserver调用)，会根据配置文件的情况重新加载或卸载dll
	 * @输入：CDAServer,底层控制器
	 * @输出：无
	 * @返回：无
	 */
	virtual void onProfileUpdate( CDAServer& Server );

	/**
	 * @功能：服务器心跳（100毫秒）
	 * @输入：无
	 * @输出：无
	 * @返回：无
	 */
	void execute(void);

	/**
	 * @功能：网络连接断开
	 * @输入：CSessionPtr网络连接session
	 * @输出：无
	 * @返回：无
	 */
	void onSessionClose(CSessionPtr pSession);

	/**
	 * @功能：进程关闭，会调用卸载所有dll
	 * @输入：无
	 * @输出：无
	 * @返回：无
	 */
	void onServerStop();

	/**
	 * @功能：关闭服务器
	 */
	void stopServer();

private:
	//当前加载的dll
	std::map<uint32, CDllInfoPtr> m_mapDll;
	std::map<uint32, uint32> m_mapDllUpdateNum;//dll更新标记

	std::string m_strSession;//配置文件节点
	std::string m_strProfile;//配置文件路径
	CProfile m_Profile;//dll配置
	time_t m_nProfileModifiedTime;	//配置文件上次修改时间
	CTinyTimer m_tCheckProfile;//检查配置文件周期	
};	
typedef CRefObject<CProgramDllMgr> CProgramDllMgrPtr;

#endif//_PROGRM_DLL_MGR_H_