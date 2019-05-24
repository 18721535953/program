/*----------------------------------------------------------------
// 模块名：robot_mgr
// 模块描述：机器人管理器【配置、分配、释放、读取】。
//----------------------------------------------------------------*/


#include "robot_mgr.h"
#include "NFServer/NFGameServerNet_ServerPlugin/logger.hpp"
#include "type_area.h"
//#include "mailbox.h"
#include "global_var.h"

// 每次读取几个机器人信息
#define READ_ROBOT_USER_COUNT 5

static CRobotMgr* gStaticRobotMgr = NULL;

//////////////////////////////////////////////////////////////////////////
CRobotMgr::CRobotMgr(): m_isInit(false), m_isReading(false), m_readSuccessCount(0), m_allUserId(), m_allRobot(), m_idleRobot(), m_gameRobot()
{
//	InitCfg();
}

CRobotMgr::~CRobotMgr()
{
    m_allUserId.clear();

    //CLEAR_POINTER_MAP(m_allRobot);
    // 3者共用内存
    m_idleRobot.clear();
    m_gameRobot.clear();
}

CRobotMgr* CRobotMgr::Inst()
{
	if (gStaticRobotMgr == NULL)
		gStaticRobotMgr = new CRobotMgr();
	return gStaticRobotMgr;
}

void CRobotMgr::FreeInst()
{
	if (gStaticRobotMgr != NULL)
	{
		delete gStaticRobotMgr;
		gStaticRobotMgr = NULL;
	}
}

// void CRobotMgr::InitCfg()
// {
// 	char buffer[100];
// 	int startId = 0;//atoi(cfg->GetOptValue("robotusers", "startId", "").c_str());
// 	int endId = 10; //m_pConfigAreaModule->; //atoi(cfg->GetOptValue("robotusers", "endId", "").c_str());
// 	//LogInfo("CRobotMgr::InitCfg", "机器人Id startId=%d, endId=%d", startId, endId);
// 	for (int i = startId; i < endId; i++)
// 	{
// 		m_allRobot.insert(make_pair(i, new SUserInfo()));
// 		m_allUserId.push_back(i);
// 	}
// }

void CRobotMgr::CheckStartReadUserInfo()
{
    if (m_isInit)
        return;
    if (m_isReading)
        return;
    if (m_allRobot.size() <= 0)
    {
        m_isInit = true;
        return;
    }

    StartReadRobotUserInfo();
}

void CRobotMgr::ReadUserInfoCallback(int retCode, const char* retMsg, int userIndex, SUserBaseInfo& baseInfo)
{
//     if (!m_isReading)
//     {
//         LogWarning("CRobotMgr::ReadUserInfoCallback", "!m_isReading");
//         return;
//     }
//     
//     if (0 != retCode)
//     {
//         LogWarning("CRobotMgr::ReadUserInfoCallback", "retCode=%d, retMsg=%s", retCode, retMsg);
//         Read1RobotUserInfo(userIndex, true);
//         return;
//     }
// 
//     map<int, SUserInfo*>::iterator iterFind = m_allRobot.find(baseInfo.userId);
//     if (m_allRobot.end() == iterFind)
//     {
//         LogWarning("CRobotMgr::ReadUserInfoCallback", "find User failed: userId=%d", baseInfo.userId);
//         return;
//     }
//     if (iterFind->first == iterFind->second->baseInfo.userId)
//     {
//         LogWarning("CRobotMgr::ReadUserInfoCallback", "user allready read: userId=%d", baseInfo.userId);
//         return;
//     }
//     if (m_allUserId[userIndex] != iterFind->first)
//     {
//         LogWarning("CRobotMgr::ReadUserInfoCallback", "m_allUserId[userIndex] != userId");
//         return;
//     }
// 
//     SUserInfo* pUserInfo = iterFind->second;
//     pUserInfo->baseInfo.CopyFrom(baseInfo);
//     m_readSuccessCount++;
// 
//     if (m_readSuccessCount >= (int)m_allRobot.size())
//     {
//         // init idleRobot
//         for(map<int, SUserInfo*>::iterator iter = m_allRobot.begin(); m_allRobot.end() != iter; ++iter)
//         {
//             m_idleRobot.push_back(iter->second);
//         }
// 
//         m_isReading = false;
//         m_isInit = true;
//     }
//     else
//     {
//         Read1RobotUserInfo(userIndex, false);
//     }
}

bool CRobotMgr::IsRobot(int userId)
{
    return m_allRobot.find(userId) != m_allRobot.end();
}

NF_SHARE_PTR<SUserInfo> CRobotMgr::AllocRobotUser(int selScore)
{
    int len = (int)m_idleRobot.size();
    if (len < 1)
    {
        //LogError("CRobotMgr::AllocRobotUser", "robot User all using");
        return NULL;
    }

    int index = rand() % len;
    // 为了用erase
    vector<NF_SHARE_PTR<SUserInfo>>::iterator iter = m_idleRobot.begin() + index;
	NF_SHARE_PTR<SUserInfo> ret = *iter;
    m_gameRobot.insert(make_pair(ret->baseInfo.userId, ret));
    m_idleRobot.erase(iter);

    // 随机bean
    {
        SConfigScoreItem* pScoreItem = CONFIG_AREA->score_list.FindItem(selScore);
        if(pScoreItem)
        {
            ret->baseInfo.bean = pScoreItem->robotBean.GetRandom();
        }
        else
        {
            ret->baseInfo.bean += 200;
            LogError("CRobotMgr::AllocRobotUser", "find score=%d ConfigScoreItem failed!", selScore);
        }
    }
    ret->activeInfo.robotReadyMs = NFUtil::GetRandomRange(CONFIG_AREA->robot_cfg.ready_min_sec*MS_PER_SEC, CONFIG_AREA->robot_cfg.ready_max_sec*MS_PER_SEC);
    ret->activeInfo.selScore = selScore;

    //LogInfo("CRobotMgr::AllocRobotUser", "userId=%d idleCount=%d gameCount=%d", ret->baseInfo.userId, m_idleRobot.size(), m_gameRobot.size());

    return ret;
}

bool CRobotMgr::FreeRobotUser(int userId)
{
//     map<int, SUserInfo*>::iterator iter = m_gameRobot.find(userId);
//     if(m_gameRobot.end() == iter)
//     {
//         LogError("CRobotMgr::FreeRobotUser", "find user failed. userId=%d", userId);
//         return false;
//     }
// 
//     m_idleRobot.push_back(iter->second);
//     m_gameRobot.erase(iter);
// 
//     //LogInfo("CRobotMgr::FreeRobotUser", "userId=%d idleCount=%d gameCount=%d", userId, m_idleRobot.size(), m_gameRobot.size());
    return true;
}

void CRobotMgr::Read1RobotUserInfo(int lastIndex, bool reRead)
{
//     if (m_isInit)
//         return;
// 
// 	CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
// 	if (mbDbmgr)
// 	{
// 		// 找到一个未读用户信息的机器人
// 		int curIndex = lastIndex;
// 		if (!reRead)
// 			curIndex = lastIndex + READ_ROBOT_USER_COUNT;
// 
// 		if (curIndex >= (int)m_allUserId.size())
// 		{
// 			LogInfo("CRobotMgr::Read1RobotUserInfo", "end read: UserIndex=%d", lastIndex);
// 			return;
// 		}
// 		int userId = m_allUserId[curIndex];
// 		if (m_pNFIGameServerToDBModule)
// 		{
// 			short dataLen = tmpStr.size();
// 			m_pNFIGameServerToDBModule->SendReadUserInfo(taskId, gameRoomId, dataLen, tmpStr.c_str());
// 
// 		}
// 		else
// 		{
// 			m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
// 		}
// 		//创建任务
// 		CAreaTaskReadUserInfo* task = new CAreaTaskReadUserInfo(MSGID_ROBOT_READ_USERINFO, curIndex);
// 		WORLD_GAME_AREA->AddTask(task);
// 
// 		//LogInfo("CRobotMgr::Read1RobotUserInfo", "userId=%d", userId);
// 
// 		// 为了延续task，即使未连接，也要创建task
// 		if (!mbDbmgr->IsConnected())
// 		{
// 			LogWarning("CRobotMgr::Read1RobotUserInfo", "!mbDbmgr->IsConnected()");
// 		}
// 		else
// 		{
// 			int32_t gameLock = 0;
// 			int32_t gameRoomId = CONFIG_AREA->gameRoomId;
// 			ProcUserInfoToDb(task->GetTaskId(), userId, "", gameRoomId, gameLock, curIndex);
// 			//CPluto* pu = new CPluto();
// 			//(*pu).Encode(MSGID_DBMGR_READ_USERINFO) << task->GetTaskId() << userId << "" << gameLock << gameRoomId << EndPluto;
// 		   // mbDbmgr->PushPluto(pu);
// 		}
// 	}
// 	else
// 	{
// 		LogError("CRobotMgr::Read1RobotUserInfo", "!mbDbmgr");
// 	}
}

void CRobotMgr::StartReadRobotUserInfo()
{
// 	CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
//     if(mbDbmgr)
//     {
//         if(!mbDbmgr->IsConnected())
//         {
//             LogWarning("CRobotMgr::StartReadRobotUserInfo", "!mbDbmgr->IsConnected()");
//             return;
//         }
//         else
//         {
//             for(int i = 0; i < READ_ROBOT_USER_COUNT; ++i)
//             {
//                 if (i >= (int)m_allUserId.size())
//                     break;
// 
//                 int userId = m_allUserId[i];
//                 //创建任务
//                 CAreaTaskReadUserInfo* task = new CAreaTaskReadUserInfo(MSGID_ROBOT_READ_USERINFO, i);
//                 WORLD_GAME_AREA->AddTask(task);
// 
//                 //LogInfo("CRobotMgr::StartReadRobotUserInfo", "userId=%d", userId);
// 
//                 int32_t gameLock = 0;
//                 int32_t gameRoomId = CONFIG_AREA->gameRoomId;
// 
// 				ProcUserInfoToDb(task->GetTaskId(), userId, "", gameRoomId, gameLock, i);
//                 //CPluto* pu = new CPluto();
//                 //(*pu).Encode(MSGID_DBMGR_READ_USERINFO) << task->GetTaskId() << userId << "" << gameLock << gameRoomId << EndPluto;
//                 //mbDbmgr->PushPluto(pu);
//             }
// 
//             m_isReading = true;
//         }
//     }
//     else
//     {
//         LogError("CRobotMgr::StartReadRobotUserInfo", "!mbDbmgr");
//     }
}


int CRobotMgr::ProcUserInfoToDb(uint32_t taskId, int32_t userId, string accessToken, int32_t gameRoomId, int32_t gameLock, int srcFd)
{
// 	int32_t retCode = 0;
// 	string retMsg = "";
// 	SUserBaseInfo userInfo;
// 	int index = 0;
// 	string jsStr;
// 	const char* pszAccessToken = accessToken.c_str();
// 	try
// 	{
// 		//如果有accessToken，验证下
// 		string accessToken(pszAccessToken);
// 		string aes_key = CONFIG_AREA->m_aes_key;
// 		if (accessToken.size() > 0)
// 		{
// 			string decryptToken;
// 			AesDecryptStr(accessToken, aes_key, decryptToken);
// 
// 			vector<string> spl;
// 			SplitStringToVector(decryptToken, '^', spl);
// 			if (spl.size() < 3)
// 				ThrowException(1, "验证失败-参数少");
// 
// 			string data = "";
// 			if (spl.size() > 3)
// 				data = spl[3];
// 
// 			string md5;
// 			GetStrMd5(spl[0] + spl[1] + data + aes_key, md5);
// 			data = md5;
// 			GetStrMd5(data, md5);
// 			if (spl[2] != md5)
// 				ThrowException(2, "验证失败-md5 不对");
// 
// 			userId = atoi(spl[0].c_str());
// 		}
// 		if (userId <= 0)
// 			ThrowException(2, "验证失败-用户没有id");
// 
// 		uint64_t stamp = GetTimeStampInt64Ms();
// 		char buffer[150];
// 		const int C_GAME_ID = 8; // 游戏id
// 		snprintf(buffer, sizeof(buffer), "%d", C_GAME_ID);
// 		string strGameId(buffer);
// 		string m_cis_key = "goisnfgsf34-9f25";
// 		// 获得校验值
// 		snprintf(buffer, sizeof(buffer), "%d%s%d%d%llu%s", userId, strGameId.c_str(), gameLock, gameRoomId, stamp, m_cis_key.c_str());
// 		string tmpStr = buffer;
// 		string md5;
// 		GetStrMd5(tmpStr, md5);
// 		// postdata
// 		snprintf(buffer, sizeof(buffer), "action=GetUserInfos&userId=%d&gameId=%s&gameLock=%d&gameRoomId=%d&timestamp=%llu&checkCode=%s", userId, strGameId.c_str(), gameLock, gameRoomId, stamp, md5.c_str());
// 		tmpStr = buffer;
// 		CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
// 		if (mbDbmgr)
// 		{
// 			if (!mbDbmgr->IsConnected())
// 			{
// 				LogWarning(" cRobotMgr::ProcUserInfoToDb", "!mbDbmgr->IsConnected()");
// 				//直接登录失败
// 				//ClientUpdateUserInfoResponse(srcFd, 100, "服务器维护中");
// 			}
// 			else
// 			{
// 				LogInfo("CWorldGameArea::ProcUserInfoToDb", "创建发送任务:taskid:%d ,json:%s ", taskId, tmpStr.c_str());
// 				CPluto* pu = new CPluto();
// 				(*pu).Encode(MSGID_DBMGR_READ_USERINFO_TODB) << taskId << CONFIG_AREA->gameRoomId << tmpStr << EndPluto;
// 				mbDbmgr->PushPluto(pu);
// 			}
// 		}
// 		else
// 		{
// 			LogError(" cRobotMgr::ProcUserInfoToDb-robot", "!mbDbmgr");
// 		}
// 	}
// 	catch (CException & ex)
// 	{
// 		retCode = ex.GetCode();
// 		retMsg = ex.GetMsg();
// 
// 		LogError(" cRobotMgr::ProcUserInfoToDb-robot", "code=%d, error: %s cisRet=%s", ex.GetCode(), ex.GetMsg().c_str(), jsStr.c_str());
// 	}


	return 0;
}

//MSGID_AREA_READ_USERINFO_CALLBACK
