/******************************************************
 *			Copyright 2013, by feifan.
 *				All right reserved.
 *���ܣ�dll���ؿ����߼�������ý���������dll�ļ��غ�ж��
 *���ڣ�2013-3-13 19:34
 *���ߣ�lxy
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

//dll�Ļ�����Ϣ��
class CDllInfo : public CRefShare
{
protected:
	friend class CRefObject<CDllInfo>;
public:
	CDynamicFile* m_pFile;//dll�ļ�ָ��
	IConsummerPtr m_pConsummer;//dll�ṩ�Ľӿ�
	std::map<uint32, IServantPtr> m_mapServant;//dllע���servant
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

//dll������
class CProgramDllMgr : public TimeEventHandle
{
protected:
	friend class CRefObject<CProgramDllMgr> ;
public:
	CProgramDllMgr();
	virtual ~CProgramDllMgr();

	/**
	 * @���ܣ�������������������dll
	 * @���룺const char*�����������������ö�ȡ�ý���������dll�Ľڵ�
	 * @�������
	 * @���أ���������dll�Ƿ�ɹ�
	 */
	bool loadDll(const char* strSettings);

	/**
	 * @���ܣ�����ָ��dll
	 * @���룺std::stringΪdll�����֣�uint32Ϊdll�ı��
	 * @�������
	 * @���أ�����dll�Ƿ�ɹ�
	 */
	bool loadDll(std::string strName, uint32 nModule);

	/**
	 * @���ܣ�ж��ָ����dll�����ұ���ԭdllָ��
	 * @���룺uint32Ϊdll�ı��
	 * @�������
	 * @���أ���
	 */
	void unLoadDll(uint32 nMoudle);

	/**
	 * @���ܣ���ȡ����dll�Ľӿ�ָ��
	 * @���룺��
	 * @�����std::map<uint32, IConsummerPtr>&����������dll�ӿ�ָ��
	 * @���أ���
	 */
	void getConsummer(std::map<uint32, IConsummerPtr>& mapConsummer);

	void initGlobalConsummer();

	/**
	 * @���ܣ�ע��servant��daserver����
	 * @���룺std::map<uint32, IServantPtr>&�Ӽ��سɹ���dll�л�ȡ��servantָ��
	 * @�������
	 * @���أ���
	 */
	void registServant(std::map<uint32, IServantPtr>& mapServant);

	/**
	 * @���ܣ���daserverȡ��servantע��
	 * @���룺std::map<uint32, IServantPtr>&��ж�سɹ�dll�л�ȡ��servantָ��
	 * @�������
	 * @���أ���
	 */
	void unregistServant(std::map<uint32, IServantPtr>& mapServant);

	/**
	 * @���ܣ������ļ�����(��daserver����)������������ļ���������¼��ػ�ж��dll
	 * @���룺CDAServer,�ײ������
	 * @�������
	 * @���أ���
	 */
	virtual void onProfileUpdate( CDAServer& Server );

	/**
	 * @���ܣ�������������100���룩
	 * @���룺��
	 * @�������
	 * @���أ���
	 */
	void execute(void);

	/**
	 * @���ܣ��������ӶϿ�
	 * @���룺CSessionPtr��������session
	 * @�������
	 * @���أ���
	 */
	void onSessionClose(CSessionPtr pSession);

	/**
	 * @���ܣ����̹رգ������ж������dll
	 * @���룺��
	 * @�������
	 * @���أ���
	 */
	void onServerStop();

	/**
	 * @���ܣ��رշ�����
	 */
	void stopServer();

private:
	//��ǰ���ص�dll
	std::map<uint32, CDllInfoPtr> m_mapDll;
	std::map<uint32, uint32> m_mapDllUpdateNum;//dll���±��

	std::string m_strSession;//�����ļ��ڵ�
	std::string m_strProfile;//�����ļ�·��
	CProfile m_Profile;//dll����
	time_t m_nProfileModifiedTime;	//�����ļ��ϴ��޸�ʱ��
	CTinyTimer m_tCheckProfile;//��������ļ�����	
};	
typedef CRefObject<CProgramDllMgr> CProgramDllMgrPtr;

#endif//_PROGRM_DLL_MGR_H_