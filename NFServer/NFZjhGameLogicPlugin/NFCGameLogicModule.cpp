#ifndef __NFC_GAME_LOGIC_MODULE_CPP_
#define __NFC_GAME_LOGIC_MODULE_CPP_ 


/*----------------------------------------------------------------
 // 模块描述：区域服务器逻辑
 //----------------------------------------------------------------*/
 
 //#include "mailbox.h"
 //#include "epoll_server.h"
 #include "global_var.h"
 #include <string.h>
 //#include <curl/curl.h> 
#include "type_area.h"
#include "NFServer/NFGameServerNet_ServerPlugin/logger.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/exception.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "NFCGameLogicModule.h"

//全局静态变量
//static CWorldGameArea* gStaticGameArea = NULL;


NFCGameLogicModule::NFCGameLogicModule(NFIPluginManager* p)
{
	pPluginManager = p;
}

NFCGameLogicModule::~NFCGameLogicModule()
{
// 	CLEAR_POINTER_MAP(m_fd2userInfo);
// 	// 两者公用内存
// 	m_userId2userInfo.clear();
 }

bool NFCGameLogicModule::Awake()
{
	return true;
}

bool NFCGameLogicModule::Init()
{
	m_pNFIGameServerToDBModule = pPluginManager->FindModule<NFIGameServerToDBModule>();


	m_pTableManagerModule = pPluginManager->FindModule <NFITableManagerModule>();
	m_pConfigAreaModule = pPluginManager->FindModule<NFIConfigAreaModule>();
	//m_pNetCommonModule = pPluginManager->FindModule<NFINetCommonModule>();
	m_pRobotModule = pPluginManager->FindModule<NFIRobotModule>();
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pNetModule = pPluginManager->FindModule<NFINetModule>();
	m_pNetClientModule = pPluginManager->FindModule<NFINetClientModule>();
	m_pQsTaskScheduleModule = pPluginManager->FindModule<NFIQsTaskScheduleModule>();
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();
	/////////////////////////////////////////////////////////

	return true;
}

bool NFCGameLogicModule::AfterInit()
{
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_LOGIN, this, &NFCGameLogicModule::OnMsGidClientLogin);
 	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_UPDATE_USERINFO, this, &NFCGameLogicModule::OnMsGidClientUpdateUserInfo);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_ONTICK, this, &NFCGameLogicModule::OnMsGidClientOnTick);
 	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_CHAT, this, &NFCGameLogicModule::OnMsGidClientChat);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_SIT, this, &NFCGameLogicModule::OnProcClientSit);
	// 	//56 //MSGID_CLIENT_READY
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_READY, this, &NFCGameLogicModule::OnProcClientReady);

	// 	//58 MSGID_CLIENT_G_BET
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_G_BET, this, &NFCGameLogicModule::OnProcClienGBet);
	//MSGID_CLIENT_G_TRUST
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_G_TRUST, this, &NFCGameLogicModule::OnProcClienGTrust);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_SEE_CARD, this, &NFCGameLogicModule::OnProcClientSeeCard);
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_GOLDEN_FLOWERS, NFMsg::MSGID_CLIENT_BULLETIN_NOTIFY, this, &NFCGameLogicModule::SendBulletin);
	
	m_pQsTaskScheduleModule->AddTaskTimeOutCallBack(this, &NFCGameLogicModule::OnTaskTimeOutProcess);


	m_pConfigAreaModule->InitCfg(NFGamespace::EG_GOLDEN_FLOWERS);
	m_pRobotModule->InitCfg(NFGamespace::EG_GOLDEN_FLOWERS);

	return true;
}

int NFCGameLogicModule::init(const char* pszEtcFile)
{
//     int ret = CWorld::init(pszEtcFile);
// 
//     try
//     {
		m_pConfigAreaModule->InitCfg(NFGamespace::EG_GOLDEN_FLOWERS);
		m_pRobotModule->InitCfg(NFGamespace::EG_GOLDEN_FLOWERS);
//     }
//     catch (CException & ex)
//     {
//         LogError("CWorldGameArea::init", "error: %s", ex.GetMsg().c_str());
//         return -1;
//     }

    return 0;
}

int NFCGameLogicModule::OnFdClosed(NFGUID fd)
{
	return	OnCloseed(fd);
//     map<int, SUserInfo*>::iterator iter = m_fd2userInfo.find(fd);
//     if (m_fd2userInfo.end() == iter)
//     {
//         LogInfo("CWorldGameArea::OnFdClosed", "not login fd=%d", fd);
//         return -1;
//     }
// 
//     SUserInfo* pUser = iter->second;
//     m_fd2userInfo.erase(iter);
//     int userId = pUser->baseInfo.userId;
//     if (userId > 0)
//     {
//         m_userId2userInfo.erase(userId);
//         TABLE_MGR->OnUserOffline(userId);
//     }

//    LogInfo("$$$$$$$$$$$$$$$$$$$$CWorldLogin::OnFdClosed", "fd=%d;userId=%d", fd, userId);
    //delete pUser;
    //return 0;
}

bool NFCGameLogicModule::Execute()
{
	if (m_lastRunTick.GetTimePassMillionSecond() > 300)
	{
//		ProcLeaveTableList();
		m_lastRunTick.SetNowTime();
	}

  	if (m_checkUserTimeoutTime.GetTimePassMillionSecond() > 1018) {
  		m_checkUserTimeoutTime.SetNowTime();
  		//         int count = GetServer()->CheckUserTimeout();
  		//         if (count > 0)
  		//             LogInfo("CWorldGameArea::OnThreadRun", "CheckUserTimeout count=%d", count);
  	}
	// 桌子定时器
	return  m_pTableManagerModule->Execute();
}

void NFCGameLogicModule::OnServerStart()
{
//    CWorld::OnServerStart();
}


int NFCGameLogicModule::ClientLoginResponse(const NFGUID& clientID, const int32_t retCode, std::string retErrorMsg)
{
    //CMailBox* mb = GetServer()->GetMailboxByFd(clientFd);
	//if (!mb)
	//{
	//	LogInfo("CWorldGameArea::ClientLoginResponse", "!mb");
	//	return -1;
	//}
	//map<int, SUserInfo*>::iterator iter = FindUserByUserId(clientFd);
	//if (m_fd2userInfo.end() == iter)
	//{
	//	LogInfo("CWorldGameArea::ClientLoginResponse", "m_fd2userInfo.end() == iter");
	//	return -1;
	//}
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);


	// 自动释放内存
	//auto_new1_ptr<SUserInfo> autoUser(NULL);
	if (0 != retCode)
	{
		if (EUS_NONE != pUser->activeInfo.userState)
		{
			LogInfo("CWorldGameArea::ClientLoginResponse", "EUS_NONE != pUser->ativeInfo.userState");
			return -1;
		}
		LogInfo("CWorldGameArea::ClientLoginResponse", "登录失败，可以重试");
		// 登录失败，可以重试
		m_MapGuidForUserInfo.RemoveElement(pUser->self);
	//	autoUser.OverridePtr(pUser);
	}
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_LOGIN_RESP;
	jsonSend["code"] = retCode;
	jsonSend["msg"] = "";


	//CPluto* pu = new CPluto;
	//(*pu).Encode(MSGID_CLIENT_LOGIN_RESP) << retCode << retErrorMsg << pUser->activeInfo.userState;
	pUser->baseInfo.WriteToPluto(jsonSend);
	//(*pu) << EndPluto;
	//mb->PushPluto(pu);
	//SendPlutoToUser(NFMsg::MSGID_CLIENT_G_BET_RESP, jsonSend, pUser->baseInfo.userId);
	std::string msgStr = jsonSend.dump();
	m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::MSGID_CLIENT_G_BET_RESP, msgStr, pUser->self);
    return 0;
}

void NFCGameLogicModule::ClientUpdateUserInfoResponse(const NFGUID& clientFd, const int32_t retCode, const char* retErrorMsg)
{
	//CMailBox* mb = GetServer()->GetMailboxByFd(clientFd);
	//if (!mb)
	//{
	//    LogInfo("CWorldGameArea::ClientUpdateUserInfoResponse", "!mb");
	//    return -1;
	//}
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientFd);
	//SUserInfo* pUser = FindUserByFd(clientFd);
	if (!pUser)
	{
		LogInfo("CWorldGameArea::ClientUpdateUserInfoResponse", "find user failed");
		return  ;
	}

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_UPDATE_USERINFO_RESP;
	jsonSend["code"] = retCode;
	jsonSend["msg"] = "";

	pUser->baseInfo.WriteToPluto(jsonSend);
	std::string msgStr = jsonSend.dump();
	m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::MSGID_CLIENT_UPDATE_USERINFO_RESP, msgStr, pUser->self);
	//CPluto* pu = new CPluto;
	//(*pu).Encode(MSGID_CLIENT_UPDATE_USERINFO_RESP) << retCode << retErrorMsg;
	//(*pu) << EndPluto;
	//mb->PushPluto(pu);

	return  ;
}

bool NFCGameLogicModule::CheckClientRpc(CPluto& u)
{
//     bool bbase = CWorld::CheckClientRpc(u);
//     if (bbase)
//         return bbase;
// 
//     CMailBox* mb = u.GetMailbox();
//     if (!mb)
//     {
// 		LogError("bool CWorldGameArea::CheckClientRpc(CPluto& u)","mb is null");
//         return false;
//     }
// 
//     uint8_t authz = mb->GetAuthz();
// 
//     if (authz == MAILBOX_CLIENT_UNAUTHZ)
//     {
//         pluto_msgid_t msg_id = u.GetMsgId();
//         return msg_id == MSGID_CLIENT_LOGIN;
//     }
//     else
//     {
//         // 认证成功之后，可以发送哪些包
//         pluto_msgid_t msg_id = u.GetMsgId();
//         return msg_id > MSGID_CLIENT_MIN && msg_id < MSGID_CLIENT_MAX;
//     }
    return false;
}

void NFCGameLogicModule::CheckTaskListTimeOut()
{
  //  m_checkTaskTime.SetNowTime();

    // 获得超时任务列表
//     list<CAreaTaskItemBase*> lTimeout;
//     for (map<uint32_t, CAreaTaskItemBase*>::iterator iter = m_taskList.begin(); iter != m_taskList.end(); ++iter)
//     {
//         CAreaTaskItemBase* item = iter->second;
//         if (item->IsTimeout())
//         {
//             lTimeout.push_back(item);
//         }
//     }
//     // 处理超时任务
//     for (list<CAreaTaskItemBase*>::iterator iter = lTimeout.begin(); iter != lTimeout.end(); ++iter)
//     {
//         CAreaTaskItemBase* item = *iter;
//         switch (item->GetMsgId())
//         {
// 		case NFMsg::EGameMsgID::MSGID_DBMGR_REPORT_SCORE:
//         {
//             CAreaTaskReportScore* itemR = (CAreaTaskReportScore*)item;
//             CGameTable* pTable = TABLE_MGR->GetTableByHandle(itemR->GetTableHandle());
// 
//             if (pTable)
//                 pTable->OnReportScoreError(101, "处理数据超时");
//             else
//                 LogError("CWorldGameArea::CheckTaskListTimeOut", "REPORT_SCORE find table failed!");
// 
//             break;
//         }
//         case NFMsg::EGameMsgID::MSGID_CLIENT_LOGIN:
//         {
//             CAreaTaskReadUserInfo* itemR = (CAreaTaskReadUserInfo*)item;
//             ClientLoginResponse(itemR->GetClientFd(), 101, "验证超时", itemR);
//             break;
//         }
//         case NFMsg::EGameMsgID::MSGID_CLIENT_UPDATE_USERINFO:
//         {
//             CAreaTaskReadUserInfo* itemR = (CAreaTaskReadUserInfo*)item;
//             ClientUpdateUserInfoResponse(itemR->GetClientFd(), 101, "刷新超时", itemR);
//             break;
//         }
//         case NFMsg::EGameMsgID::MSGID_ROBOT_READ_USERINFO:
//         {
//             CAreaTaskReadUserInfo* itemR = (CAreaTaskReadUserInfo*)item;
//             SUserBaseInfo baseInfo;
//             ROBOT_MGR->ReadUserInfoCallback(101, "读机器人超时", itemR->GetClientFd(), baseInfo);
//             break;
//         }
// 
//         default:
//         {
// 			LogWarning("CWorldGameArea::CheckTaskListTimeOut", "not find msgid%d", item->GetMsgId());
//             break;
//         }
//         }
//     }
//     // 删除超时任务
//     for (list<CAreaTaskItemBase*>::iterator iter = lTimeout.begin(); iter != lTimeout.end(); ++iter)
//     {
//         CAreaTaskItemBase* item = *iter;
//         m_taskList.erase(item->GetTaskId());
//         delete item;
//     }
}

// void CWorldGameArea::ReportStart2FS()
// {
//     CMailBox* mbDbmgr = this->GetMailboxByServerID(SERVER_AREAMGR);
//     if (mbDbmgr)
//     {
//         if (mbDbmgr->IsConnected())
//         {
//             int32_t gameServerId = GetServer()->GetBindServerID();
//             int32_t	areaNum = CONFIG_AREA->area_num;
//             int32_t isMaster = 1;
//             int32_t masterFsId = CONFIG_AREA->masterFsId;
//             int32_t gameRoomId = CONFIG_AREA->gameRoomId;
//             int64_t totalMemorySize = 0;
//             int32_t maxPlayer = 900;
//             int32_t deskCount = 225;
//             string gameServerIp;
//             uint16_t gameServerPort;
//             GetServer()->GetBindServerAddr(gameServerIp, gameServerPort);
//             string maxJingDu = "";
//             string minJingDu = "";
//             string maxWeiDu = "";
//             string minWeiDu = "";
//             string entryRestriction = "";
// 
//             // 创建任务
//             CAreaTaskReport2FS* task = new CAreaTaskReport2FS(FS_REPORT_TYPE_START);
//             WORLD_GAME_AREA->AddTask(task);
// 
// 
//             char buffer[256];
//             snprintf(buffer, sizeof(buffer), "%d", gameServerId);
//             string strGameServerId(buffer);
//             snprintf(buffer, sizeof(buffer), "%d", areaNum);
//             string strAreaNum(buffer);
//             snprintf(buffer, sizeof(buffer), "%d", isMaster);
//             string strIsMaster(buffer);
//             snprintf(buffer, sizeof(buffer), "%d", masterFsId);
//             string strMasterFsId(buffer);
//             snprintf(buffer, sizeof(buffer), "%d", gameRoomId);
//             string strGameRoomId(buffer);
//             snprintf(buffer, sizeof(buffer), "%lld", totalMemorySize);
//             string strTotalMemorySize(buffer);
//             snprintf(buffer, sizeof(buffer), "%d", maxPlayer);
//             string strMaxPlayer(buffer);
//             snprintf(buffer, sizeof(buffer), "%d", deskCount);
//             string strDeskCount(buffer);
//             snprintf(buffer, sizeof(buffer), "%u", gameServerPort);
//             string strGameServerPort(buffer);
// 
//             string strJingWeiDu("");
//             snprintf(buffer, sizeof(buffer), "{maxJingDu: %s, minJingDu: %s, maxWeiDu: %s, minWeiDu: %s}",
//                 maxJingDu.c_str(), minJingDu.c_str(), maxWeiDu.c_str(), minWeiDu.c_str());
//             strJingWeiDu += buffer;
// 
//             string strEntryRestriction("");
// 
//             string postData("gameServerId=");
//             postData += strGameServerId;
//             postData += "&areaNum=";
//             postData += strAreaNum;
//             postData += "&isMaster=";
//             postData += strIsMaster;
//             postData += "&masterFsId=";
//             postData += strMasterFsId;
//             postData += "&gameRoomId=";
//             postData += strGameRoomId;
//             postData += "&totalMemorySize=";
//             postData += strTotalMemorySize;
//             postData += "&maxPlayers=";
//             postData += strMaxPlayer;
//             postData += "&deskCount=";
//             postData += strDeskCount;
//             postData += "&gameServerIp=";
//             postData += gameServerIp;
//             postData += "&gameServerPort=";
//             postData += strGameServerPort;
//             postData += "&jingWeiDu=";
//             postData += strJingWeiDu;
//             postData += "&entryRestriction=";
//             postData += strEntryRestriction;
// 
// 
//             CPluto* pu = new CPluto;
//             (*pu).Encode(MSGID_AREAMGR_REPORT_FS) << task->GetTaskId()
//                 << "gsinterface/start" << postData << EndPluto;
// 
//             mbDbmgr->PushPluto(pu);
//         }
//         else
//         {
//             LogError("CWorldGameArea::ReportStart2FS", "areaMgr not connected");
//         }
//     }
// }

int NFCGameLogicModule::SendBulletin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
      int32_t areaId = 0;
      int32_t bltType = 0;
      string btlMsg = "";
      int index = 0;
	  try
	  {
		  areaId = jsonObject.at("areaId").get<int>();
		  bltType = jsonObject.at("bltType").get<int>();
		  btlMsg = jsonObject.at("btlMsg").get<std::string>();
	  }
	  catch (const nlohmann::detail::exception& ex)
	  {
		  return -1;
	  }

	  NF_SHARE_PTR<SUserInfo>  it = m_MapGuidForUserInfo.First();
     while(it)
      {
		  nlohmann::json jsonSend;
		  jsonSend["action"] = NFMsg::MSGID_CLIENT_BULLETIN_NOTIFY;
		  jsonSend["bltType"] = bltType;
		  jsonSend["btlMsg"] = btlMsg;
		  std::string msgStr = jsonSend.dump();
		  m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::MSGID_CLIENT_BULLETIN_NOTIFY, msgStr, it->self);

		  it = m_MapGuidForUserInfo.Next();
      }

    return 0;
}
// 
int NFCGameLogicModule::GetYaoyaoUserList(map<int, SArrangeUserList*>& retMap)
{
     int ret = 0;
// //    CLEAR_POINTER_MAP(retMap);
// 
      //for (map<int, SUserInfo*>::iterator iter = m_MapUserIdForUserInfo.begin(); iter != m_MapUserIdForUserInfo.end(); ++iter)
	 auto  pModule = m_MapUserIdForUserInfo.First();
	 while(pModule)
	 {
          //SUserInfo* item = pModule->baseInfo;
          if (EUS_YAOYAO == pModule->activeInfo.userState)
          {
              SArrangeUserList* pUserAry = NULL;
              int selScore = pModule->activeInfo.selScore;
              map<int, SArrangeUserList*>::iterator itMap = retMap.find(selScore);
              if (itMap != retMap.end())
              {
                  pUserAry = itMap->second;
              }
              else
              {
                  pUserAry = new SArrangeUserList();
                  pUserAry->selScore = selScore;
                  retMap.insert(make_pair(selScore, pUserAry));
              }
			 if (pUserAry->len < MAX_FILE_HANDLE_COUNT)
			 {
				 pUserAry->UserAry[pUserAry->len++] = pModule;
				 
			 }
			 ++ret;
          }
		  pModule = m_MapUserIdForUserInfo.Next();
      }
      if (ret)
          LogInfo("CWorldGameArea::GetYaoyaoUserList", "current yaoyao count=%d", ret);
     return ret;
 }

 int NFCGameLogicModule::GetYaoyaoUserListIgnoreSelScore(SArrangeUserList& retList)
 {
     int ret = 0;
 
     //for (map<int, SUserInfo*>::iterator iter = m_userId2userInfo.begin(); iter != m_userId2userInfo.end(); ++iter)
	 auto  pModule = m_MapUserIdForUserInfo.First();
	 while (pModule)
	 {
         //SUserInfo* item = iter->second;
         if (EUS_YAOYAO == pModule->activeInfo.userState)
         {
			 if (retList.len < MAX_FILE_HANDLE_COUNT)
			 {
				 retList.UserAry[retList.len++] = pModule;
				 ++ret;
			 }
       
         }
		 pModule = m_MapUserIdForUserInfo.Next();
     }
 
     return ret;
 }


void  NFCGameLogicModule::ProcUserInfoToDb(uint32_t taskId, int32_t userId, string accessToken, int32_t gameRoomId, int32_t gameLock, int srcFd)
{

	uint64_t stamp = NFGetTimeMS();
	char buffer[1024];

	const int C_GAME_ID = 8; // 游戏id
	snprintf(buffer, sizeof(buffer), "%d", C_GAME_ID);
	string strGameId(buffer);
	string m_cis_key = "goisnfgsf34-9f25";
	// 获得校验值
	snprintf(buffer, sizeof(buffer), "%d%s%d%d%llu%s", userId, strGameId.c_str(), gameLock, gameRoomId, stamp, m_cis_key.c_str());
	string tmpStr = buffer;
	string md5;
	md5 = MD5(tmpStr).toStr();
	// postdata
	snprintf(buffer, sizeof(buffer), "action=GetUserInfos&userId=%d&gameId=%s&gameLock=%d&gameRoomId=%d&timestamp=%llu&checkCode=%s&token=%s",
		userId, strGameId.c_str(), gameLock, gameRoomId, stamp, md5.c_str(), accessToken.c_str());
	tmpStr = buffer;

	if (m_pNFIGameServerToDBModule)
	{
		short dataLen = tmpStr.size();
		m_pNFIGameServerToDBModule->SendReadUserInfo(taskId, gameRoomId, dataLen, tmpStr.c_str());

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}

	return  ;
}

//MSGID_AREA_READ_USERINFO_CALLBACK

void NFCGameLogicModule::OnTaskTimeOutProcess(const int taskId, const int msgId)
{
	std::ostringstream logStr;
	logStr << "超时任务 TaskId = " << taskId << "; MsgId == " << msgId;
	m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
	if (msgId == NFMsg::MSGID_CLIENT_LOGIN_RESP)
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


int NFCGameLogicModule::OnMsGidClientLogin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{

	if (!m_pConfigAreaModule->IsInited())
	{
		// 登录失败
		//  ClientLoginResponse(srcFd, 100, "服务器准备中");
		return -1;
	}

	if (!m_pRobotModule->GetIsInit())
	{
		// 登录失败
		//  ClientLoginResponse(srcFd, 101, "服务器维护中");
		return -1;
	}

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		// 登录失败
		//  ClientLoginResponse(srcFd, 101, "服务器维护中");
		//return -1;
	}

	string accessToken = "";
	string mac = "";
	int32_t whereFrom = 0;
	int userid = 0;
	try
	{
		NFGamespace::ClientLoginStruct loginStruct = jsonObject;
		accessToken = loginStruct.accessToken;
		mac = loginStruct.mac;
		whereFrom = loginStruct.whereFrom;
		userid = loginStruct.userId;
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);

		return -1;
	}
	catch (...)
	{
		m_pLogModule->LogError("error", __FUNCTION__, __LINE__);
		return -1;
	}

	std::ostringstream logStr;
	logStr << "accessToken = " << accessToken << "; mac = " << mac << "; whereFrom = " << whereFrom;
	m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

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

	NF_SHARE_PTR<CAreaTaskItemBase> readUserItask = make_shared<CAreaTaskReadUserInfo>(NFMsg::MSGID_CLIENT_LOGIN, clientID);
	m_pQsTaskScheduleModule->AddTask(readUserItask);
	ProcUserInfoToDb(readUserItask->GetTaskId(), userid, accessToken, m_pConfigAreaModule->gameRoomId, 0, nSockIndex);
	
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
		// 登录失败
		ClientLoginResponse(clientID, 101, "服务器维护中");
		return -1;
	}

	if (m_pNFIGameServerToDBModule)
	{
		NF_SHARE_PTR<CAreaTaskItemBase> readUserItask = make_shared<CAreaTaskReadUserInfo>(NFMsg::MSGID_CLIENT_UPDATE_USERINFO, 1330001);
		m_pQsTaskScheduleModule->AddTask(readUserItask);
		ProcUserInfoToDb(1, (int32_t)0, "", m_pConfigAreaModule->gameRoomId, 0, nSockIndex);

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}
	return 1;
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

	// TODO
	//m_pNetCommonModule->SendMail(share_pu, srcFd);
	return 1;
}

//MSGID_CLIENT_SIT
int NFCGameLogicModule::OnProcClientSit(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	int index = 0;
	float64_t jingDu = 0;
	float64_t weiDu = 0;
	int32_t maxTotalBet = 0;
	try
	{
		NFGamespace::ClientSitStruct xobj;
		NFGamespace::from_json(jsonObject, xobj);
		jingDu = xobj.jingdu;//(*p)[index++]->vv.f64;
		weiDu = xobj.weidu;//(*p)[index++]->vv.f64;
		maxTotalBet = xobj.maxTotalBet;  //(*p)[index++]->vv.i32;
		
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
		return -1;
	}
	std::ostringstream str;
	str << " clientID"<<  clientID.ToString() <<"jingDu"<< std::to_string(jingDu)<<"weiDu:"<< weiDu<<" maxTotalBet: "<< maxTotalBet;
	m_pLogModule->LogInfo(str, __FUNCTION__, __LINE__);
	

	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
	if (!pUser)
	{
		LogWarning("CWorldGameArea::ProcClientSit", "find user error");
		return -1;
	}

	
	int32_t retCode = 0;
	string retMsg = "";
	try
	{
	    SConfigScoreItem* pScoreItem =  m_pConfigAreaModule->score_list->FindItem(maxTotalBet);
	    if (!pScoreItem)
	        ThrowException(102, "选择的封顶不存在");
	    int minBean = pScoreItem->minBean;
	if (minBean > 0 && pUser->baseInfo.bean < minBean)
	{
		LogInfo("CWorldGameArea::ProcClientSit", "您的游戏%s小于%d, 不能满足房间要求", m_pConfigAreaModule->bean_name.c_str(), minBean);
//		ThrowException(ERROR_CODE_BEAN_TOO_LITTLE, "您的游戏%s小于%d, 不能满足房间要求", m_pConfigAreaModule->bean_name.c_str(), minBean / 100);
	}
	       
	
	    int userId = pUser->baseInfo.userId;
	    int lastTableHandle = -1;
	    // 检测是否可以换桌
	    int chairIndex = 0;
		NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetPUserTable(*pUser, chairIndex);
	    if (pTable)
	    {
	        lastTableHandle = pTable->GetHandle();
	        bool mayOffline = false;
	        pTable->OnlyNomalLeaveTable(userId, mayOffline);
	        if (mayOffline)
	            ThrowException(1, "您正在游戏中, 不能换桌");
	    }
	    pUser->activeInfo.lastSitTableHandle = lastTableHandle;
	
	    if (pUser->activeInfo.userState > EUS_YAOYAO)
	    {
	        ThrowException(2, "您正在游戏中, 不能换桌");
	    }
	
	    pUser->activeInfo.jingDu = jingDu;
	    pUser->activeInfo.weiDu = weiDu;
	    pUser->activeInfo.selScore = maxTotalBet;
	    pUser->activeInfo.yaoyaoTick = GetNowMsTick();
	    pUser->activeInfo.isAddRobot = m_pConfigAreaModule->addrobot_rate;
	    // 根据这个值决定允许与机器人玩的时间
	    pUser->activeInfo.addRobotMs = m_pConfigAreaModule->addrobot_sec * MS_PER_SEC;// GetRandomRange(CONFIG_AREA->addrobot_sec * MS_PER_SEC, CONFIG_AREA->realuser_max_waitsec * MS_PER_SEC);
	
	    pUser->activeInfo.userState = EUS_YAOYAO;
	}
	catch (CException& ex)
	{
	    retCode = ex.GetCode();
	    retMsg = ex.GetMsg();
	}
	
	nlohmann::json jsonSend;

	jsonSend["action"] = NFMsg::MSGID_CLIENT_SIT_RESP;
	jsonSend["code"] = retCode;
	jsonSend["msg"] = NFUtil::EncodStringForJson( retMsg);
	jsonSend["userState"] = pUser->activeInfo.userState;
	SendMsgToClient(NFMsg::MSGID_CLIENT_SIT_RESP, jsonSend, clientID);

// 	CPluto* pu = new CPluto;
// 	(*pu).Encode(MSGID_CLIENT_SIT_RESP) << retCode << retMsg << pUser->activeInfo.userState << EndPluto;
// 	mb->PushPluto(pu);

	return 0;
}

int NFCGameLogicModule::OnProcClientReady(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json & jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
	if (pUser)
	{
		int chairIndex = 0;
		NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetPUserTable(*pUser, chairIndex);
		if (pTable)
			return pTable->ProcClientReady(jsonObject, chairIndex);
	}
	return -1;
}

int NFCGameLogicModule::OnProcClienGBet(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json & jsonObject)
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

int NFCGameLogicModule::OnProcClientSeeCard(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json & jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
	if (pUser)
	{
		int chairIndex = 0;
		NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetPUserTable(*pUser, chairIndex);
		if (pTable)
			return pTable->ProcClientSee(jsonObject, chairIndex);
	}

	return -1;
}

int NFCGameLogicModule::OnProcClienGTrust(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json & jsonObject)
{
	NF_SHARE_PTR<SUserInfo> pUser = GetUserInfoByGuid(clientID);
	if (pUser)
	{
		int chairIndex = 0;
		NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetPUserTable(*pUser, chairIndex);
		if (pTable)
			return pTable->ProcClientTrust(jsonObject, chairIndex);
	}

	return -1;
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

void NFCGameLogicModule::GameMsgSenderUnlock(int tableHandle, int userId, int roomId)
{
	if (-1 == tableHandle)
		return;

	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		return;
	}

	NF_SHARE_PTR<CAreaTaskItemBase> task = std::make_shared<CAreaTaskReportScore>(tableHandle, NFMsg::MSGID_DBMGR_UNLOCK_GAME_PLAYER);
	if (!task)
		return;

	m_pQsTaskScheduleModule->AddTask(task);

	GameMsgSenderLockOrUnLock(task->GetTaskId(), roomId, userId, NFGamespace::EG_GOLDEN_FLOWERS, UN_LOCK_USER_LEAVE);

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

	GameMsgSenderLockOrUnLock(task->GetTaskId(), roomId, userId, NFGamespace::EG_GOLDEN_FLOWERS, LOCK_USER_LOGIN);

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


#endif