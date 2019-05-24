#ifndef NFC_GAME_LOGIC_MODULE_CPP
#define NFC_GAME_LOGIC_MODULE_CPP

#include "NFComm/NFCore/NFDateTime.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "NFComm/NFPluginModule/NFIItemFillProcessModule.h"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFCGameLogicModule.h"
#include "Dependencies/json/json.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFQsTaskStruct.hpp"
#include "../NFTableModule/NFGameTable.h"
#include "../NFTableModule/StructHPP/NFTableUser.hpp"
#include "NFBaccaratExpendLogic.h"


NFCGameLogicModule::NFCGameLogicModule(NFIPluginManager* p)
{
    pPluginManager = p;
}

NFCGameLogicModule::~NFCGameLogicModule()
{

}

bool NFCGameLogicModule::Awake()
{
    return true;
}

bool NFCGameLogicModule::Init()
{
    //m_pWorldGameAreaModule = pPluginManager->FindModule<NFIWorldGameAreaModule>();
	//m_pGameServerNet_ClientModule = pPluginManager->FindModule<NFIGameServerNet_ClientModule>();
	m_pNFIGameServerToDBModule =  pPluginManager->FindModule<NFIGameServerToDBModule>();


    m_pTableManagerModule = pPluginManager->FindModule <NFITableManagerModule>();
    m_pConfigAreaModule = pPluginManager->FindModule<NFIConfigAreaModule>();
    m_pRobotModule = pPluginManager->FindModule<NFIRobotModule>();
    m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pNetModule = pPluginManager->FindModule<NFINetModule>();
	m_pNetClientModule = pPluginManager->FindModule<NFINetClientModule>();
	m_pQsTaskScheduleModule = pPluginManager->FindModule<NFIQsTaskScheduleModule>();
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();

	m_pNFBaccaratExpendLogic = std::make_shared<NFBaccaratExpendLogic>(pPluginManager);
	if (m_pNFBaccaratExpendLogic)
		m_pNFBaccaratExpendLogic->Init();


    return true;
}

bool NFCGameLogicModule::AfterInit()
{
	m_pNetModule->AddReceiveCallBack(NFMsg::EGMI_REQ_LEAVE_GAME, this, &NFCGameLogicModule::OnClientLeaveGameProcess);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_LOGIN, this, &NFCGameLogicModule::OnMsGidClientLogin);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_UPDATE_USERINFO, this, &NFCGameLogicModule::OnMsGidClientUpdateUserInfo);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_ONTICK, this, &NFCGameLogicModule::OnMsGidClientOnTick);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_ENTER, this, &NFCGameLogicModule::OnMsGidClientEnterRoom);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_LEAVE, this, &NFCGameLogicModule::OnMsGidClientLeaveRoom);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_CHAT, this, &NFCGameLogicModule::OnMsGidClientChat);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT,NFMsg::MSGID_CLIENT_BET, this, &NFCGameLogicModule::OnMsGidClientBet);

	m_pQsTaskScheduleModule->AddTaskTimeOutCallBack(this, &NFCGameLogicModule::OnTaskTimeOutProcess);

	m_pConfigAreaModule->InitCfg(NFGamespace::EG_BACCARAT);
	m_pRobotModule->InitCfg(NFGamespace::EG_BACCARAT);

    return true;
}

bool NFCGameLogicModule::Execute()
{
    if (m_lastRunTick.GetTimePassMillionSecond() > 300)
    {
        ProcLeaveTableList();
		m_lastRunTick.SetNowTime();
    }
    
    return true;
}

void NFCGameLogicModule::ProcLeaveTableList()
{
    while (!m_leaveTableList.empty())
    {
        int userId = m_leaveTableList.front();

        int chairIndex = 0;
        NF_SHARE_PTR<NFGameTable> table = m_pTableManagerModule->GetUserTable(userId, chairIndex);
        if (table)
        {
            table->LeaveTable(userId);
        }
        else{
            m_pLogModule->LogError("!!!!!!!!!!!!! table error", __FUNCTION__, __LINE__);
        }
    }
}

void NFCGameLogicModule::OnClientLeaveGameProcess(const NFSOCK nSockIndex, const int nMsgID, const char *msg, const uint32_t nLen)
{
	CLIENT_MSG_PROCESS_NO_OBJECT(nMsgID, msg, nLen, NFMsg::ReqLeaveGameServer);
	// TODO  OnFdClose 清除数据
	std::ostringstream log;
	log << nPlayerID.ToString() << "---> 断开连接." << "TODO:清除数据";
	m_pLogModule->LogInfo(log, __FUNCTION__, __LINE__);

	if (!FindUserByGuid(nPlayerID))
	{
		std::ostringstream log;
		log << "没有该guid映射:" << nPlayerID.ToString();
		m_pLogModule->LogWarning(log, __FUNCTION__, __LINE__);
		return;
	}
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(nPlayerID);
	int userId = pUser->baseInfo.userId;
	RemoveUserByGuid(nPlayerID);

	if(!FindUserByUserId(userId))
	{
		std::ostringstream log;
		log << "没有该userId的映射:" << userId;
		m_pLogModule->LogWarning(log, __FUNCTION__, __LINE__);
		return;
	}
	RemoveUserByUserId(userId);

	m_pTableManagerModule->OnUserOffline(userId);
}

int NFCGameLogicModule::ClientLoginResponse(const NFGUID& clientID, const int32_t retCode, std::string retErrorMsg)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
	if (!pUser)
	{
		m_pLogModule->LogError("m_fd2userInfo.end() == iter", __FUNCTION__, __LINE__);
		return -1;
	}

	nlohmann::json jsonObject;

	int action = NFMsg::MSGID_CLIENT_LOGIN_RESP;
	jsonObject["action"] = action;
	jsonObject["code"] = retCode;
	std::string msg = retErrorMsg;
	jsonObject["msg"] = msg;

	pUser->baseInfo.WriteToPluto(jsonObject);
	m_pConfigAreaModule->writeBetNumToPluto(jsonObject);

	jsonObject["betTime"] = m_pConfigAreaModule->bet_Time;
	jsonObject["userMaxBetAmount"] = m_pConfigAreaModule->user_bet_maxnum;
	std::string ttestString = jsonObject.dump();
	SendMsgToClient(NFMsg::MSGID_CLIENT_LOGIN_RESP, jsonObject, clientID);

	if (0 != retCode)
	{
		if (EUS_NONE != pUser->activeInfo.userState)
			m_pLogModule->LogInfo("EUS_NONE != pUser->ativeInfo.userState", __FUNCTION__, __LINE__);

		// 登录失败，可以重试
		RemoveUserByGuid(clientID);
	}

    return 0;
}

void NFCGameLogicModule::ClientUpdateUserInfoResponse(const NFGUID& clientFd, const int32_t retCode, const char* retErrorMsg)
{
	// TODO
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientFd);
	if (!pUser)
	{
		m_pLogModule->LogInfo("find user failed", __FUNCTION__, __LINE__);
		return;
	}

	nlohmann::json jsonObject;

	jsonObject["action"] = NFMsg::MSGID_CLIENT_UPDATE_USERINFO_RESP;
	jsonObject["code"] = retCode;
	jsonObject["msg"] = retErrorMsg;

    pUser->baseInfo.WriteToPluto(jsonObject);

	SendMsgToClient(NFMsg::MSGID_CLIENT_UPDATE_USERINFO_RESP, jsonObject, clientFd);
}

int NFCGameLogicModule::OnMsGidClientOnTick(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	NFGamespace::ClientOntick tickStruct;
	try
	{
		tickStruct = jsonObject;
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
	}

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_ONTICK_RESP;
	jsonSend["tick"] = tickStruct.tick;
	SendMsgToClient(NFMsg::MSGID_CLIENT_ONTICK_RESP, jsonSend, clientID);
    return 1;
}

int NFCGameLogicModule::OnMsGidClientLogin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{

	string accessToken = "";
	string mac = "";
	int32_t whereFrom = 0;
	int32_t userId = 0;
	try
	{
		NFGamespace::ClientLoginStruct loginStruct = jsonObject;
		accessToken = loginStruct.accessToken;
		mac = loginStruct.mac;
		whereFrom = loginStruct.whereFrom;
		userId = loginStruct.userId;
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);

		return -1;
	}

	if (GetUserInfoByGuid(clientID))
	{
		std::ostringstream logStr;
		logStr << "同一个连接登陆请求：GUID = " << clientID.ToString();
		m_pLogModule->LogWarning(logStr, __FUNCTION__, __LINE__);
		return 0;
	}

	NF_SHARE_PTR<SUserInfo> pUser = std::make_shared<SUserInfo>();
	pUser->activeInfo.fd = nSockIndex;
	pUser->activeInfo.whereFrom = whereFrom;
	pUser->activeInfo.mac = mac;
	pUser->self = clientID;
	AddUserToGuid(clientID, pUser);

	if (!m_pConfigAreaModule->IsInited())
	{
		std::string errorMsg = NFUtil::EncodStringForJson("服务器准备中");
		// 登录失败
		ClientLoginResponse(clientID, 100, errorMsg);
		return -1;
	}

	if (!m_pRobotModule->GetIsInit())
	{
		std::string errorMsg = NFUtil::EncodStringForJson("服务器准备中");
		// 登录失败
		 ClientLoginResponse(clientID, 101, errorMsg);
		return -1;
	}

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		std::string errorMsg = NFUtil::EncodStringForJson("服务器准备中");
		// 登录失败
		ClientLoginResponse(clientID, 101, errorMsg);
		return -1;
	}

	std::ostringstream logStr;
	logStr << "accessToken = " << accessToken << "; mac = " << mac << "; whereFrom = " << whereFrom;
	m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);


	NF_SHARE_PTR<CAreaTaskItemBase> readUserItask = make_shared<CAreaTaskReadUserInfo>(NFMsg::MSGID_CLIENT_LOGIN, clientID);
	m_pQsTaskScheduleModule->AddTask(readUserItask);
	ProcUserInfoToDb(readUserItask->GetTaskId(), userId, accessToken, m_pConfigAreaModule->gameRoomId, 0, nSockIndex);
    return 1;
}

int NFCGameLogicModule::OnMsGidClientUpdateUserInfo(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{

	NF_SHARE_PTR<SUserInfo> mUserInfo = GetUserInfoByGuid(clientID);
	if (!mUserInfo)
	{
		std::ostringstream logStr;
		logStr << "can't find clientId for " << clientID.ToString();
		return -1;
	}

	int userId = mUserInfo->baseInfo.userId;

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		std::string errorMsg = NFUtil::EncodStringForJson("服务器维护中");
		// 登录失败
		ClientLoginResponse(clientID, 101, errorMsg);
		return -1;
	}

	if (m_pNFIGameServerToDBModule)
	{
		NF_SHARE_PTR<CAreaTaskItemBase> readUserItask = make_shared<CAreaTaskReadUserInfo>(NFMsg::MSGID_CLIENT_UPDATE_USERINFO, clientID);
		m_pQsTaskScheduleModule->AddTask(readUserItask);
		ProcUserInfoToDb(1, (int32_t)0, "", m_pConfigAreaModule->gameRoomId, 0, nSockIndex);

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}
    return 1;
}

int NFCGameLogicModule::OnMsGidClientEnterRoom(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
	if (!pUser)
	{
		std::ostringstream logStr;
		logStr << "find user error  GUID = " << clientID.ToString();
		m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
		return -1;
	}

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		return -1;
	}

	int32_t retCode = 0;
	std::string retMsg = "";

	NFGamespace::ClientEnter xEnterStruct;
	try
	{
		xEnterStruct = jsonObject;
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
	}

	int betScore = xEnterStruct.id;
	// 1 是 贵宾场[筹码, 1 5 10 20 50 100 ]  0 是富豪场[筹码，100 200 500 1000 2000 5000 ]
	pUser->activeInfo.selScore = betScore;


	// 检测断线返回
	bool isOfflineRet = false;
	NF_SHARE_PTR<NFGameTable> pTable = nullptr;
	int chairIndex = 0;
	{

		int userId = pUser->baseInfo.userId;
		pTable = m_pTableManagerModule->GetUserTable(userId, chairIndex);
		if (pTable)
		{
			//isOfflineRet = pTable->EnterTable(pUser, chairIndex, false);
			if (!isOfflineRet)
			{
				m_pLogModule->LogError("EnterTable failed", __FUNCTION__, __LINE__);
				std::string msg = "进入游戏失败";
				nlohmann::json jsonSend;
				jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP;
				jsonSend["code"] = 12;
				jsonSend["msg"] = NFUtil::EncodStringForJson(msg);
				SendMsgToClient(NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP, jsonSend, clientID);
			}

			//发送进入房间成功包
			nlohmann::json jsonSend;
			jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP;
			jsonSend["code"] = retCode;
			jsonSend["msg"] = retMsg;
			SendMsgToClient(NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP, jsonSend, clientID);


			//发送桌子状态
			pTable->SendTableStateToOneUser(pUser, pTable->GetCurStateEnum<TableState>(), -1);
			//发送桌子记录
			pTable->SendBroadTableRecord(pUser->self);
			//发送桌子上的所有下注信息、牌型信息、玩家金币
			pTable->SendUserBeanAndCardTypeAndBetBeanOfTable(pUser->activeInfo.fd);


			std::ostringstream logStr;
			logStr << "offline return success userId = " << pUser->baseInfo.userId << " table = " << pTable->GetHandle() << " chairIndex = " << chairIndex;
			m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

			return 0;
		}
	}

	//不是断线返回，是新加入房间的
	if (!m_pTableManagerModule->EnterRoomBySelectSocre(pUser, -1, [&]()->void {
		//发送进入房间成功包
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP;
		jsonSend["code"] = retCode;
		jsonSend["msg"] = retMsg;
		SendMsgToClient(NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP, jsonSend, clientID);
	}))
	{
		std::string msg = "进入游戏失败";
		//发送进入房间失败包
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP;
		jsonSend["code"] = 12;
		jsonSend["msg"] = NFUtil::EncodStringForJson(msg);
		SendMsgToClient(NFMsg::baccarat_MSGID_CLIENT_ENTER_RESP, jsonSend, clientID);
	}

	return 0;
}

int NFCGameLogicModule::OnMsGidClientLeaveRoom(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
    if (!pUser)
    {
		std::ostringstream logStr;
		logStr << "find user error GUID = " << clientID.ToString();
        m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
        return -1;
    }

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		// TODO
		return -1;
	}

    int code = 1;

    NF_SHARE_PTR<NFGameTable> pTable = nullptr;
    int chairIndex = 0;
    int userId = pUser->baseInfo.userId;
    pTable = m_pTableManagerModule->GetUserTable(userId, chairIndex);
    if (pTable)
    {
        NF_SHARE_PTR<NFTableUser> pTableUesr = pTable->FindUserById(userId);
        if (pTableUesr && pTableUesr->GetBetAllSum() > 0)
            code = 0;
        else
        {
            pTable->LeaveTable(userId);
            code = 1;
        }
    }
    else
    {
        code = 1;
    }

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_LEAVE_RESP;
	jsonSend["code"] = code;
	jsonSend["msg"] = "";
	SendMsgToClient(NFMsg::MSGID_CLIENT_LEAVE_RESP, jsonSend, clientID);
    return 0;
}


int NFCGameLogicModule::OnMsGidClientChat(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
    if (pUser)
    {
        int chairIndex = 0;
        NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetPUserTable(*pUser, chairIndex);
        if (pTable)
            return pTable->ProcClientChat(jsonObject, chairIndex);
    }

    return -1;
}

int NFCGameLogicModule::OnMsGidClientBet(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
    if (pUser)
    {
        int chairIndex = 0;
        NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetPUserTable(*pUser, chairIndex);
        if (pTable)
            return pTable->ProcClientBet(jsonObject, chairIndex);
    }

    return -1;
}

void NFCGameLogicModule::GameMsgSenderUnlock(int tableHandle, int userId, int roomId)
{
    if (-1 == tableHandle)
        return;

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		return;
	}

	NF_SHARE_PTR<CAreaTaskItemBase> task = std::make_shared<CAreaTaskReportScore>(tableHandle,NFMsg::MSGID_DBMGR_UNLOCK_GAME_PLAYER);
	if (!task)
		return;

	m_pQsTaskScheduleModule->AddTask(task);

    GameMsgSenderLockOrUnLock(task->GetTaskId(), roomId, userId, NFGamespace::EG_BACCARAT, UN_LOCK_USER_LEAVE);

}

void NFCGameLogicModule::GameMsgSenderLock(int tableHandle, int userId, int roomId)
{
    if (-1 == tableHandle)
        return;

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		return;
	}

	NF_SHARE_PTR<CAreaTaskItemBase> task = std::make_shared<CAreaTaskReportScore>(tableHandle, NFMsg::MSGID_DBMGR_LOCK_GAME_PLAYER);
	if (!task)
		return;

	m_pQsTaskScheduleModule->AddTask(task);

    GameMsgSenderLockOrUnLock(task->GetTaskId(), roomId, userId, NFGamespace::EG_BACCARAT, LOCK_USER_LOGIN);

}

void NFCGameLogicModule::GameMsgSenderLockOrUnLock(uint32_t taskId, int32_t gameRoomId, int32_t userId, int32_t gameServerId, int32_t type)
{
	uint64_t stamp = NFUtil::GetTimeStampInt64Ms();
	char buffer[10000];
	memset(buffer, 0, sizeof(buffer) / sizeof(char));

	// 获得校验值
	snprintf(buffer, sizeof(buffer), "%d%d%d%llu%s", userId, gameRoomId, type, stamp, m_pConfigAreaModule->m_cis_key.c_str());
	string tmpStr = buffer;

	string md5;
	md5 = MD5(tmpStr).toStr();
	// postdata
	snprintf(buffer, sizeof(buffer), "action=LockGameRoom&userId=%d&gameRoomId=%d&gameServerId=%d&type=%d&timeOutMin=%d&timestamp=%llu&checkCode=%s",
		userId, gameRoomId, gameServerId, type, 5, stamp, md5.c_str());
	tmpStr = buffer;

	// TODO
	if (m_pNFIGameServerToDBModule)
	{
		short dataLen = tmpStr.size();
		m_pNFIGameServerToDBModule->SendDataToDb(NFMsg::MSGID_DBMGR_LOCK_GAMEROOM_TODB, taskId, gameRoomId, dataLen, tmpStr.c_str());

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}
	std::ostringstream logUnLock;
	logUnLock << "解锁/上锁用户,ID = " << userId << tmpStr;
	m_pLogModule->LogInfo(logUnLock, __FUNCTION__, __LINE__);
}

void NFCGameLogicModule::GameMsgSenderCheckerOpenDoor(int gameRoomId, SUserBaseInfo& baseInfo, int& retCode, std::string & retError)
{

	if (1 == baseInfo.gameRoomLockStatus)
	{
		std::ostringstream logStr1;
		logStr1 << "LastLockState: LockState = " << baseInfo.gameRoomLockStatus << " lockRoomid = " << baseInfo.lastLockGameRoomId << " userId = " << baseInfo.userId;
		m_pLogModule->LogInfo(logStr1, __FUNCTION__, __LINE__);

		if ((gameRoomId == baseInfo.lastLockGameRoomId) || (0 == baseInfo.lastLockGameRoomId))
		{
			std::ostringstream logStr2;
			logStr2 << "Unlock:Open door, lockState = " << baseInfo.gameRoomLockStatus << " lockRoomid = " << baseInfo.lastLockGameRoomId << " userId = " << baseInfo.userId << "gameRoomId = " << gameRoomId;
			retCode = 0;
			m_pLogModule->LogInfo(logStr2, __FUNCTION__, __LINE__);
			retError = std::string();
		}
		else
		{
			retCode = 101;
			retError = "进入房间失败,您已经进入其他游戏!";
			m_pLogModule->LogInfo("Unlock!!!!", __FUNCTION__, __LINE__);
		}
	}
}

void NFCGameLogicModule::ProcUserInfoToDb(uint32_t taskId, int32_t userId, string accessToken, int32_t gameRoomId, int32_t gameLock, int srcFd)
{
	int32_t retCode = 0;
	string retMsg = "";
	SUserBaseInfo userInfo;

	string aes_key = m_pConfigAreaModule->m_aes_key;

	string accessTokenToDb = accessToken;

	uint64_t stamp = NFGetTimeMS();
	char buffer[1024];

	snprintf(buffer, sizeof(buffer), "%d", C_GAME_ID);
	string strGameId(buffer);
	string m_cis_key = "goisnfgsf34-9f25";
	// 获得校验值
	snprintf(buffer, sizeof(buffer), "%d%s%d%d%llu%s", userId, strGameId.c_str(), gameLock, gameRoomId, stamp, m_cis_key.c_str());
	string tmpStr = buffer;
	string md5;
	md5 = MD5(tmpStr).toStr();
	// postdata
	snprintf(buffer, sizeof(buffer), "action=GetUserInfos&userId=%d&gameId=%s&gameLock=%d&gameRoomId=%d&timestamp=%llu&checkCode=%s&token=%s", userId, strGameId.c_str(), gameLock, gameRoomId, stamp, md5.c_str(), accessTokenToDb.c_str());
	tmpStr = buffer;
	//CMailBox* mbDbmgr = m_pNetCommonModule->GetServerMailbox(SERVER_DBMGR);
	if (m_pNFIGameServerToDBModule)
	{
		short dataLen = tmpStr.size();
		m_pNFIGameServerToDBModule->SendReadUserInfo(taskId, gameRoomId, dataLen, tmpStr.c_str());

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}

}


void NFCGameLogicModule::OnTaskTimeOutProcess(const int taskId, const int msgId)
{
	std::ostringstream logStr;
	logStr << "超时任务 TaskId = " << taskId << "; MsgId == " << msgId;
	m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);

	if(msgId == NFMsg::MSGID_ROBOT_READ_USERINFO)
	{
		NF_SHARE_PTR<CAreaTaskReadUserInfo> task = NFUtil::Cast<CAreaTaskReadUserInfo, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
		if (!task) return;
		NFGamespace::DB_UserBaseInfo baseInfo;
		m_pRobotModule->ReadUserInfoCallback(101, "读机器人超时", task->GetTaskId(), baseInfo);
		return;
	}

	if(msgId == NFMsg::MSGID_CLIENT_UPDATE_USERINFO)
	{
		NF_SHARE_PTR<CAreaTaskReadUserInfo> task = NFUtil::Cast<CAreaTaskReadUserInfo, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
		if (!task) return;
		ClientUpdateUserInfoResponse(task->GetGUId(), 101, "刷新超时");
		return;
	}

	if(msgId == NFMsg::MSGID_DBMGR_REPORT_SCORE)
	{
		NF_SHARE_PTR<CAreaTaskReportScore> task = NFUtil::Cast<CAreaTaskReportScore, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
		if(!task)
		{
			return;
		}
		NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetTableByHandle(task->GetTableHandle());

		if (pTable)
			pTable->OnReportScoreError(101, "处理数据超时");
		else
			m_pLogModule->LogError("find table error", __FUNCTION__, __LINE__);

		return;
	}

	if(msgId == NFMsg::MSGID_CLIENT_LOGIN)
	{
		NF_SHARE_PTR<CAreaTaskReadUserInfo> task = NFUtil::Cast<CAreaTaskReadUserInfo, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
		if(!task)
		{
			return;
		}
		ClientLoginResponse(task->GetGUId(), 101, "验证超时");
		return;
	}

	if (msgId == NFMsg::MSGID_DBMGR_UNLOCK_GAME_PLAYER)
	{
		NF_SHARE_PTR<CAreaTaskLockOrUnlockUser> task = NFUtil::Cast<CAreaTaskLockOrUnlockUser, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
		if (!task)
		{
			m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
			return;
		}
		NFMsg::RoleOfflineNotify xMsg;

		NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByUserId(task->GetClientUserId());

		if (pUser)
		{
			NFGUID self = pUser->self;

			*xMsg.mutable_self() = NFINetModule::NFToPB(self);
			*xMsg.mutable_guild() = NFINetModule::NFToPB(self);
			xMsg.set_game(pPluginManager->GetAppID());
			xMsg.set_proxy(0);

			std::string strMsg;
			if (!xMsg.SerializeToString(&strMsg))
			{
				m_pLogModule->LogError("Serlize strMsg fail ", __FUNCTION__, __LINE__);
				return;
			}
			m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::EGMI_ACK_OFFLINE_NOTIFY, strMsg.c_str(), self);
		}
	}
}
#endif