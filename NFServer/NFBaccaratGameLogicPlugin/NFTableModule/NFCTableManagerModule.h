#pragma once
#ifndef NFC_TABLE_MANAGER_MODULE_H
#define NFC_TABLE_MANAGER_MODULE_H
#include "NFComm/NFPluginModule/NFITableManagerModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFPluginModule/NFITableUserModule.h"
#include "StructHPP/GameConfig.hpp"
#include "Dependencies/json/json.hpp"

class SArrangeTableItem;
class CCalcTimeTick;
class SArrangeUserList;

class NFCTableManagerModule
    :public NFITableManagerModule
{
public:
    NFCTableManagerModule(NFIPluginManager* p);

    virtual ~NFCTableManagerModule();

    virtual bool Awake() override;

    virtual bool Init() override;

    virtual bool AfterInit() override;

    virtual bool Execute() override;

    virtual bool Shut() override;

    ////////////////////////////////////////////////////////////////

    virtual void OnUserOffline(int userId) override;

    virtual bool AddSitUser(int userId, int tableHandle, int chairIndex) override;

    virtual bool RemoveSitUser(int userId) override;

    virtual NF_SHARE_PTR<NFGameTable> GetUserTable(int usreId, int& chairIndex) override;

    virtual NF_SHARE_PTR<NFGameTable> GetPUserTable(const SUserInfo& pUser, int& chairIndex) override;

    virtual NF_SHARE_PTR<NFGameTable> GetTableByHandle(int handle) override;

    virtual void AddTable(NF_SHARE_PTR<NFGameTable> newTable) override;

    virtual int GetUserCount() override;

    virtual int GetSelScoreByHandle(int handle) override;

    virtual bool EnterRoomBySelectSocre(NF_SHARE_PTR<SUserInfo> pUser, int ignoreTableHandle, function<void()> successCallBack) override;

    virtual bool userChangeTable(SUserInfo* pUser, NFGameTable* oldTable, function<void()> successCallBack)  override;

    //发送邮件给单个用户
	virtual void SendMailToTableUser(const int nMsgId, const nlohmann::json& jsonSend, const NFGUID& fd) override;
    //广播邮件给一个桌子上的所有用户
    virtual void SendMailToUserAtTheSameTable(const int nMsgId, const nlohmann::json& jsonSend, const int ignoreUserId, int isIgnoreState, map<int, NF_SHARE_PTR<NFITableUserModule>>& tableUserMap) override;
    //广播邮件给所有桌子上的所有用户
    virtual void SendMailToAllTableUser(const int nMsgId, const nlohmann::json& jsonSend, int ignoreUserId, vector<NF_SHARE_PTR<NFGameTable>>& tableVector) override;

public:

	void OnNodeDbGidReadUserInfoCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

    void OnMsGidScoreReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

    void OnMsGidGameControlCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

    void OnMsGidMatchReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void OnMsGidLockGameRoomCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

protected:
private:
    bool FindSitUser(int userId, int& retTableHandle, int&retChairIndex);

    SArrangeTableItem* FindEmptyTable(int selScore, int minEmptyChair, int maxEmptyChair, int ignoreHandle);

    bool UpdateCanArrangeTableList();

    void AddUserToArrange(SArrangeUserList& pUserList, SArrangeTableItem& pItem, int delUserIndex, NF_SHARE_PTR<SUserInfo> pUser);

    bool AddUserToArrange(SArrangeTableItem& pItem, NF_SHARE_PTR<SUserInfo> pUser);
private:
    CCalcTimeTick*                  m_arrangeTableTime;
    SArrangeTableItem*              m_arrangeTableAry[MAX_TABLE_HANDLE_COUNT];
    int                             m_emptyChairAry[MAX_TABLE_USER_COUNT];
    int                             m_arrangeTableLen;
    
private:
    vector<NF_SHARE_PTR<NFGameTable>>            m_pTableList;
    map<int, uint32_t>                           m_pSitAllUserList;
    map<int, NF_SHARE_PTR<SArrangeUserList>>     m_arrangeUserMap;

    NFILogModule *                  m_pLogModule;
	NFIGameServerNet_ServerModule * m_pGameServerNet_ServerModule;
    NFIGameLogicModule *            m_pGameLogicModule;
    NFIConfigAreaModule *           m_pConfigAreaModule;
    NFIRobotModule *                m_pRobotModule;
	NFINetClientModule *			m_pNetClientModule;
	NFIQsTaskScheduleModule *		m_pQsTaskScheduleModule;
};

#endif