#ifndef NFC_GAME_LOGIC_MODULE_H
#define NFC_GAME_LOGIC_MODULE_H

#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFITableManagerModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ClientModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFCore/NFUtil.hpp"
#include "../NFTableModule/StructHPP/NFTableUser.hpp"
class NFCGameLogicModule
    :public NFIGameLogicModule
{
public:
    NFCGameLogicModule(NFIPluginManager* p);

    virtual ~NFCGameLogicModule();

    virtual bool Awake() override;

    virtual bool Init() override;

    virtual bool AfterInit() override;

    virtual bool Execute() override;

    ////////////////////////////////////////////////

    virtual void GameMsgSenderUnlock(int tableHandle, int userId, int roomId) override;

    virtual void GameMsgSenderLock(int tableHandle, int userId, int roomId) override;

    virtual void GameMsgSenderLockOrUnLock(uint32_t taskId, int32_t gameRoomId, int32_t userId, int32_t gameServerId, int32_t type) override;

    virtual void GameMsgSenderCheckerOpenDoor(int gameRoomId, SUserBaseInfo& baseInfo, int& retCode, std::string & retError) override;

    virtual int ClientLoginResponse(const NFGUID& clientID, const int32_t retCode,  std::string retErrorMsg) override;

    virtual void ClientUpdateUserInfoResponse(const NFGUID& clientFd, const int32_t retCode, const char* retErrorMsg) override;

    virtual void ProcUserInfoToDb(uint32_t taskId, int32_t userId, string accessToken, int32_t gameRoomId, int32_t gameLock, int srcFd) override;

public:
	inline bool OnCloseed(const NFGUID & guid)
	{
		if (FindUserByGuid(guid))
		{
			NF_SHARE_PTR<SUserInfo> userInfo = GetUserInfoByGuid(guid);
			m_MapUserIdForUserInfo.RemoveElement(userInfo->baseInfo.userId); 
			auto ret = m_MapGuidForUserInfo.RemoveElement(guid);
			return ret;
		}
		return false;
	}
 	inline bool FindUserByGuid(const NFGUID & guid)
 	{
 		return m_MapGuidForUserInfo.ExistElement(guid);
 	}
  	inline void AddUserToGuid(NFGUID guid, NF_SHARE_PTR<SUserInfo> & userInfo)
  	{
  		if (!m_MapGuidForUserInfo.ExistElement(guid))
  		{
  			m_MapGuidForUserInfo.AddElement(guid, userInfo);
  		}
  	}
 
  	inline NF_SHARE_PTR<SUserInfo> GetUserInfoByGuid(const NFGUID & guid)
  	{
  		if (m_MapGuidForUserInfo.ExistElement(guid))
  			return m_MapGuidForUserInfo.GetElement(guid);
  
  		return nullptr;
  	}

	inline bool FindUserByUserId(int userId)
	{
		return m_MapUserIdForUserInfo.ExistElement(userId);
	}


	inline void RemoveUserByGuid(const NFGUID& guid)
	{
		m_MapGuidForUserInfo.RemoveElement(guid);
	}

	inline void AddUserToUserId(int userId, NF_SHARE_PTR<SUserInfo> & userInfo)
	{
		if (!m_MapUserIdForUserInfo.ExistElement(userId))
		{
			m_MapUserIdForUserInfo.AddElement(userId, userInfo);
		}
	}

	virtual inline NF_SHARE_PTR<SUserInfo> GetUserInfoByUserId(int userId)
	{
		if (m_MapUserIdForUserInfo.ExistElement(userId))
			return m_MapUserIdForUserInfo.GetElement(userId);

		return nullptr;
	}

	inline void RemoveUserByUserId(const int userId)
	{
		m_MapUserIdForUserInfo.RemoveElement(userId);
	}

    inline void AddUserToLeaveTableList(int userId) override
    {
        m_leaveTableList.remove(userId);
        m_leaveTableList.push_back(userId);
    }

    inline void DelUserFromLeaveTableList(const int userId) override
    {
        m_leaveTableList.remove(userId);
    }

	inline void SendMsgToClient(const uint16_t nMsgId,const nlohmann::json & jsonObject, const NFGUID& clientGuid) override
	{
		if (!FindUserByGuid(clientGuid))
		{
			std::ostringstream strLog;
			strLog << "没有找到guid:" << clientGuid.ToString();
			m_pLogModule->LogError(strLog, __FUNCTION__, __LINE__);
			return;
		}
		std::string msgStr = jsonObject.dump();
		m_pGameServerNet_ServerModule->SendMsgToGate(nMsgId, msgStr, clientGuid);
	}

public:
	void OnClientLeaveGameProcess(const NFSOCK nSockIndex, const int nMsgID, const char *msg, const uint32_t nLen);

    int OnMsGidClientLogin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    int OnMsGidClientUpdateUserInfo(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    int OnMsGidClientOnTick(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    int OnMsGidClientEnterRoom(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    int OnMsGidClientLeaveRoom(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    int OnMsGidClientChat(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    int OnMsGidClientBet(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

    ///////////////////////////////////////////////////
    void ProcLeaveTableList();

	void OnTaskTimeOutProcess(const int taskId, const int msgId);

private:
    //TODO NFIWorldGameAreaModule * m_pWorldGameAreaModule;
    NFITableManagerModule * m_pTableManagerModule;
    NFIConfigAreaModule * m_pConfigAreaModule;
    NFIRobotModule * m_pRobotModule;
	NFINetClientModule * m_pNetClientModule;
	NFINetModule * m_pNetModule;
    NFILogModule * m_pLogModule;
	NFIGameServerNet_ServerModule * m_pGameServerNet_ServerModule;
	NFIGameServerNet_ClientModule * m_pGameServerNet_ClientModule;
	NFIGameServerToDBModule * m_pNFIGameServerToDBModule;
	NFIQsTaskScheduleModule * m_pQsTaskScheduleModule;

	NFMapEx<NFGUID, SUserInfo> m_MapGuidForUserInfo;
	NFMapEx<int, SUserInfo> m_MapUserIdForUserInfo;

    int const LOCK_USER_LOGIN = 1;           //用户登录游戏时加锁
    int const UN_LOCK_USER_LEAVE = -1;       //用户退出游戏时解锁

    std::list<int>  m_leaveTableList;        // 离开房间队列
    NFTimer   m_lastRunTick;           // 计时器
public:
	virtual int GetYaoyaoUserList(std::map<int, SArrangeUserList*>& retMap) { return 0; }
	virtual int GetYaoyaoUserListIgnoreSelScore(SArrangeUserList& retList) { return 0; }
};

#endif
