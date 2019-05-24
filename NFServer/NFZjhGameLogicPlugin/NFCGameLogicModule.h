#ifndef __WORLD_GAMEAREA_HEAD__
#define __WORLD_GAMEAREA_HEAD__

//#include "world.h"
//#include "pluto.h"
#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/type_mogo_def.hpp"
#include "NFCTableManagerModule.h"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFITableManagerModule.h"
//#include "../../NFComm/NFPluginModule/NFIWorldGameAreaModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
//#include "NFComm/NFPluginModule/NFINetCommonModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ClientModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFQsTaskStruct.hpp"

class NFCGameLogicModule : public NFIGameLogicModule, NFMapEx<int, NFIItemFillProcessModule>
{
public:
	NFCGameLogicModule(NFIPluginManager* p);
	~NFCGameLogicModule();

	//static CWorldGameArea* Inst();

	virtual bool Awake() override;

	virtual bool Init() override;

	virtual bool AfterInit() override;

	virtual bool Execute() override;
public:
	int init(const char* pszEtcFile);
	//int FromRpcCall(CPluto& u);
	//void OnThreadRun();
	void OnServerStart();
	int OnFdClosed(NFGUID fd);

	//todo
	//int ProcUserInfoToDb(uint32_t taskId, int32_t userId, string accessToken, int32_t gameRoomId, int32_t type, int srcFd);
	virtual void ProcUserInfoToDb(uint32_t taskId, int32_t userId, std::string accessToken, int32_t gameRoomId, int32_t gameLock, int srcFd);

	//int ProcUserLogReportToDb(int srcFd);
	//togo 
	//int ProcClientLogin(NFGamespace::ClientLoginStruct & p, int srcFd);

	int OnMsGidClientLogin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnMsGidClientUpdateUserInfo(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnMsGidClientOnTick(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnProcClientSit(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnProcClientReady(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int	OnProcClienGBet(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnProcClientSeeCard(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnProcClienGTrust(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
	int OnMsGidClientChat(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);
private:
	int ClientLoginResponse(const NFGUID& clientID, const int32_t retCode, std::string retErrorMsg);
	void ClientUpdateUserInfoResponse(const NFGUID& clientFd, const int32_t retCode, const char* retErrorMsg);
	bool CheckClientRpc(CPluto& u);
	void CheckTaskListTimeOut();

	int SendBulletin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

	void OnTaskTimeOutProcess(const int taskId, const int msgId);
public:
	inline bool OnCloseed(const NFGUID & guid)
	{
		if (FindUserByGuid(guid))
		{
			NF_SHARE_PTR<SUserInfo> info = GetUserInfoByGuid(guid);
			m_MapUserIdForUserInfo.RemoveElement(info->baseInfo.userId);
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
	inline void RemoveUserByGuid(const NFGUID& guid)
	{
		m_MapGuidForUserInfo.RemoveElement(guid);
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

	inline void SendMsgToClient(const uint16_t nMsgId, const nlohmann::json & jsonObject, const NFGUID& clientGuid) override
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

private:
	//         map<int, SUserInfo*> m_fd2userInfo;                     //socket fd和userInfo的关联关系
	//         map<int, SUserInfo*> m_userId2userInfo;                 //userId和userInfo的关联关系
	//         map<uint32_t, CAreaTaskItemBase*> m_taskList;           //任务列表
	NFTimer m_checkTaskTime;                          //检测任务超时
	NFTimer m_reportNumTime;                          //定时上报服务器信息
	NFTimer m_checkReadRobotUserInfoTime;             //定时检测是否需要读取机器人信息
	NFTimer m_checkUserTimeoutTime;                   //定时检测用户是否超时无数据包
	//         bool m_tryReportStart2FS;								// 开启服务器时，上报游戏服务信息给FS
	NFITableManagerModule * m_pTableManagerModule;
	NFIConfigAreaModule * m_pConfigAreaModule;
	//		 NFINetCommonModule * m_pNetCommonModule;
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
   //CWorldGameArea(const CWorldGameArea&);
   //CWorldGameArea& operator=(const CWorldGameArea&);
public:
	virtual void GameMsgSenderUnlock(int tableHandle, int userId, int roomId);


	virtual void GameMsgSenderLock(int tableHandle, int userId, int roomId);
	virtual void GameMsgSenderLockOrUnLock(uint32_t taskId, int32_t gameRoomId, int32_t userId, int32_t gameServerId, int32_t type) override;
	virtual void GameMsgSenderCheckerOpenDoor(int gameRoomId, SUserBaseInfo& baseInfo, int& retCode, std::string & retError) override;
	//virtual bool ResgisterFillProcessModule(const int nModuleType, NF_SHARE_PTR<NFIItemFillProcessModule> pModule) override;
	//virtual NF_SHARE_PTR<NFIItemFillProcessModule> GetFillProcessModule(const int nModuleType) override;
	virtual int GetCardOfGroup(size_t index) { return 0; }

	virtual void DealCardToTable(NFGameTable* table) {}

	virtual void FillCardToTable(NFGameTable* table) {}

	virtual void CalculateResultOfBetArea(NFGameTable* table, std::map<int, float>& result) {}

	virtual float GetLossPerCentByBetId(int betId) { return 0.0; }

	virtual int GetYaoyaoUserList(std::map<int, SArrangeUserList*>& retMap);
	virtual int GetYaoyaoUserListIgnoreSelScore(SArrangeUserList& retList);
};
// 
#endif
