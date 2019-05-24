#ifndef __NFC_TABLE_MANAGER_MODULE_CPP__
#define __NFC_TABLE_MANAGER_MODULE_CPP__

/*----------------------------------------------------------------
// 模块名：table_mgr
// 模块描述：桌子（包含机器人处理）桌子列表【包含分桌处理、断线用户列表（可以返回）、激活桌子定时器】
//----------------------------------------------------------------*/

#include <limits.h>
#include "NFCTableManagerModule.h"
#include "type_area.h"
#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/logger.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/exception.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/type_mogo_def.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "type_mogo.h"
#include "global_var.h"
int const LOCK_USER_LOGIN = 1;			 //用户登录游戏时加锁
int const UN_LOCK_USER_LEAVE = -1;       //用户退出游戏时解锁

#define CTimerInterval 200

NFGameTable::NFGameTable(int handle, NFITableManagerModule * p) : m_handle(handle), m_isActive(false), m_hasStartGame(false), m_endGameTick(0), m_lastRunTick(), m_userList(), m_sitUserList(),
m_curRound(0), m_createUserId(INVALID_USERID), m_forceLeaveSec(INVALID_TIME_COUNT), m_clearRemainSec(INVALID_TIME_COUNT), m_minSpecialGold(0), m_vipRoomType(vrtNone),
m_totalScore(), m_robotWinUsers()
{
	m_pTableManagerModule = p;
	this->m_handle = handle;

	Init();
	AfterInit();
}

NFGameTable::~NFGameTable()
{
	m_sitUserList.clear();
	//CLEAR_POINTER_CONTAINER(m_userList);
}

void NFGameTable::Init()
{
	if (m_pTableManagerModule->GetPluginManager() == nullptr)
		return;
	for (int i = 0; i < MAX_TABLE_USER_COUNT; i++)
	{
		//m_userList.push_back(new CTableUser());
		m_userList.push_back(std::make_shared<CTableUser>());
	}
	m_pNFIGameServerToDBModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIGameServerToDBModule>();

	m_pLogModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFILogModule>();
	m_pRobotMoudle = m_pTableManagerModule->GetPluginManager()->FindModule<NFIRobotModule>();
	m_pQsTaskScheduleModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIQsTaskScheduleModule>();
	m_pNetClientModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFINetClientModule>();
	m_pGameLogicModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIGameLogicModule>();
	m_pGameServerNet_ServerModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIGameServerNet_ServerModule>();
	m_pConfigAreaModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIConfigAreaModule>();
	//m_pControlCardModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIControlCardModule>();
}

void NFGameTable::AfterInit()
{

	RoundStopClearData();
	NoUserClearData();
	
}

int NFGameTable::ProcClientChat(const nlohmann::json& p, int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
		return -1;

	int index = 0;

	NFGamespace::ClientChat xChat;
	try
	{
		xChat = p;
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
	}

	if (xChat.chatMsg == -1)
	{
		m_pLogModule->LogError("chatMsg empty", __FUNCTION__, __LINE__);
		return -1;
	}

	NF_SHARE_PTR<CTableUser> pTUser =  m_userList[chairIndex];
	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_CHAT_RESP;
		jsonSend["code"] = 0;
		jsonSend["msg"] = "";
		jsonSend["chatMsg"] = xChat.chatMsg;

		m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_CHAT_RESP, jsonSend, pTUser->userInfo.self);
	}

	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_OTHER_CHAT_NOTIFY;
		jsonSend["userId"] = pTUser->userInfo.baseInfo.userId;
		jsonSend["userName"] = pTUser->userInfo.baseInfo.userName;
		jsonSend["chatCode"] = xChat.chatType;
		jsonSend["chatMsg"] = std::to_string(xChat.chatMsg);

		int userId = pTUser->userInfo.baseInfo.userId;
		//m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::MSGID_CLIENT_OTHER_CHAT_NOTIFY, jsonSend, userId, tusWaitNextRound, m_sitUserList);
	}
}

int NFGameTable::ProcClientReady(const nlohmann::json& p, int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
		return -1;
	int index = 0; 
	int32_t i64Param = 0;
	try
	{
		i64Param = p.at("i64param").get<int>();
	}
	catch (const nlohmann::detail::exception& ex)
	{
		return -1;
	}
	//int64_t i64Param = (*p)[index++]->vv.i64;

	NF_SHARE_PTR<CTableUser> pTUser = m_userList[chairIndex];
	int code = 0;
	string retMsg = "";
	try
	{
		if (IsGaming())
			ThrowException(1, "游戏已经开始");
		if (pTUser->isReady)
			ThrowException(2, "您已经准备");
		if (m_curRound <= 0)
			ThrowException(ERROR_CODE_TO_MAX_ROUND, "对不起，创建的牌局已经结束");
	}
	catch (CException& ex)
	{
		code = ex.GetCode();
		retMsg = ex.GetMsg();
	}

	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_READY_RESP;
		jsonSend["code"] = code;
		jsonSend["msg"] = NFUtil::EncodStringForJson(retMsg);
		SendPlutoToUser(NFMsg::MSGID_CLIENT_READY_RESP, jsonSend, pTUser->userInfo.self);

		//CPluto* puResp = new CPluto();
		//(*puResp).Encode(MSGID_CLIENT_READY_RESP) << code << retMsg << EndPluto;
		//SendPlutoToUser(puResp, pTUser);
	}

	if (0 == code)
	{
		DoUserReady(pTUser);
	}

	return 0;
}

int NFGameTable::ProcClientTrust(const nlohmann::json& p, int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
		return -1;
	int index = 0; 
	int32_t isTrust = 0;
	try
	{
		isTrust = p.at("isTrust").get<int>();
	}
	catch(const nlohmann::detail::exception& ex)
	{
		return -1;
	}
	NF_SHARE_PTR<CTableUser> pTUser = m_userList[chairIndex];
	int code = 0;
	string retMsg = "";
	try
	{
		if (!IsGaming())
		{
			ThrowException(1, "游戏还没有开始");
		}
		if (0 != isTrust && 1 != isTrust)
		{
			ThrowException(2, "参数错误");
		}
		if (pTUser->isTrust == isTrust)
		{
			ThrowException(3, "参数错误");
		}
	}
	catch (CException& ex)
	{
		code = ex.GetCode();
		retMsg = ex.GetMsg();
	}

	if (0 == code)
	{
		DoUserTrust(pTUser, isTrust, pTUser->userInfo.baseInfo.userId);
		pTUser->manualTrust = isTrust;
	}

	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_G_TRUST_RESP;
		jsonSend["code"] = code;
		jsonSend["msg"] = "";
		SendPlutoToUser(NFMsg::MSGID_CLIENT_G_TRUST_RESP, jsonSend, pTUser->userInfo.self);
		// 		// 返回包 发送当前isTrust状态
		// 		CPluto* puResp = new CPluto();
		// 		(*puResp).Encode(MSGID_CLIENT_G_TRUST_RESP) << code << retMsg << pTUser->isTrust << EndPluto;
		// 		SendPlutoToUser(puResp, pTUser);
	}

	return 0;
}

int NFGameTable::ProcClientBet(const nlohmann::json& p, int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
		return -1;
	int index = 0;


	int32_t betType = 0;
	int32_t hideBetValue = 0;
	int32_t vsUserId = 0;

	try
	{
		betType = p.at("betType").get<int>();
		hideBetValue = p.at("hideBetValue").get<int>();
		vsUserId = p.at("vsUserId").get<int>();
	}
	catch (const nlohmann::detail::exception& ex)
	{
		return -1;
	}

	NF_SHARE_PTR<CTableUser> pTUser = m_userList[chairIndex];
	if (!pTUser)
	{
		LogError("NFGameTable::ProcClientBet", "pTuser is null");
		return -1;
	}

	int code = 0;
	string retMsg = "";
	try
	{
		if (tbsBet != m_tstate)
		{
			ThrowException(1, "当前不是下注状态");
		}
		if (betType != sbtFold)
		{
			if (m_curUserId != pTUser->userInfo.baseInfo.userId)
			{
				ThrowException(2, "没有轮到您操作");
			}
		}
		if (!pTUser->CanBet())
			ThrowException(2, "状态错误");

		switch (betType)
		{
		case sbtFold:
		{
			// none
			break;
		}
		case sbtVsCard:
		{
			if (m_isInPin)
				ThrowException(10, "血拼状态不允许比牌");
			if (!CanVs())
				ThrowException(11, "还没到比牌的轮数");
			// 不看也能比。
			if (vsUserId == pTUser->userInfo.baseInfo.userId)
				ThrowException(12, "不能和自己比");
			NF_SHARE_PTR<CTableUser> pTVsWho = FindUserById(vsUserId);
			if (!pTVsWho)
				ThrowException(12, "玩家不存在");
			if (!pTVsWho->CanBet())
				ThrowException(12, "玩家不在游戏中");
			if (m_betRound < m_tableRule.pkRound + 1)
			{
				ThrowException(14, "还没到比牌轮数");
			}

			break;
		}
		case sbtCall:
		{
			if (m_isInPin)
				ThrowException(10, "血拼状态不允许跟");
			if (m_hideMinBet + pTUser->totalBet > pTUser->userInfo.baseInfo.bean)
				ThrowException(16, "玩家现有额度不足");
			// none
			break;
		}
		case sbtRaise:
		{
			if (m_isInPin)
				ThrowException(10, "血拼状态不允许加");

			bool bFind = false;
			for (vector<int>::iterator it = m_betRule.raiseBet.begin(); it != m_betRule.raiseBet.end(); ++it)
			{
				if (hideBetValue == *it)
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)
				ThrowException(11, "下注额度错误");
			if (hideBetValue <= m_hideMinBet)
				ThrowException(12, "加注要大于跟的额度");
			if (hideBetValue + pTUser->totalBet > pTUser->userInfo.baseInfo.bean)
				ThrowException(15, "加注小于玩家现有额度");
			break;
		}
		case sbtPin:
		{
			if (!m_hasPin)
				ThrowException(10, "规则不允许血拼");
			if (!CanPin())
				ThrowException(11, "还没到血拼的轮数");

			break;
		}
		case sbtSeeCard:
		{
			if (pTUser->bSeeCard)
				ThrowException(10, "您已经看牌了");

			// 看牌独立了数据包
			ThrowException(10, "不支持");
			break;
		}
		default:
		{
			ThrowException(10, "参数错误");
			break;
		}
		}
	}
	catch (CException ex)
	{
		code = ex.GetCode();
		retMsg = ex.GetMsg();
	}
	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_G_BET_RESP;
		jsonSend["code"] = code;
		jsonSend["msg"] = "";
	
		// 		// 返回包
		// 		CPluto* puResp = new CPluto();
		// 		(*puResp).Encode(MSGID_CLIENT_G_BET_RESP) << code << retMsg;
		 		if (0 == code && sbtSeeCard == betType)
		 		{
		 			pTUser->player.cardList.WriteToPluto(jsonSend);
		 		}
		 		else
		 		{
					jsonSend["cards"] = 0;
		 			//(*puResp) << (uint16_t)0;
		 		}
		// 		(*puResp) << EndPluto;
		// 		SendPlutoToUser(puResp, pTUser);
		SendPlutoToUser(NFMsg::MSGID_CLIENT_G_BET_RESP, jsonSend, pTUser->userInfo.self);
	}
	if (0 == code)
	{
		DoUserBet(pTUser, betType, hideBetValue, vsUserId);
	}

	return 0;
}

int NFGameTable::ProcClientSee(const nlohmann::json& p, int chairIndex)
{
	NF_SHARE_PTR<CTableUser> pTUser = m_userList[chairIndex];

	int code = 0;
	string retMsg = "";
	try
	{
		if (tbsBet != m_tstate)
		{
			ThrowException(1, "当前不是下注状态");
		}
		if (!pTUser->CanBet())
			ThrowException(2, "状态错误");

		if (pTUser->bSeeCard == 1)
		{
			ThrowException(10, "您已经看牌了");
		}
		if (m_tableRule.menPiRound > 1)			//选择闷牌
		{
			if (m_betRound < m_tableRule.menPiRound + 1)
			{
				ThrowException(3, "没达到闷牌轮数，您只能下注或弃牌");
			}
		}

	}
	catch (CException& ex)
	{
		code = ex.GetCode();
		retMsg = ex.GetMsg();
	}

	// 	CPluto* puResp = new CPluto();
	// 	(*puResp).Encode(MSGID_CLIENT_SEE_CATD_RESP) << code << retMsg << pTUser->player.cardList.m_cardType.TypeNum;
	// 	if (0 == code)
	// 	{
	// 		pTUser->player.cardList.WriteToPluto(*puResp);
	// 	}
	// 	else
	// 	{
	// 		(*puResp) << (uint16_t)0;
	// 	}
	// 	(*puResp) << EndPluto;
	// 	SendPlutoToUser(puResp, pTUser);

	if (0 == code)
	{
		DoUserSeeCard(pTUser);
	}

	return 0;
}


void NFGameTable::OnReportScoreError(int code, const char* errorMsg)
{
	if (tbsCalcResult != m_tstate)
	{
		LogError("NFGameTable::OnReportScoreError", "table state error");
		return;
	}

	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		// 记录结果日志
		SInfoLogItem *item = m_matchLog.infoLog[pTUser->userInfo.baseInfo.userId];
		item->userId = pTUser->userInfo.baseInfo.userId;
		item->channelId = pTUser->userInfo.baseInfo.channelId;
		item->username = pTUser->userInfo.baseInfo.userName;
		item->nickname = pTUser->userInfo.baseInfo.nickName;
		item->chairId = pTUser->userInfo.activeInfo.chairIndex;
		item->startBean = pTUser->userInfo.baseInfo.bean;
		item->endBean = pTUser->userInfo.baseInfo.bean + pTUser->roundIncBean;
		item->totalBet = pTUser->totalBet;
		item->SysRect = pTUser->roundRevenue;
		if (pTUser->isRobot)
		{
			item->startBean = 0;
			item->endBean = 0;
		}
		//uint64_t stamp = GetTimeStampInt64Sec();
		uint64_t stamp = time(NULL);
		m_matchLog.endTime = stamp;

		// 记录玩家日志
		pTUser->leaveBean = pTUser->userInfo.baseInfo.bean + pTUser->roundIncBean;
		pTUser->leaveTime = GetTimeStampInt64Ms();


		// 同步内存信息
		pTUser->userInfo.baseInfo.bean += pTUser->roundIncBean;
		NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByUserId(pTUser->userInfo.baseInfo.userId);
		if (pUser)
		{
			pUser->baseInfo.bean += pTUser->roundIncBean;
			UnlockOrlockUser(pTUser, UN_LOCK_USER_LEAVE);
			SendForceLeaveNotify(pTUser, 1, "结算异常");

		}

	}

	LogWarning("NFGameTable::OnReportScoreError", "code=%d errorMsg=%s", code, errorMsg);

	StartReportMatchLog();
	m_endGameTick = GetNowMsTick();
	SendGameResultNotify();
}

void NFGameTable::OnReportScoreSuccess(const nlohmann::json& p, int index)
{
	if (tbsCalcResult != m_tstate)
	{
		LogError("NFGameTable::OnReportScoreSuccess", "table state error");
		return;
	}

	SCisScoreReportRetItem tmpReportItem;
	//T_VECTOR_OBJECT* pAry = (*p)[index++]->vv.oOrAry;
	//uint16_t len = (uint16_t)pAry->size();
	//for (uint16_t i = 0; i < len; ++i)
	nlohmann::json pAryJson = p[""].get<nlohmann::json>();
	for (nlohmann::json pItem : pAryJson)
	{
		//T_VECTOR_OBJECT* pItem = (*pAry)[i]->vv.oOrAry;
		int indexItem = 0;
		// 		tmpReportItem.ReadFromVObj(*pItem, indexItem);
		// 
		// 		NF_SHARE_PTR<CTableUser> pTUser = FindUserById(tmpReportItem.userId);
		// 		if (!pTUser)
		// 		{
		// 			LogError("CGameTable::OnReportScoreSuccess", "find userId=%d failed", tmpReportItem.userId);
		// 		}
		// 		else
		// 		{
		// 			// 记录结果日志
		// 			SInfoLogItem *item = m_matchLog.infoLog[pTUser->userInfo.baseInfo.userId];
		// 			item->userId = pTUser->userInfo.baseInfo.userId;
		// 			item->channelId = pTUser->userInfo.baseInfo.channelId;
		// 			item->username = pTUser->userInfo.baseInfo.userName;
		// 			item->nickname = pTUser->userInfo.baseInfo.nickName;
		// 			item->chairId = pTUser->userInfo.activeInfo.chairIndex;
		// 			item->startBean = pTUser->userInfo.baseInfo.bean;
		// 			item->endBean = tmpReportItem.bean;
		// 			item->totalBet = pTUser->totalBet;
		// 			item->SysRect = pTUser->roundRevenue;
		// 			if (pTUser->isRobot)
		// 			{
		// 				item->startBean = 0;
		// 				item->endBean = 0;
		// 			}
		// 			//LogInfo("CGameTable::OnReportScoreSuccess", "pushbacktimes=%d, userId=%d", i, pTUser->userInfo.baseInfo.userId);
		// 			uint64_t stamp = GetTimeStampInt64Sec();
		// 			m_matchLog.endTime = stamp;
		// 
		// 			// 记录玩家日志
		// 			pTUser->leaveBean = tmpReportItem.bean;
		// 			pTUser->leaveTime = GetTimeStampInt64Ms();
		// 
		// 			pTUser->userInfo.baseInfo.score = tmpReportItem.score;
		// 			// 机器人自己计算输赢
		// 			if (!pTUser->isRobot)
		// 				pTUser->userInfo.baseInfo.bean += pTUser->roundIncBean;
		// 			else
		// 				pTUser->userInfo.baseInfo.bean += tmpReportItem.incBean;
		// 			pTUser->userInfo.baseInfo.level = tmpReportItem.level;
		// 			//pTUser->userInfo.baseInfo.specialGold = tmpReportItem.specialGold;
		// 
		// 			// 同步内存信息
		// // 			SUserInfo* pUser = WORLD_GAME_AREA->FindUserById(tmpReportItem.userId);
		// // 			if (pUser)
		// // 			{
		// // 				pUser->baseInfo.score = tmpReportItem.score;
		// // 				// 机器人自己计算输赢
		// // 				if (!pTUser->isRobot)
		// // 					pUser->baseInfo.bean = tmpReportItem.bean;
		// // 				else
		// // 					pUser->baseInfo.bean += tmpReportItem.incBean;
		// // 				pUser->baseInfo.level = tmpReportItem.level;
		// // 				//pUser->baseInfo.specialGold = tmpReportItem.specialGold;
		// // 			}
		// 		}
	}

	StartReportMatchLog();
	m_endGameTick = GetNowMsTick();
	SendGameResultNotify();
}

void NFGameTable::OnReportConsumeSpecialGoldSuccess(T_VECTOR_OBJECT* p, int index, bool isPin)
{
	T_VECTOR_OBJECT* pAry = (*p)[index++]->vv.oOrAry;
	uint16_t len = (uint16_t)pAry->size();
	for (uint16_t i = 0; i < len; ++i)
	{
		T_VECTOR_OBJECT* pItem = (*pAry)[i]->vv.oOrAry;
		int indexItem = 0;
		SCisSpecialGoldComsumeRetItem tmpConsumeItem;
		tmpConsumeItem.ReadFromVObj(*pItem, indexItem);

		NF_SHARE_PTR<CTableUser> pTUser = FindUserById(tmpConsumeItem.userId);
		if (!pTUser)
		{
			LogError("CGameTable::OnReportConsumeSpecialGoldSuccess", "find userId=%d failed", tmpConsumeItem.userId);
		}
		else
		{
			pTUser->userInfo.baseInfo.specialGold = tmpConsumeItem.specialGold;
			// 同步内存信息
// 			SUserInfo* pUser = WORLD_GAME_AREA->FindUserById(tmpConsumeItem.userId);
// 			int64_t incSpecialGold = 0;
// 			if (pUser)
// 			{
// 				incSpecialGold = tmpConsumeItem.specialGold - pUser->baseInfo.specialGold;
// 				pUser->baseInfo.specialGold = tmpConsumeItem.specialGold;
// 			}
// 
// 			CPluto* pu = new CPluto();
// 			(*pu).Encode(MSGID_CLIENT_G_CONSUME_SPECIAL_GOLD_NOTIFY)
// 				<< tmpConsumeItem.userId << incSpecialGold << tmpConsumeItem.specialGold << (int32_t)isPin << EndPluto;
// 			//SendPlutoToUser(pu, pTUser);
// 			SendBroadTablePluto(pu, -1);
			/*LogInfo("CGameTable::OnReportConsumeSpecialGoldSuccess",
				"userId=%d, isPin=%d incSpecialGold = %lld", tmpConsumeItem.userId, (int)isPin, incSpecialGold);*/
		}
	}
}

bool NFGameTable::CheckRunTime()
{
	if (m_isActive)
	{
		if (m_lastRunTick.GetTimePassMillionSecond() >= 1000)
		{
			//m_lastRunTick.AddTableMsTick(1000);
			RobotAction();
			RunTime();
		}

		return true;
	}
	else
	{
		return false;
	}
}

void NFGameTable::SetNotActive()
{
	if (m_isActive)
	{
		m_isActive = false;
		m_hasStartGame = false;
		RoundStopClearData();
		NoUserClearData();

		//LogInfo("CGameTable::SetNotActive", "NoUserClearData handle=%d", m_handle);
	}
}

bool NFGameTable::EnterTable(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex, bool isRobot)
{
	NF_SHARE_PTR<CTableUser> pTOldUser = FindUserById(pUser->baseInfo.userId);
	NF_SHARE_PTR<CTableUser> pTSitUser = FindUserByChairIndex(chairIndex);
	if (!pTSitUser)
	{
		LogError("CGameTable::EnterTable", "cannot find chairIndex");
		return false;
	}

	// 真人可以替换机器人，机器人不能替换真人。
	if (!isRobot && !IsGaming())
	{
		if (tusNone != pTSitUser->ustate && pTSitUser->isRobot)
		{
			LogInfo("CGameTable::EnterTable", "replace robot realUser=%d robot=%d", pUser->baseInfo.userId, pTSitUser->userInfo.baseInfo.userId);
			ClearUser(pTSitUser);
		}
	}

	if (pTOldUser)
	{
		// 判断是否可以返回
		/*if(pTOldUser->ustate <= tusNomal)
		{
		LogError("CGameTable::EnterTable", "user exists userId=%d", pTOldUser->userInfo.baseInfo.userId);
		return false;
		}*/
		if (pTOldUser != pTSitUser)
		{
			LogError("CGameTable::EnterTable", "offline return chairIndex error");
			return false;
		}

		SetIsActive();
		LogInfo("CGameTable::EnterTable", "offline return success userId= %d", pUser->baseInfo.userId);
		AddUser(pUser, chairIndex);
		pTOldUser->ustate = tusNomal;
		// selScore: 断线返回需要以旧的为准
		int selScore = pTOldUser->userInfo.activeInfo.selScore;
		pTOldUser->userInfo.CopyFrom(*pUser);
		pTOldUser->userInfo.activeInfo.selScore = selScore;

		// 通知他人用户状态变化
		SendUserStateNotify(pUser->baseInfo.userId, pTOldUser->ustate, pUser->baseInfo.bean);
		UpdateCurBaseScore();
		return true;
	}
	else
	{
		if (pTSitUser->ustate != tusNone)
		{
			LogError("CGameTable::EnterTable", "chairIndex=%d has a user", chairIndex);
			return false;
		}

		if (IsGaming())
		{
			if (!GAME_CAN_ENTER_WHEN_GAMING)
			{
				LogError("CGameTable::EnterTable", "table is gaming handle=%d", m_handle);
				return false;
			}

			if (GAME_CAN_DIRECT_GAME)
				pTSitUser->ustate = tusNomal;
			else
				pTSitUser->ustate = tusWaitNextRound;
		}
		else
		{
			pTSitUser->ustate = tusNomal;
		}

		SetIsActive();
		LogInfo("CGameTable::EnterTable", "sit success userId= %d handle=%d chairIndex=%d", pUser->baseInfo.userId, m_handle, chairIndex);
		AddUser(pUser, chairIndex);
		pTSitUser->userInfo.CopyFrom(*pUser);
		pTSitUser->isRobot = isRobot;
		pTSitUser->userInfo.activeInfo.enterTableTick = GetNowMsTick();

		if (pTSitUser->isRobot)
		{
			pTSitUser->userInfo.activeInfo.ip = "127.0.0.1";
		}

		if (!isRobot)
		{
			//NF_SHARE_PTR<CTableUser> pTSitUser = FindUserByChairIndex(chairIndex);
			LogInfo("用户进入桌子, 给用户上锁", "userid:%d,", pTSitUser->userInfo.baseInfo.userId);
			UnlockOrlockUser(pTSitUser, LOCK_USER_LOGIN);
		}

		// 记录进入桌子时的金币(断线重连则不更新)
		if (pTSitUser->enterBean == 0)
		{
			pTSitUser->enterBean = pTSitUser->userInfo.baseInfo.bean;
			pTSitUser->enterTime = GetTimeStampInt64Ms();
		}

		// 通知其他人有人入座
 // 		CPluto* pu = new CPluto();
 // 		(*pu).Encode(MSGID_CLIENT_OTHER_ENTER_NOTIFY) << chairIndex
 // 			<< pTSitUser->isReady << pTSitUser->totalBet << pTSitUser->cardState << pTSitUser->bSeeCard
 // 			<< pTSitUser->agreeEnd << pTSitUser->ustate << GetUserTotalScore(pTSitUser->userInfo.baseInfo.userId);
		nlohmann::json jsonObject;

		jsonObject["action"] = NFMsg::MSGID_CLIENT_OTHER_ENTER_NOTIFY;
		
	
		jsonObject["chairIndex"] = chairIndex;
		jsonObject["isReady"] = pTSitUser->isReady;
		jsonObject["totalBet"] = pTSitUser->totalBet;
		jsonObject["cardState"] = pTSitUser->cardState;
		jsonObject["bSeeCard"] = pTSitUser->bSeeCard;
		jsonObject["agreeEnd"] = pTSitUser->agreeEnd;
		jsonObject["tuserState"] = pTSitUser->ustate;
		jsonObject["ZScore"] = GetUserTotalScore(pTSitUser->userInfo.baseInfo.userId);
  		pTSitUser->userInfo.baseInfo.WriteToPluto(jsonObject);
 // 		(*pu) << pTSitUser->userInfo.activeInfo.ip;
 // 		(*pu) << EndPluto;
		jsonObject["ip"] = pTSitUser->userInfo.activeInfo.ip;
  		SendBroadTablePluto(NFMsg::MSGID_CLIENT_OTHER_ENTER_NOTIFY, jsonObject, pUser->baseInfo.userId);
 // 		UpdateCurBaseScore();

		return true;
	}
}

bool NFGameTable::LeaveTable(int userId)
{
	bool isOffline = false;
	NF_SHARE_PTR<CTableUser> pTUser = OnlyNomalLeaveTable(userId, isOffline);

	if (isOffline)
	{
		pTUser->ustate = tusOffline;
		pTUser->offlineTick = GetNowMsTick();
		// 通知用户状态改变
		SendUserStateNotify(userId, pTUser->ustate, pTUser->userInfo.baseInfo.bean);
	}

	return true;
}

NF_SHARE_PTR<CTableUser> NFGameTable::OnlyNomalLeaveTable(int userId, bool& mayOffline)
{
	mayOffline = false;
	//NF_SHARE_PTR<SUserInfo> pTUser = m_pGameLogicModule->GetUserInfoByUserId(userId);
	NF_SHARE_PTR<CTableUser> pTUser = FindUserById(userId);
	if (!pTUser)
	{
		LogError("CGameTable::OnlyNomalLeaveTable", "not find user");
		return NULL;
	}
	if (pTUser->ustate > tusNomal)
	{
		LogError("CGameTable::OnlyNomalLeaveTable", "user state not normal");
		return NULL;
	}

	if (IsGaming())
	{
		if (!CanLeaveWhenGaming(pTUser))
		{
			mayOffline = true;
		}
	}
	if (pTUser && !pTUser->isRobot)
	{
		//用户离线/关进程,但是 桌子不在游戏中,那么发解锁包
		if ((pTUser->ustate < tusNomal) || (!IsGaming()))
		{
			if (!pTUser->isAbnormal)
			{
				LogInfo("不在游戏中玩家离线, 解锁用户!", "userid:%d,", userId);
				UnlockOrlockUser(pTUser, UN_LOCK_USER_LEAVE);
			}
		}
	}
	if (!mayOffline)
	{
		// 用户正常退出，发送统计日志
		if (!pTUser->isRobot)
			StartReportUserLog(pTUser);
		ClearUser(pTUser);
	}
	return pTUser;
}

void NFGameTable::ClientUpdateUserInfo(NFGamespace::DB_UserBaseInfo& baseInfo, int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
	{
		LogError("CGameTable::ClientUpdateUserInfo", "chairIndex error");
		return;
	}
	NF_SHARE_PTR<CTableUser> pTUser = m_userList[chairIndex];
	if (tusNone == pTUser->ustate || baseInfo.userId != pTUser->userInfo.baseInfo.userId)
	{
		LogError("CGameTable::ClientUpdateUserInfo", "no user or userId error");
		return;
	}

	//pTUser->userInfo.baseInfo.CopyFrom(baseInfo);
	//SendUserStateNotify(baseInfo.userId, pTUser->ustate, baseInfo.bean);
}

void NFGameTable::SendStartGameNotify(int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
	{
		LogError("CGameTable::SendStartGameNotify", "chairIndex error");
		return;
	}
	NF_SHARE_PTR<CTableUser> pTUser = m_userList[chairIndex];
	// robot do not need send
	if (pTUser->isRobot)
		return;
	auto mb = pTUser->userInfo.self;
	if (mb.IsNull())
	{
		LogError("CGameTable::SendStartGameNotify", "!mb");
		return;
	}

	// 通知开始游戏
	{
		////发送响应包
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_BEGINGAME_NOTIFY;
		jsonSend["tableHandle"] = m_handle;
		jsonSend["createUserId"] = m_createUserId;
		jsonSend["tbState"] = m_tstate;
		jsonSend["chairIndex"] = chairIndex;

		jsonSend["isReady"] = pTUser->isReady;
		jsonSend["totalBet"] = pTUser->totalBet;
		jsonSend["cardState"] = pTUser->cardState;
		jsonSend["bSeeCard"] = pTUser->bSeeCard;

		jsonSend["agreeEnd"] = pTUser->agreeEnd;
		jsonSend["tuserState"] = pTUser->ustate;
		jsonSend["ZScore"] = GetUserTotalScore(pTUser->userInfo.baseInfo.userId);
		jsonSend["hasPin"] = m_hasPin;

	//	CPluto* puStart = new CPluto();
	//	(*puStart).Encode(MSGID_CLIENT_BEGINGAME_NOTIFY) << m_handle << m_createUserId << m_tstate << chairIndex
	//		<< pTUser->isReady << pTUser->totalBet << pTUser->cardState << pTUser->bSeeCard
	//		<< pTUser->agreeEnd << pTUser->ustate << GetUserTotalScore(pTUser->userInfo.baseInfo.userId) << m_hasPin;
		m_betRule.WriteToPluto(jsonSend);

		jsonSend["minBean"] = m_minBean;
		jsonSend["curRound"] = m_curRound;
		jsonSend["maxRound"] = INT_MAX;
		jsonSend["tableNum"] = m_tableNum;
		jsonSend["selfIp"] = pTUser->userInfo.activeInfo.ip;
	//	(*puStart) << m_minBean << m_curRound << INT_MAX << m_tableNum << pTUser->userInfo.activeInfo.ip;
		m_tableRule.WriteRulePluto(jsonSend);
	//	(*puStart) << EndPluto;
	//	mb->PushPluto(puStart);
		m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_BEGINGAME_NOTIFY, jsonSend, pTUser->GetSUserInfo().self);
	}

	// 通知用户列表
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pItem = iter->second;
		if (pItem != pTUser)
		{
			//CPluto* pu = new CPluto();
			//(*pu).Encode(MSGID_CLIENT_OTHER_ENTER_NOTIFY) << pItem->userInfo.activeInfo.chairIndex
			//	<< pItem->isReady << pItem->totalBet << pItem->cardState << pItem->bSeeCard
			//	<< pItem->agreeEnd << pItem->ustate << GetUserTotalScore(pItem->userInfo.baseInfo.userId);
			//pItem->userInfo.baseInfo.WriteToPluto(*pu);
			//(*pu) << pItem->userInfo.activeInfo.ip;
			//(*pu) << EndPluto;
			//mb->PushPluto(pu);
			nlohmann::json jsonSend;
			jsonSend["action"] = NFMsg::MSGID_CLIENT_OTHER_ENTER_NOTIFY;
	
			jsonSend["chairIndex"] = pItem->userInfo.activeInfo.chairIndex;
			jsonSend["isReady"] = pItem->isReady;
			jsonSend["totalBet"] = pItem->totalBet;
			jsonSend["cardState"] = pItem->cardState;
			jsonSend["bSeeCard"]= pItem->bSeeCard;
			jsonSend["agreeEnd"] = pItem->agreeEnd;
			jsonSend["tuserState"] = pItem->ustate;
			jsonSend["ZScore"]= GetUserTotalScore(pItem->userInfo.baseInfo.userId);

			pItem->userInfo.baseInfo.WriteToPluto(jsonSend);
			jsonSend["ip"] = pItem->userInfo.activeInfo.ip;
			m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_OTHER_ENTER_NOTIFY, jsonSend, pTUser->GetSUserInfo().self);
		}
	}

	if (IsGaming())
	{
		// 通知完整信息 开始游戏和用户列表已经发送的数据就不用重复了
		//CPluto* puSyn = new CPluto();
		//(*puSyn).Encode(MSGID_CLIENT_G_SYN_NOTIFY) << m_curUserId << m_totalBet << m_hideMinBet << m_isInPin << m_betRound << m_decTimeCount \
		//	<< CanPin() << CanVs() << m_vipRoomType << m_firstBetUserId;
		//if (m_tstate >= tbsDealCard && pTUser->bSeeCard)
		//	pTUser->player.cardList.GetGameCard()->WriteToPluto(*puSyn);
		//else
		//	(*puSyn) << (uint16_t)0;
		//uint16_t len = (uint16_t)m_sitUserList.size();
		//(*puSyn) << len;
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_OTHER_ENTER_NOTIFY;
		jsonSend["curUserId"] = m_curUserId;
		jsonSend["totalBet"] = m_totalBet;
		jsonSend["hideMinBet"] = m_hideMinBet;
		jsonSend["isInPin"] = m_isInPin;
		jsonSend["betRound"] = m_betRound;
		jsonSend["decTimeCount"] = m_decTimeCount;
		jsonSend["canPin"] = CanPin();
		jsonSend["canVs"] = CanVs();
		jsonSend["vipRoomType"] = m_vipRoomType;
		jsonSend["firstBetUserId"] = m_firstBetUserId;

		if (m_tstate >= tbsDealCard && pTUser->bSeeCard)
		pTUser->player.cardList.GetGameCard()->WriteToPluto(jsonSend);
	    else
			jsonSend["cards"] = 0;

		for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
		{
			NF_SHARE_PTR<CTableUser> pTItem = iter->second;
			//(*puSyn) << pTItem->userInfo.baseInfo.userId << pTItem->isTrust << pTItem->bSeeCard << pTItem->cardState << pTItem->player.cardList.GetGameCard()->cardCount;
			nlohmann::json jsonStruct;

			jsonStruct["userId"] = pTItem->userInfo.baseInfo.userId;
			jsonStruct["isTrust"] = pTItem->isTrust;
			jsonStruct["seecard"] = pTItem->bSeeCard;
			jsonStruct["lostcard"] = pTItem->cardState;
			jsonStruct["cardCount"] = pTItem->player.cardList.GetGameCard()->cardCount;

			if (pTItem->totalBet > 0)
			{
				jsonStruct["len"] = (uint16_t)pTItem->betAry.size();
				//(*puSyn) << (uint16_t)pTItem->betAry.size();
				auto it = pTItem->betAry.begin();
				for (; it != pTItem->betAry.end(); it++)
				{
					jsonStruct["betAry"] = *it;
					//(*puSyn) << *it;
				}
			}
			else
			{
				jsonStruct["len"] = 0;
			}
				//(*puSyn) << (uint16_t)0;
			nlohmann::json jsonStructObject = nlohmann::json::object({ {"users",jsonStruct} });

			jsonSend.insert(jsonStructObject.begin(), jsonStructObject.end());
			

		}
		m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_G_SYN_NOTIFY, jsonSend, pTUser->GetSUserInfo().self);
		//(*puSyn) << m_betRule.baseBet << EndPluto;
		//mb->PushPluto(puSyn);
	}
}

int NFGameTable::GetEmptyChairAry(int* chairAry)
{
	int len = 0;
	int maxLen = (int)m_userList.size();
	// 优先进入空位置
	for (int i = 0; i < maxLen; ++i)
	{
		NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
		if (tusNone == pItem->ustate)
		{
			chairAry[len] = i;
			++len;
		}
	}
	// 其次替换机器人，游戏中不能替换机器人, 可能机器人已经发牌
	if (!IsGaming())
	{
		for (int i = 0; i < maxLen; ++i)
		{
			NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
			if (tusNone != pItem->ustate && pItem->isRobot)
			{
				chairAry[len] = i;
				++len;
			}
		}
	}

	return len;
}

int NFGameTable::GetRealUserCount()
{
	int ret = 0;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		if (!iter->second->isRobot)
			++ret;
	}

	return ret;
}

int NFGameTable::GetRealUserMaxSelScore()
{
	int ret = m_betRule.maxTotalBet;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pItem = iter->second;
		if (!pItem->isRobot)
		{
			if (pItem->userInfo.activeInfo.selScore > ret)
				ret = pItem->userInfo.activeInfo.selScore;
		}
	}

	return ret;
}

void NFGameTable::ClearRobotUserIfNoRealUser()
{
	if (m_sitUserList.size() > 0 && 0 == GetRealUserCount())
	{
		// 如果发现剩余都是机器人，删除所有机器人 循环m_userList是因为ClearUser里面m_sitUserList会改变
		for (vector<NF_SHARE_PTR<CTableUser>>::iterator iter = m_userList.begin(); iter != m_userList.end(); ++iter)
		{
			NF_SHARE_PTR<CTableUser> pTUser = *iter;
			if (pTUser->ustate != tusNone)
			{
				//LogInfo("CGameTable::ClearRobotUserIfNoRealUser", "handle=%d, robotId=%d", m_handle, pTUser->userInfo.baseInfo.userId);
				ClearUser(pTUser);
			}
		}
	}
}

bool NFGameTable::AllowUserGaming(int userId)
{
	if (m_pConfigAreaModule->allow_other_gaming)
		return true;

	if (FindUserById(userId))
		return true;

	map<int, int64_t>::iterator iter = m_totalScore.find(userId);
	if (m_totalScore.end() == iter)
		return false;
	else
		return true;
}

void NFGameTable::UpdateCurBaseScore()
{
	int64_t minBean = -1;
	int minSelScore = -1;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		// 机器人的豆不计算，因为他会自动离开，计算也有问题，机器人的豆范围配置可以和真人不一样
		if (tusNone != pTUser->ustate && !pTUser->isRobot)
		{
			if (minBean < 0)
				minBean = pTUser->userInfo.baseInfo.bean;
			else if (pTUser->userInfo.baseInfo.bean < minBean)
				minBean = pTUser->userInfo.baseInfo.bean;
			if (minSelScore < 0)
				minSelScore = pTUser->userInfo.activeInfo.selScore;
			else if (pTUser->userInfo.activeInfo.selScore < minSelScore)
				minSelScore = pTUser->userInfo.activeInfo.selScore;
		}
	}

	if (minBean < 0 || minSelScore < 0)
	{
		LogError("UpdateCurBaseScore", "minBean=%lld, minSelScore=%d", minBean, minSelScore);
	}
	else
	{
		SConfigScoreItem* pScoreItem = m_pConfigAreaModule->score_list->FindItem(minSelScore);
		if (!pScoreItem)
		{
			LogError("UpdateCurBaseScore", "find SelScore=%d failed", minSelScore);
		}
		else
		{
			SetBetRule(pScoreItem);
		}
	}
}

void NFGameTable::RunTime()
{
	if (m_forceLeaveSec != INVALID_TIME_COUNT)
	{
		--m_forceLeaveSec;
		if (m_forceLeaveSec <= 0)
		{
			ClearTableUser(ERROR_CODE_GAME_OVER, "牌局完成，系统强制您退出游戏");
			SetNotActive();
			return;
		}
	}

	if (m_clearRemainSec != INVALID_TIME_COUNT)
	{
		// 处理定时清理桌子，1秒内又有人来了就不清理了。
		if (m_sitUserList.size() > 0)
			m_clearRemainSec = INVALID_TIME_COUNT;
		else
		{
			--m_clearRemainSec;
			if (m_clearRemainSec <= 0)
				SetNotActive();

			return;
		}
	}

	switch (m_tstate)
	{
	case tbsNone:
	{
		--m_decTimeCount;
		if (m_decTimeCount < 0)
		{
			CheckCanDealCard();
		}
		break;
	}
	case tbsDealCard:
	{
		--m_decTimeCount;
		if (m_decTimeCount <= 0)
			StartBet();
		break;
	}
	case tbsBet:
	{
		--m_decTimeCount;
		if (INVALID_USERID == m_curUserId)
		{
			StartReportScore();
		}
		else
		{
			NF_SHARE_PTR<CTableUser> pTUser = FindUserById(m_curUserId);
			if (!pTUser)
			{
				LogError("CGameTable::RunTime", "bet find user failed");
				return;
			}

			if (pTUser->isTrust)
			{
				// 托管自动放弃，除非手动取消托管
				DoUserBet(pTUser, sbtFold, 0, 0);
			}
			else if (m_decTimeCount <= 0)
			{
				if (tusOffline == pTUser->ustate)
				{
					// 断线的人不操作一直等待
					//m_decTimeCount = 0;
					DoUserBet(pTUser, sbtFold, 0, 0);
					if (!pTUser->isTrust)
						DoUserTrust(pTUser, 1, INVALID_USERID);
				}
				else
				{
					// 逃跑或在线的人自动放弃
					DoUserBet(pTUser, sbtFold, 0, 0);
					if (!pTUser->isTrust)
						DoUserTrust(pTUser, 1, INVALID_USERID);
				}
			}
		}

		break;
	}
	case tbsDiscard:
	{
		--m_decTimeCount;
		NF_SHARE_PTR<CTableUser> pTUser = FindUserById(m_curUserId);
		if (!pTUser)
		{
			LogError("CGameTable::RunTime", "Discard find user failed");
			return;
		}

		break;
	}
	case tbsCalcResult:
	{
		uint32_t nowTick = GetNowMsTick();
		int elapseTick = nowTick - m_endGameTick;
		if (elapseTick >= ((NFCConfigAreaModule*)m_pConfigAreaModule)->robot_cfg.finish_wait_sec * MS_PER_SEC)
		{
			//SendGameResultNotify();
			AddGameRound();
			SendTableStateNotify();
			RoundStopClearData();
		}
		else
		{
			//LogInfo("CGameTable::RunTime", "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%等待玩家看牌");
		}
		break;
	}
	}


	{
		// 处理断线超时
		uint32_t nowTick = GetNowMsTick();
		for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
		{
			NF_SHARE_PTR<CTableUser> pItem = iter->second;
			if (tusOffline == pItem->ustate)
			{
				if (GetMsTickDiff(pItem->offlineTick, nowTick) >= (uint32_t)m_pConfigAreaModule->max_offline_sec * MS_PER_SEC)
				{
					LogInfo("CGameTable::RunTime", "断线超时 userId = %d", pItem->userInfo.baseInfo.userId);
					pItem->ustate = tusFlee;
					SendUserStateNotify(pItem->userInfo.baseInfo.userId, tusFlee, pItem->userInfo.baseInfo.bean);
				}
			}
		}
	}
}

void NFGameTable::RobotAction()
{
	if (!IsGaming())
	{
		// 非游戏状态不用判断tusWaitNextRound
		ClearRobotUserIfNoRealUser();

		{
			// 机器人自动ready
			uint32_t curTick = GetNowMsTick();
			for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
			{
				NF_SHARE_PTR<CTableUser> pTUser = iter->second;
				if (pTUser->isRobot && !pTUser->isReady)
				{
					if (GetMsTickDiff(m_endGameTick, curTick) >= pTUser->userInfo.activeInfo.robotReadyMs)
					{
						DoUserReady(pTUser);
					}
				}
			}
		}

		{
			// 踢出长时间不ready的人
			uint32_t curTick = GetNowMsTick();
			uint32_t maxTick = m_pConfigAreaModule->robot_cfg.max_waitready_sec * MS_PER_SEC;
			for (vector<NF_SHARE_PTR<CTableUser>>::iterator iter = m_userList.begin(); iter != m_userList.end(); ++iter)
			{
				NF_SHARE_PTR<CTableUser> pTUser = *iter;
				if (pTUser->ustate != tusNone && !pTUser->isReady)
				{
					if (GetMsTickDiff(pTUser->userInfo.activeInfo.enterTableTick, curTick) >= maxTick)
					{
						char buffer[100];
						snprintf(buffer, sizeof(buffer), "您%d秒内没有准备，系统强制您退出游戏", m_pConfigAreaModule->robot_cfg.max_waitready_sec);
						// 清理之前发送玩家日志
						if (!pTUser->isRobot)
							StartReportUserLog(pTUser);
						SendForceLeaveNotify(pTUser, 1, buffer);

						ClearUser(pTUser);
					}
				}
			}
		}

		if (GetRealUserCount() > 0)
		{
			// 游戏结束后，长时间不开局，机器人自动补人或离开
			uint32_t curTick = GetNowMsTick();
			int selScore = GetRealUserMaxSelScore();
			int len = (int)m_userList.size();
			for (int i = 0; i < len; ++i)
			{
				NF_SHARE_PTR<CTableUser> pTUser = m_userList[i];

				if (tusNone == pTUser->ustate)
				{
					// GetMsTickDiff不能提到外边
					if (GetMsTickDiff(pTUser->readyOrLeaveTick, curTick) >= pTUser->fillOrLeaveRobotMs)
					{
						// enter
						bool isAct = false;
						//						if (IsFix100Rate(CONFIG_AREA->robot_cfg.fill_rate))
						{
							NF_SHARE_PTR< SUserInfo> pRobotUser = m_pRobotMoudle->AllocRobotUser(selScore);
							if (pRobotUser)
							{
								isAct = EnterTable(pRobotUser, i, true);
								if (!isAct)
									m_pRobotMoudle->FreeRobotUser(pRobotUser->baseInfo.userId);
							}
						}

						if (!isAct)
							pTUser->fillOrLeaveRobotMs += 3 * MS_PER_SEC;
					}
				}
				else if (tusNomal == pTUser->ustate && pTUser->isReady && pTUser->isRobot)
				{
					if (GetMsTickDiff(pTUser->readyOrLeaveTick, curTick) >= pTUser->fillOrLeaveRobotMs)
					{
						// leave
   						if (!m_pConfigAreaModule->robot_cfg.fill_rate)
   						{
   							ClearUser(pTUser);
   						}
   						else
   						{
   							pTUser->fillOrLeaveRobotMs += 3 * MS_PER_SEC;
   						}
					}
				}
			}
		}
	}
	else
	{
		// robot bet
		if (m_tstate != tbsBet)
			return;
		if (INVALID_USERID == m_curUserId)
			return;
		// 更换方位，需要清理上个方位的robot信息
		if (m_lastUserId != m_curUserId)
		{
			NF_SHARE_PTR<CTableUser> pTUser = FindUserById(m_lastUserId);
			if (pTUser)
				pTUser->robotInfo.OperationClear();
			m_lastUserId = m_curUserId;
		}
		NF_SHARE_PTR<CTableUser> pTCurUser = FindUserById(m_curUserId);
		if (!pTCurUser)
		{
			LogError("robot declare discard", "find user failed");
			return;
		}
		if (!pTCurUser->isRobot)
			return;

		if (tbsBet == m_tstate)
			DoRobotBet(pTCurUser);
	}
}

void NFGameTable::SetIsActive()
{
	// 有人进入则倒计时结束。
	m_clearRemainSec = INVALID_TIME_COUNT;

	if (!m_isActive)
	{
		m_isActive = true;
		m_lastRunTick.SetNowTime();
		m_endGameTick = GetNowMsTick();
		m_StartGameTick = GetTimeStampInt64Ms();
		for (vector<NF_SHARE_PTR<CTableUser>>::iterator iter = m_userList.begin(); iter != m_userList.end(); ++iter)
		{
			(*iter)->readyOrLeaveTick = GetNowMsTick();
		}
	}
}

void NFGameTable::ClearTableUser(int errCode, const char* errMsg)
{
	int len = (int)m_userList.size();
	for (int i = 0; i < len; i++)
	{
		NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
		if (tusNomal == pItem->ustate)
			SendForceLeaveNotify(pItem, errCode, errMsg);
		if (pItem->ustate != tusNone)
			ClearUser(pItem);
	}
}

void NFGameTable::NoUserClearData()
{
	// 桌子的人都离开，清理信息，比如统计信息
	m_tstate = tbsNone;
	m_hasPin = 0;
	m_betRule.Clear();
	//if (m_createUserId != INVALID_USERID)
	//	m_pTableManagerModule->RemoveUserCreateTable(m_createUserId);
	m_createUserId = INVALID_USERID;
	m_curRound = 1;
	m_minBean = 0;
	m_tableNum = "";
	m_forceLeaveSec = INVALID_TIME_COUNT;
	m_clearRemainSec = INVALID_TIME_COUNT;
	m_minSpecialGold = 0;
	m_vipRoomType = vrtBean;
	ClearTotalScore();
	m_winUserId = 0;
	m_dissolveTable = false;
}

void NFGameTable::RoundStopClearData()
{
	// 每局结束清理信息，比如断线和逃跑玩家、游戏状态
	m_controlValue = -1;
	m_controlLimit = 0;
	m_getControlTimes = 0;
	m_endGameTick = GetNowMsTick();
	m_firstBetUserId = INVALID_USERID;
	m_totalBet = 0;
	m_hideMinBet = 0;
	m_tstate = tbsNone;
	m_isReportFinished = 0;
	if (m_curRound == 1)
	{
		m_decTimeCount = INVALID_TIME_COUNT;
	}
	else
	{
		m_decTimeCount = TIME_COUNT_DEAL_CARD * 3 + 1; //播放动画
	}
	m_curUserId = INVALID_USERID;
	m_resultType = grtNone;
	m_isInPin = 0;
	m_betRound = 0;
	m_showCardUsers.clear();
	m_roundFee = 0;
	m_clearTableTick = GetNowMsTick();
	int len = (int)m_userList.size();
	// 清理断线用户
	for (int i = 0; i < len; i++)
	{
		NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
		if (pItem->ustate > tusNomal)
		{
			// 清理断线用户之前先统计玩家日志
			if (!pItem->isRobot)
				StartReportUserLog(pItem);
			if (!pItem->isAbnormal)
			{
				LogInfo("CGameTable::RoundStopClearData", "清理断线用户解锁");
				UnlockOrlockUser(pItem, UN_LOCK_USER_LEAVE);
				ClearUser(pItem);
			}
		}
	}
	// 清理游戏豆不足用户 机器人正常踢出，下次分桌机器人就自动加豆
	for (int i = 0; i < len; i++)
	{
		NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
		if (pItem->ustate != tusNone)
		{
			if (m_betRule.minBean != 0 && pItem->userInfo.baseInfo.bean < m_betRule.minBean && !pItem->isRobot)
			{
				//解
				UnlockOrlockUser(pItem, UN_LOCK_USER_LEAVE);
				// 机器人在此处踢出
				char buffer[100];
				snprintf(buffer, sizeof(buffer), "您的%s不足%d，系统强制您退出游戏", m_pConfigAreaModule->bean_name.c_str(), (int)(m_betRule.minBean / 100));
				SendForceLeaveNotify(pItem, ERROR_CODE_BEAN_TOO_LITTLE, buffer);
				ClearUser(pItem);
			}
			else if (pItem->isRobot)
			{
				// 机器人不符合selScore
				if (!m_pConfigAreaModule->RobotScoreValid(pItem->userInfo.baseInfo.bean, pItem->userInfo.activeInfo.selScore))
				{
					LogInfo("NFGameTable::RoundStopClearData", "robotId=%d bean out of selScore", pItem->userInfo.baseInfo.userId);
					ClearUser(pItem);
				}
			}
		}
	}

	// 清理游戏数据
	for (int i = 0; i < len; i++)
	{
		NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
		if (pItem->ustate != tusNone)
		{
			pItem->isReady = false;
			pItem->cardState = ucsNormal;
			pItem->bSeeCard = 0;
			pItem->isTrust = false;
			pItem->manualTrust = false;
			pItem->vsUserIds.clear();
			pItem->hasDoBet = false;
			pItem->userInfo.activeInfo.enterTableTick = GetNowMsTick();
			pItem->player.RoundClear();
			pItem->robotInfo.Clear();
			pItem->betAry.clear();
			pItem->IsMenPai = false;
			pItem->betTick.SetNowTime();
			pItem->roundIncBean = 0;
			pItem->roundRevenue = 0;
			pItem->totalBet = 0;
			if (tusWaitNextRound == pItem->ustate)
			{
				pItem->ustate = tusNomal;
				SendUserStateNotify(pItem->userInfo.baseInfo.userId, pItem->ustate, pItem->userInfo.baseInfo.bean);
			}
		}
	}

	m_robotWinUsers.clear();
	m_lastUserId = INVALID_USERID;
}

void NFGameTable::AddGameRound()
{
	++m_curRound;
}

void NFGameTable::AddUser(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex)
{
	pUser->activeInfo.userState = EUS_INTABLE;
	pUser->activeInfo.tableHandle = m_handle;
	pUser->activeInfo.chairIndex = chairIndex;

	int userId = pUser->baseInfo.userId;
	if (m_sitUserList.find(userId) == m_sitUserList.end())
	{
		m_userList[chairIndex]->userInfo.self = pUser->self;
		m_userList[chairIndex]->userInfo.CopyFrom(*pUser);

		m_sitUserList.insert(make_pair(userId, m_userList[chairIndex]));
		m_pTableManagerModule->AddSitUser(userId, m_handle, chairIndex);
	}
}

void NFGameTable::ClearUser(NF_SHARE_PTR<CTableUser>  pTUser)
{
	int userId = pTUser->userInfo.baseInfo.userId;
	m_sitUserList.erase(userId);
	m_pTableManagerModule->RemoveSitUser(userId);
	if (pTUser->isRobot)
		m_pRobotMoudle->FreeRobotUser(userId);

	pTUser->Clear();
	NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByUserId(userId);
	if (pUser)
	{
		pUser->activeInfo.userState = EUS_AUTHED;
		pUser->activeInfo.tableHandle = -1;
		pUser->activeInfo.chairIndex = -1;
	}

	// 给其他人发送离开桌子包
// 	CPluto* pu = new CPluto();
// 	(*pu).Encode(MSGID_CLIENT_OTHER_LEAVE_NOTIFY) << userId << EndPluto;
// 	SendBroadTablePluto(pu, userId);
// 
// 	if (m_sitUserList.size() <= 0)
// 	{
// 		m_clearRemainSec = 0;
// 	}

	LogInfo("CGameTable::ClearUser", "userId=%d left user count=%d", userId, m_sitUserList.size());
}

NF_SHARE_PTR<CTableUser>  NFGameTable::FindUserById(int userId)
{
	map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.find(userId);
	if (m_sitUserList.end() == iter)
		return NULL;

	return iter->second;
}

NF_SHARE_PTR<CTableUser>  NFGameTable::FindUserByChairIndex(int chairIndex)
{
	if (!IsChairIndexValid(chairIndex))
		return NULL;

	return m_userList[chairIndex];
}

NF_SHARE_PTR<CTableUser>   NFGameTable::FindSitUserByChairIndex(int chairIndex)
{
	if (!(IsChairIndexValid(chairIndex)))
		return NULL;

	NF_SHARE_PTR<CTableUser> ret = m_userList[chairIndex];
	if (ret->ustate == tusNone)
		return NULL;

	return ret;
}

// CMailBox* CGameTable::GetTUserMailBox(NF_SHARE_PTR<CTableUser> pTUser)
// {
// 	CMailBox* ret = NULL;
// 	if (HasFd(pTUser))
// 	{
// 		CMailBox* mb = WORLD_MGR->GetServer()->GetMailboxByFd(pTUser->userInfo.activeInfo.fd);
// 		if (mb)
// 		{
// 			ret = mb;
// 		}
// 		else
// 		{
// 			LogError("CGameTable::GetTUserMailBox", "find mb failed. userId=%d", pTUser->userInfo.baseInfo.userId);
// 		}
// 	}
// 
// 	return ret;
// }

void NFGameTable::ClearTotalScore()
{
	m_totalScore.clear();
}

void NFGameTable::CheckMayClearTotalScore()
{
	// 新人加入，开始游戏后，判断是否清理统计
	for (map<int, int64_t>::iterator iter = m_totalScore.begin(); iter != m_totalScore.end(); ++iter)
	{
		if (!FindUserById(iter->first))
		{
			ClearTotalScore();
			LogInfo("CGameTable::CheckMayClearTotalScore", "clear TotalScore handle=%d", m_handle);
			return;
		}
	}
}

int64_t NFGameTable::GetUserTotalScore(int userId)
{
	map<int, int64_t>::iterator iter = m_totalScore.find(userId);
	if (m_totalScore.end() == iter)
		return 0;
	else
		return iter->second;
}

void NFGameTable::AddUserTotalScore(int userId, int64_t incScore)
{
	map<int, int64_t>::iterator iter = m_totalScore.find(userId);
	if (m_totalScore.end() == iter)
		m_totalScore.insert(make_pair(userId, incScore));
	else
		iter->second += incScore;
}

void NFGameTable::SendBroadTablePluto(CPluto* pu, int ignoreUserId)
{
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (pTUser->userInfo.baseInfo.userId != ignoreUserId)
		{
			// 			CMailBox* mb = GetTUserMailBox(pTUser);
			// 			if (mb)
			// 			{
			// 				CPluto* pSend = new CPluto(pu->GetLen());
			// 				pSend->OverrideBuffer(pu->GetBuff(), (uint16_t)pu->GetLen());
			// 				mb->PushPluto(pSend);
			// 			}
		}
	}

	delete pu;
}
void  NFGameTable::SendBroadTablePluto(const uint16_t nMsgId, const nlohmann::json & jsonObject, int ignordUserId)
{

	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{

		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (pTUser == nullptr)
		{
			continue;
		}

		if (pTUser->userInfo.baseInfo.userId != ignordUserId)
		{
			auto selfObj = m_pGameLogicModule->GetUserInfoByUserId(pTUser->userInfo.baseInfo.userId);
			if (selfObj)
			{
				//std::string msgStr = jsonObject.dump();
				//m_pGameServerNet_ServerModule->SendMsgToGate(nMsgId, msgStr, selfObj->self);
				m_pGameLogicModule->SendMsgToClient(nMsgId, jsonObject, selfObj->self);
			}
		}
	}
}
void  NFGameTable::SendBroadTablePluto(const uint16_t nMsgID, const std::string& strMsg, int ignordUserId)
{


	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{

		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (pTUser == nullptr)
		{
			continue;
		}

		if (pTUser->userInfo.baseInfo.userId != ignordUserId)
		{
			auto selfObj = m_pGameLogicModule->GetUserInfoByUserId(pTUser->userInfo.baseInfo.userId);
			if (selfObj)
			{
				m_pGameServerNet_ServerModule->SendMsgToGate(nMsgID, strMsg, selfObj->self);
			}
		}
	}

}

void NFGameTable::SendPlutoToUser(const uint16_t nMsgId, const nlohmann::json & jsonObject, const NFGUID& clientGuid)
{
	if (!m_pGameLogicModule->FindUserByGuid(clientGuid))
	{
		std::ostringstream strLog;
		strLog << "没有找到guid:" << clientGuid.ToString();
		m_pLogModule->LogError(strLog, __FUNCTION__, __LINE__);
		return;
	}
	m_pGameLogicModule->SendMsgToClient(nMsgId, jsonObject, clientGuid);
	//std::string msgStr = jsonObject.dump();
	//m_pGameServerNet_ServerModule->SendMsgToGate(nMsgId, msgStr, clientGuid);
	// 	CMailBox* mb = GetTUserMailBox(pTUser);
	// 	if (mb)
	// 	{
	// 		mb->PushPluto(pu);
	// 	}
	// 	else
	// 	{
	// 		delete pu;
	// 	}
}

void NFGameTable::SendPlutoToUser(const uint16_t nMsgID, const std::string& strMsg, const NFGUID& self)
{
	if (!m_pGameLogicModule->FindUserByGuid(self))
	{
		std::ostringstream strLog;
		strLog << "没有找到guid:" << self.ToString();
		m_pLogModule->LogError(strLog, __FUNCTION__, __LINE__);
		return;
	}
	m_pGameServerNet_ServerModule->SendMsgToGate(nMsgID, strMsg, self);
}

void NFGameTable::SendPlutoToUser(const uint16_t nMsgID, const std::string& strMsg, const int& userId)
{
	if (!m_pGameLogicModule->FindUserByUserId(userId))
	{
		std::ostringstream strLog;
		strLog << "没有找到guid:" << std::to_string(userId);
		m_pLogModule->LogError(strLog, __FUNCTION__, __LINE__);
		return;
	}
	auto selfObj = m_pGameLogicModule->GetUserInfoByUserId(userId);
	if (selfObj)
	{
		m_pGameServerNet_ServerModule->SendMsgToGate(nMsgID, strMsg, selfObj->self);
	}

}

void NFGameTable::SendForceLeaveNotify(NF_SHARE_PTR<CTableUser> pTUser, int code, const char* errorMsg)
{
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY;
	jsonSend["code"] = 62;
	m_pGameLogicModule->SendMsgToClient(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, jsonSend, pTUser->userInfo.self);
	// 	CPluto* pu = new CPluto();
	// 	(*pu).Encode(MSGID_CLIENT_FORCE_LEAVE_NOTIFY) << code << errorMsg << EndPluto;
	// 	SendPlutoToUser(pu, pTUser);
}

void NFGameTable::DoUserReady(NF_SHARE_PTR<CTableUser> pTUser)
{
	if (!pTUser->isReady)
	{
		// 		pTUser->isReady = true;
		// 		pTUser->readyOrLeaveTick = GetNowMsTick();
		// 		pTUser->fillOrLeaveRobotMs = CONFIG_AREA->GetRobotFillMs();
		// 
		// 		int userId = pTUser->userInfo.baseInfo.userId;
		// 		CPluto* pu = new CPluto();
		// 		(*pu).Encode(MSGID_CLIENT_OTHER_READY_NOTIFY) << userId << EndPluto;
		// 		SendBroadTablePluto(pu, userId);
	}
	else
	{
		LogError("CGameTable::DoUserReady", "isReady");
	}
}

void NFGameTable::DoUserTrust(NF_SHARE_PTR<CTableUser> pTUser, int isTrust, int ignoreUserId)
{
	if (pTUser->isTrust != isTrust)
	{
		// 		pTUser->isTrust = isTrust;
		// 
		// 		int userId = pTUser->userInfo.baseInfo.userId;
		// 		CPluto* pu = new CPluto();
		// 		(*pu).Encode(MSGID_CLIENT_G_OTHER_TRUST_NOTIFY) << userId << isTrust << EndPluto;
		// 		SendBroadTablePluto(pu, ignoreUserId);
	}
	else
	{
		LogError("CGameTable::DoUserTrust", "trust state error");
	}
}

void NFGameTable::DoUserBet(NF_SHARE_PTR<CTableUser> pTUser, int betType, int hideBetValue, int vsUserId)
{
	if (!pTUser->CanBet())
	{
		LogError("CGameTable::DoUserBet", "userId=%d can not bet", pTUser->userInfo.baseInfo.userId);
		return;
	}

	int vsWinUserId = INVALID_USERID;
	int betValue = 0;

	bool isSeeCard = false;
	int userId = pTUser->userInfo.baseInfo.userId;
	switch (betType)
	{
	case sbtFold:
	{
		pTUser->cardState = ucsFold;
		break;
	}
	case sbtVsCard:
	{
		int tempMinBet = m_hideMinBet * 2;
		if (pTUser->bSeeCard)
		{
			tempMinBet += tempMinBet;
		}

		if (pTUser->totalBet + tempMinBet > pTUser->userInfo.baseInfo.bean)
		{
			// 下注金币超过了自身金币总数，不允许下注
			//SendForceLeaveNotify(pTUser, ERROR_CODE_BEAN_TOO_LITTLE, "金币不足");
  // 			CPluto* pu = new CPluto();
  // 			(*pu).Encode(MSGID_ERROR_CODE_KICK_LESSBEAN) << 2 << EndPluto;
  // 			SendPlutoToUser(pu, pTUser);
  // 			pTUser->cardState = ucsFold;
  // 			//
  // 			betType = sbtFold;
  // 			betValue = 0;
			break;
		}
		// 比：根据规则来算翻倍
		//if (m_tableRule.pkDouble)
		//	betValue = 2 * m_hideMinBet;
		//else
		betValue = m_hideMinBet;
		NF_SHARE_PTR<CTableUser> pTVsWho = FindUserById(vsUserId);
		if (userId == vsUserId || !pTVsWho || !pTVsWho->CanBet())
		{
			LogError("CGameTable::DoUserBet", "find VsUserId=%d failed", vsUserId);;
			return;
		}

		int vsRet = pTUser->player.cardList.m_cardType.CompareCardType(pTVsWho->player.cardList.m_cardType);
		// 大小一样的情况，发起者失败
		if (0 == vsRet)
			vsRet = -1;
		if (vsRet < 0)
		{
			vsWinUserId = vsUserId;
			pTUser->cardState = ucsVsFailed;
		}
		else
		{
			vsWinUserId = userId;
			pTVsWho->cardState = ucsVsFailed;
		}
		pTUser->vsUserIds[vsUserId] = vsUserId;
		pTVsWho->vsUserIds[userId] = userId;

		// 比牌之后重置时间
		m_decTimeCount = m_pConfigAreaModule->bet_sec;

		break;
	}
	case sbtCall:
	{
		int tempMinBet = m_hideMinBet;
		if (pTUser->bSeeCard)
		{
			tempMinBet += tempMinBet;
		}
		if (pTUser->totalBet + tempMinBet > pTUser->userInfo.baseInfo.bean)
		{
			// 下注金币超过了自身金币总数，不允许下注
			//SendForceLeaveNotify(pTUser, ERROR_CODE_BEAN_TOO_LITTLE, "金币不足");
			//CPluto* pu = new CPluto();
			//(*pu).Encode(MSGID_ERROR_CODE_KICK_LESSBEAN) << 0 << EndPluto;
			SendPlutoToUser(NFMsg::MSGID_ERROR_CODE_KICK_LESSBEAN, "", userId);
			pTUser->cardState = ucsFold;
			//
			betType = sbtFold;
			break;
		}

		betValue = m_hideMinBet;
		if (m_betRound == 1)
		{
			pTUser->IsMenPai = true;

		}
		break;
	}
	case sbtRaise:
	{
		int tempMinBet = hideBetValue;
		if (pTUser->bSeeCard)
		{
			tempMinBet += tempMinBet;
		}
		if (!pTUser->isRobot)
			LogInfo("我加注了！！！：start", "totalBet:%d,hideBetValue:%d,userInfo.bean:%d , userid:%d", pTUser->totalBet, hideBetValue, pTUser->userInfo.baseInfo.bean, userId);
		if (pTUser->totalBet + tempMinBet > pTUser->userInfo.baseInfo.bean)
		{
			// 下注金币超过了自身金币总数，不允许下注
			//SendForceLeaveNotify(pTUser, ERROR_CODE_BEAN_TOO_LITTLE, "金币不足");
			//CPluto* pu = new CPluto();
			//(*pu).Encode(MSGID_ERROR_CODE_KICK_LESSBEAN) << 1 << EndPluto;
			//SendPlutoToUser(pu, pTUser);
			SendPlutoToUser(NFMsg::MSGID_ERROR_CODE_KICK_LESSBEAN, "", userId);
			pTUser->cardState = ucsFold;

			betType = sbtFold;
			break;
		}
		//LogInfo("我加注了！！！：", "totalBet:%d,hideBetValue:%d,userInfo.bean:%d , userid:%d", pTUser->totalBet, hideBetValue, pTUser->userInfo.baseInfo.bean, userId);
		betValue = hideBetValue;
		if (m_betRound == 1)
		{
			pTUser->IsMenPai = true;

		}
		break;
	}
	case sbtPin:
	{
		if (!m_isInPin)
		{
			m_isInPin = 1;
		}

		betValue = m_betRule.pinBet;

		break;
	}
	case sbtSeeCard:
	{
		isSeeCard = true;
		pTUser->bSeeCard = 1;
		break;
	}
	default:
	{
		LogError("CGameTable::DoUserBet", "betType=%d", betType);
		return;
	}
	}
	if (betValue > 0)
	{
		// 记录下注日志
		SActionLogItem *item = new SActionLogItem;
		item->userId = userId;
		item->betBean = m_hideMinBet;
		item->currTimes = (int)(GetTimeStampInt64Sec() - m_matchLog.startTime);
		item->tstate = betType;
		m_matchLog.actionLog.push_back(item);
	}

	if (sbtSeeCard != betType)
	{
		pTUser->hasDoBet = true;

		if (betValue > 0)
		{
			m_hideMinBet = betValue;
			if (pTUser->bSeeCard)
				betValue += betValue;
			if (sbtVsCard == betType)
			{
				//主p *2
				betValue += betValue;
			}
			pTUser->betAry.push_back(betValue);
			pTUser->totalBet += betValue;
			m_totalBet += betValue;
		}
	}


	bool isEndBet = false;
	NF_SHARE_PTR<CTableUser> pNextUser = NULL;
	{
		int fromPlace = (pTUser->userInfo.activeInfo.chairIndex + 1) % MAX_TABLE_USER_COUNT;
		// seeCard 不修改curUserId
		// TODO : 比牌赢了的情况下也不修改curUserId
		if (isSeeCard || (betType == sbtVsCard && pTUser->cardState != ucsVsFailed))
			fromPlace = pTUser->userInfo.activeInfo.chairIndex;
		if (betType == sbtFold && m_curUserId != userId)								//玩家弃牌,并且当前不是该玩家操作
		{
			pNextUser = FindUserById(m_curUserId);
		}
		else
		{
			for (int i = 0; i < MAX_TABLE_USER_COUNT - 1; ++i)
			{
				NF_SHARE_PTR<CTableUser> pItem = m_userList[(i + fromPlace) % MAX_TABLE_USER_COUNT];
				if (pItem->CanBet())
				{
					pNextUser = pItem;
					break;
				}
			}
		}


		int leftUserCnt = 0;
		NF_SHARE_PTR<CTableUser> pOneCanBetUser = NULL;
		for (int i = 0; i < MAX_TABLE_USER_COUNT; ++i)
		{
			NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
			if (pItem->CanBet())
			{
				++leftUserCnt;
				pOneCanBetUser = pItem;
			}
		}

		if (leftUserCnt <= 1)
		{
			isEndBet = true;
			bool isWinByOtherFold = (sbtFold == betType);
			if (isWinByOtherFold)
			{
				m_resultType = grtBluff;
				/*
				if (pOneCanBetUser)
				pOneCanBetUser->player.cardList.m_cardType.TypeNum = sctBluff;
				else
				LogError("CGameTable::DoUserBet", "no user in game");
				*/
				// 偷鸡的牌不被没有和自己vs过玩家看
			}
			else
			{
				m_resultType = grtUserVs;
				// 最后开牌，vs失败的牌也可以被其他人看到牌
				m_showCardUsers[userId] = userId;
				m_showCardUsers[vsUserId] = vsUserId;
			}
		}
		else
		{
			if (!pNextUser)
			{
				LogError("CGameTable::DoUserBet", "find next user failed");
			}
			else
			{
				bool allHasDoBet = pNextUser->hasDoBet;
				if (allHasDoBet)
				{
					// 到封顶需要本轮完成才强制开
					bool toMaxBet = m_totalBet >= m_betRule.maxTotalBet;
					if (toMaxBet)
					{
						isEndBet = true;
						m_resultType = grtSysForceVs;

						// 强制开，所有人都可以看到开的玩家的牌
						for (int i = 0; i < MAX_TABLE_USER_COUNT; ++i)
						{
							NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
							if (pItem->CanBet())
							{
								m_showCardUsers[pItem->userInfo.baseInfo.userId] = pItem->userInfo.baseInfo.userId;
							}
						}
					}
					else
					{
						// 进行下一轮
						++m_betRound;
						for (int i = 0; i < MAX_TABLE_USER_COUNT; ++i)
						{
							NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
							if (pItem->CanBet())
							{
								pItem->hasDoBet = false;
							}
						}
					}

				}
			}
		}
	}

	if (isEndBet)
	{
		// 游戏结束
		m_decTimeCount = INVALID_TIME_COUNT;
		m_curUserId = INVALID_USERID;
		LogInfo("CGameTable::DoUserBet", "game End");

	}
	else
	{
		// 其他人弃牌时不刷新时间
		if (betType != sbtFold || (betType == sbtFold && m_curUserId == userId))
		{
			m_decTimeCount = m_pConfigAreaModule->bet_sec;
			m_curUserId = pNextUser->userInfo.baseInfo.userId;
			pNextUser->betTick.SetNowTime();
		}
	}

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_G_BET_NOTIFY;
	jsonSend["betUserId"] = userId;
	jsonSend["betType"] = betType;
	jsonSend["betValue"] = betValue;
	jsonSend["UserBetAll"] = pTUser->totalBet;
	jsonSend["vsUserId"] = vsUserId;
	jsonSend["vsWinUserId"] = vsWinUserId;
	jsonSend["curUserId"] = m_curUserId;
	jsonSend["totalBet"] = m_totalBet;
	jsonSend["hideMinBet"] = m_hideMinBet;
	jsonSend["betRound"] = m_betRound;
	jsonSend["canPin"] = CanPin();
	jsonSend["canVs"] = CanVs();
	jsonSend["decTimeCount"] = m_decTimeCount;


	SendBroadTablePluto(NFMsg::MSGID_CLIENT_G_BET_NOTIFY, jsonSend, INVALID_USERID);
	//CPluto* pu = new CPluto();
	//(*pu).Encode(MSGID_CLIENT_G_BET_NOTIFY) << userId << betType << betValue << pTUser->totalBet << vsUserId << vsWinUserId \
	//	<< m_curUserId << m_totalBet << m_hideMinBet << m_betRound << CanPin() << CanVs() << m_decTimeCount << EndPluto;
	//SendBroadTablePluto(pu, INVALID_USERID);
}

void NFGameTable::DoUserSeeCard(NF_SHARE_PTR<CTableUser> pTUser)
{
	if (1 == pTUser->bSeeCard)
	{
		LogError("DoUserSeeCard", "1==bSeeCard");
		return;
	}
	{
		pTUser->bSeeCard = 1;

		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_G_SEE_CARD_NOTIFY;
		jsonSend["userId"] = pTUser->userInfo.baseInfo.userId;
		SendBroadTablePluto(NFMsg::MSGID_CLIENT_G_SEE_CARD_NOTIFY, jsonSend, INVALID_USERID);
		// 		CPluto* pu = new CPluto();
		// 		(*pu).Encode(MSGID_CLIENT_G_SEE_CARD_NOTIFY) << pTUser->userInfo.baseInfo.userId << EndPluto;
		// 		SendBroadTablePluto(pu, INVALID_USERID);
	}
}

void NFGameTable::CheckCanDealCard()
{
	if (tbsNone != m_tstate)
	{
		LogError("CGameTable::CheckCanDealCard", "state error");
		return;
	}

	int readyCount = 0;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		if (iter->second->isReady)
			++readyCount;
		else
			return;
	}
	if (readyCount < MIN_TABLE_USER_COUNT)
		return;

	// 获取游戏控制信息, 在等待中则不继续发送
	if (m_getControlTimes == 1)
		StartGetGameControl();

	// 超过5次则不进行控制
	if (m_controlValue == -1 && m_getControlTimes <= 5)
	{
		LogInfo("CGameTable::CheckCanDealCard", "等待请求控制信息");
		m_getControlTimes += 1;
		return;
	}

	// 游戏开始 记录日志
	uint64_t stamp = GetTimeStampInt64Ms();
	snprintf(m_gameStartMsStamp, sizeof(m_gameStartMsStamp), "%llu", stamp);
	m_matchLog.startTime = GetTimeStampInt64Sec();

	uint64_t matchId = (uint64_t)(2 * pow(10, 17) + m_handle * pow(10, 13) + (double)stamp);
	m_matchLog.matchId = matchId;
	//LogInfo("CGameTable::CheckCanDealCard", "matchId=%lld", matchId);

	m_matchLog.userCount = GetCurUserCount();
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (m_matchLog.channelId.length() != 0)
			m_matchLog.channelId += ",";
		m_matchLog.channelId += to_string(pTUser->userInfo.baseInfo.channelId);
	}

	m_tstate = tbsDealCard;
	m_decTimeCount = TIME_COUNT_DEAL_CARD;

	vector<CPlayerCard*> userCards;
	int sitUserLen = (int)m_sitUserList.size();
	int randIndex = rand() % sitUserLen;
	int curIndex = 0;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;

		if (curIndex == randIndex)
		{
			m_firstBetUserId = pTUser->userInfo.baseInfo.userId;
		}

		userCards.push_back(&pTUser->player.cardList);
		pTUser->totalBet = m_betRule.baseBet;
		m_totalBet += m_betRule.baseBet;
		pTUser->betAry.push_back(m_betRule.baseBet);
		++curIndex;
		pTUser->betTick.SetNowTime();
	}

	m_hideMinBet = m_betRule.baseBet;

	// 测试用，始终让玩家赢
	//m_controlValue = 0;

	int selectId = -1;

	if (sitUserLen - GetRealUserCount() != 0)
	{
		// 当前桌子有机器人玩家，可以进行控制
		if (m_controlValue == 0)
		{
			int randIndex = rand() % GetRealUserCount();
			int curIndex = 0;
			int count = 0;
			for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
			{
				NF_SHARE_PTR<CTableUser> pTUser = iter->second;
				if (!pTUser->isRobot)
				{
					if (curIndex == randIndex)
						selectId = count;
					curIndex++;
				}
				count++;
			}
		}
		else if (m_controlValue == 1)
		{
			int randIndex = rand() % ((int)m_sitUserList.size() - GetRealUserCount());
			int curIndex = 0;
			int count = 0;
			for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
			{
				NF_SHARE_PTR<CTableUser> pTUser = iter->second;
				if (pTUser->isRobot)
				{
					if (curIndex == randIndex)
						selectId = count;
					curIndex++;
				}
				count++;
			}
		}
	}


	LOGIC_MGR->DealCard(userCards, selectId);


	//LogInfo("CGameTable::CheckCanDealCard", "$$$$$$$$$$$$$$$选中的id为 : %d", selectId);
	for (int i = 0; i < sitUserLen; ++i)
	{
		//LogInfo("CGameTable::CheckCanDealCard", "$$$$$$$$$$$$$$$%d 的当前牌组为 : %s", i, LOGIC_MGR->GameCardAry2Str(*userCards[i]->GetGameCard()).c_str());
	}
	//记录日志 玩家手牌
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		SInfoLogItem* item = new SInfoLogItem;
		m_matchLog.infoLog[pTUser->userInfo.baseInfo.userId] = item;
		// 扎金花起始手牌和结束手牌一致
		item->startCards = LOGIC_MGR->GameCardAry2Str(*(pTUser->player.cardList.GetGameCard()));
		item->endCards = LOGIC_MGR->GameCardAry2Str(*(pTUser->player.cardList.GetGameCard()));
	}
	CalcWinUsers(m_robotWinUsers);
	collectTypeNum();
	// send pack
	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_G_DEALCARD_NOTIFY;
		jsonSend["totalBet"] = m_totalBet;
		jsonSend["firstBetUserId"] = m_firstBetUserId;
		jsonSend["curRound"] = m_curRound;
		SendBroadTablePluto(NFMsg::MSGID_CLIENT_G_DEALCARD_NOTIFY, jsonSend, INVALID_USERID);

	}

	// 游戏开局就扣费
	m_hasStartGame = true;
	CheckMayClearTotalScore();
}

void NFGameTable::StartBet()
{
	m_tstate = tbsBet;
	m_decTimeCount = m_pConfigAreaModule->bet_sec;
	m_curUserId = m_firstBetUserId;
	m_betRound = 1;

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_G_BET_NOTIFY;
	jsonSend["betUserId"] = (int)INVALID_USERID;
	jsonSend["betType"] = (int)0;
	jsonSend["betValue"] = (int)0;
	jsonSend["UserBetAll"] = (int)0;
	jsonSend["vsUserId"] = int(0);
	jsonSend["vsWinUserId"] = int(0);
	jsonSend["curUserId"] = m_curUserId;
	jsonSend["totalBet"] = m_totalBet;
	jsonSend["hideMinBet"] = m_hideMinBet;
	jsonSend["betRound"] = m_betRound;
	jsonSend["canPin"] = CanPin();
	jsonSend["canVs"] = CanVs();
	jsonSend["decTimeCount"] = m_decTimeCount;
	SendBroadTablePluto(NFMsg::MSGID_CLIENT_G_BET_NOTIFY, jsonSend, INVALID_USERID);

	// 	CPluto* pu = new CPluto();
	// 	(*pu).Encode(MSGID_CLIENT_G_BET_NOTIFY) << (int)INVALID_USERID << (int)0 << (int)0 << (int)0 << int(0) << (int)0 \
	// 		<< m_curUserId << m_totalBet << m_hideMinBet << m_betRound << CanPin() << CanVs() << m_decTimeCount << EndPluto;
	// 	SendBroadTablePluto(pu, INVALID_USERID);
}

void NFGameTable::StartReportScore()
{
	m_tstate = tbsCalcResult;

	//CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
	if (m_pNFIGameServerToDBModule)
	{
		{
			// 计算分数
			map<int, int> winUsers;
			CalcWinUsers(winUsers);

			int winLen = (int)winUsers.size();
			if (winLen < 1)
			{
				LogError("CGameTable::StartReportScore", "calc win user failed handle=%d", m_handle);
			}
			else
			{
				int oneUserWin = m_totalBet / winLen;
				int modWin = m_totalBet % winLen; // 给其中一个赢家
				int SpecialScore = 0;							// 喜分
				if (m_tableRule.baoZiJiangLi)
				{
					for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
					{
						NF_SHARE_PTR<CTableUser> pTItem = iter->second;
						if (pTItem->ustate >= tusNomal)
						{
							int userId = pTItem->userInfo.baseInfo.userId;
							if (winUsers.find(userId) != winUsers.end() && pTItem->IsMenPai)
							{
								if (pTItem->player.cardList.m_cardType.TypeNum == sct3OfAKind)
								{
									SpecialScore = m_pConfigAreaModule->sct3OfAKindScore;
								}
								else if (pTItem->player.cardList.m_cardType.TypeNum == sctStraightFlush)
								{
									SpecialScore = m_pConfigAreaModule->straightFlushScore;
								}
							}
							int isRobot = pTItem->isRobot;
							if (!isRobot)
							{
								LogInfo("用户开始上报, 给用户解锁", "userid:%d,", userId);
								//UnlockOrlockUser(pTItem, UN_LOCK_USER_LEAVE);
							}
						}
					}
				}

				for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
				{
					NF_SHARE_PTR<CTableUser> pTItem = iter->second;
					if (pTItem->ustate >= tusNomal)
					{
						int userId = pTItem->userInfo.baseInfo.userId;

						if (winUsers.find(userId) == winUsers.end())
						{
							// lose user
							pTItem->roundIncBean = -(pTItem->totalBet + SpecialScore);
							pTItem->loseRound++;
						}
						else
						{
							if (modWin != 0)
							{
								pTItem->roundIncBean = (oneUserWin + modWin) - pTItem->totalBet + (GetCurUserCount() - winLen) * SpecialScore;
								modWin = 0;
							}
							else
							{
								pTItem->roundIncBean = oneUserWin - pTItem->totalBet + (GetCurUserCount() - winLen) * SpecialScore;
							}
							pTItem->winRound++;
							m_winUserId = pTItem->userInfo.baseInfo.userId;
						}

					}
				}
			}
		}

		//创建任务
		//CAreaTaskReportScore* task = new CAreaTaskReportScore(m_handle);
		//WORLD_GAME_AREA->AddTask(task);
		NF_SHARE_PTR<CAreaTaskItemBase> task = std::make_shared<CAreaTaskReportScore>(m_handle, NFMsg::MSGID_DBMGR_REPORT_SCORE_TODB);
		if (!task)
			return;

		m_pQsTaskScheduleModule->AddTask(task);

		LogInfo("CGameTable::StartReportScore", "handle=%d", m_handle);

		{
			uint32_t areaId = m_pGameServerNet_ServerModule->GetGameType();

			char openSeriesNum[100];
			uint64_t stamp = GetTimeStampInt64Ms();
			snprintf(openSeriesNum, sizeof(openSeriesNum), "%uA%dA%llu", areaId, m_handle, stamp);


			//这里值为0表示整桌游戏没有全部结束，为1代表结束
			int isVipRoomEnd = 0;

			int32_t gameRoomId = m_pConfigAreaModule->gameRoomId;
			int32_t isLockGame = 0;
			//             CPluto* pu = new CPluto;
			//             (*pu).Encode(MSGID_DBMGR_REPORT_SCORE) << task->GetTaskId() << areaId << gameRoomId << isLockGame \
  			//                 << m_tableNum << openSeriesNum << m_gameStartMsStamp \
  			//                 << m_betRule.baseBet << m_roundFee << m_vipRoomType << isVipRoomEnd;
			//             uint16_t len = (uint16_t)m_sitUserList.size();
			//             (*pu) << len;
			//             for(map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
			//             {
			//                 NF_SHARE_PTR<CTableUser> pTItem = iter->second;
			//                 int userId = pTItem->userInfo.baseInfo.userId;
			// 
			//                 if (pTItem->roundIncBean > 0)
			//                 {
			//                     int64_t tIncBean = pTItem->roundIncBean;
			//                     pTItem->roundIncBean = (int64_t)(pTItem->roundIncBean * (1000 - CONFIG_AREA->revenue) / 1000);
			//                     pTItem->roundRevenue = tIncBean - pTItem->roundIncBean;
			//                 }
			// 
			//                 (*pu) << userId << pTItem->roundIncBean << pTItem->roundRevenue << pTItem->totalBet << pTItem->totalBet;
			//             }
			//             
			//             LogInfo("CGameTable::StartReportScore", "userCnt = %u", len);
			// 
			//             (*pu) << EndPluto;
			//             mbDbmgr->PushPluto(pu);
			ProcUserScoreReportToDb(task->GetTaskId(), areaId, gameRoomId, isLockGame,
				m_tableNum, string(openSeriesNum), string(m_gameStartMsStamp), m_betRule.baseBet,
				m_roundFee, m_vipRoomType, isVipRoomEnd);
		}
	}
	else
	{
		LogError("CGameTable::StartReportScore", "!mbDbmgr");
	}
}


void NFGameTable::StartReportUserLog(NF_SHARE_PTR<CTableUser> pTUser)
{
	// 	// 未开始游戏直接退出不记录日志
	// 	if (pTUser->enterBean == 0)
	// 		return;
	// 
	// 	CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
	// 	//创建任务
	// 	CAreaTaskUserLog* task = new CAreaTaskUserLog();
	// 	WORLD_GAME_AREA->AddTask(task);
	// 
	// 	// 为了延续task，即使未连接，也要创建task
	// 	if (!mbDbmgr->IsConnected())
	// 	{
	// 		LogWarning("CGameTable::StartMatchLog", "!mbDbmgr->IsConnected()");
	// 	}
	// 	else
	// 	{
	// 		uint32_t areaId = WORLD_MGR->GetServer()->GetBindServerID();
	// 		int32_t gameRoomId = CONFIG_AREA->gameRoomId;
	// 		int32_t isLockGame = 0;
	// 
	// 		CPluto* pu = new CPluto;
	// 		(*pu).Encode(MSGID_DBMGR_USER_LOG) << task->GetTaskId() << areaId << gameRoomId << isLockGame << pTUser->userInfo.baseInfo.userId
	// 			<< m_betRule.index << m_handle << m_betRule.maxTotalBet << pTUser->enterBean << pTUser->leaveBean << pTUser->enterTime << pTUser->leaveTime;
	// 		(*pu) << EndPluto;
	// 		mbDbmgr->PushPluto(pu);
	//	}
}

void NFGameTable::StartReportMatchLog()
{
	// 	CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
	// 	//创建任务
	// 	CAreaTaskMatchLog* task = new CAreaTaskMatchLog();
	// 	WORLD_GAME_AREA->AddTask(task);
	// 
	// 	// 为了延续task，即使未连接，也要创建task
	// 	if (!mbDbmgr->IsConnected())
	// 	{
	// 		LogWarning("CGameTable::StartMatchLog", "!mbDbmgr->IsConnected()");
	// 	}
	// 	else
	// 	{
	// 		uint32_t areaId = WORLD_MGR->GetServer()->GetBindServerID();
	// 		int32_t gameRoomId = CONFIG_AREA->gameRoomId;
	// 		int32_t isLockGame = 0;
	// 
	// 		//         CPluto* pu = new CPluto;
	// 		//         (*pu).Encode(MSGID_DBMGR_MATCH_LOG) << task->GetTaskId() << areaId << gameRoomId << isLockGame;
	// 		// 
	// 		m_matchLog.handle = m_handle;
	// 		m_matchLog.selScore = m_betRule.maxTotalBet;
	// 		m_matchLog.tax = m_roundFee;
	// 		m_matchLog.typeId = GetGameType();
	// 
	// 		LogInfo("CGameTable::StartMatchLog", "!mbDbmgr->IsConnected()");
	// 		// 
	// 		//         m_matchLog.WriteToPluto(*pu);
	// 		// 
	// 		// 		(*pu) << m_winUserId;
	// 		//         (*pu) << EndPluto;
	// 		//         mbDbmgr->PushPluto(pu);
	// 		ProcMatchLogReportToDb(task->GetTaskId(), areaId, gameRoomId, isLockGame,
	// 			m_matchLog.matchId, m_matchLog.typeId, m_matchLog.userCount, m_matchLog.startTime,
	// 			m_matchLog.endTime, m_matchLog.handle, m_matchLog.selScore, m_matchLog.tax, m_matchLog.channelId.c_str(), m_winUserId);
	// 		m_matchLog.Clear();
	// 	}
}


void NFGameTable::StartGetGameControl()
{
	// 	CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
	// 	//创建任务
	// 	CAreaTaskGameControl* task = new CAreaTaskGameControl(m_handle);
	// 	WORLD_GAME_AREA->AddTask(task);
	// 
	// 	// 为了延续task，即使未连接，也要创建task
	// 	if (!mbDbmgr->IsConnected())
	// 	{
	// 		LogWarning("CGameTable::StartGetGameControl", "!mbDbmgr->IsConnected()");
	// 	}
	// 	else
	// 	{
	// 		int32_t gameRoomId = CONFIG_AREA->gameRoomId;
	// 
	// 		CPluto* pu = new CPluto;
	// 		(*pu).Encode(MSGID_DBMGR_GAME_CONTROL) << task->GetTaskId() << gameRoomId << EndPluto;
	// 		mbDbmgr->PushPluto(pu);
	// 	}
}

void NFGameTable::CalcWinUsers(map<int, int>& winUsers)
{
	// 防止235被其他比掉，特殊处理
	vector<int> v235Users;
	vector<int> v3OfAKindUsers;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTItem = iter->second;
		if (pTItem->CanBet())
		{
			int typeNum = pTItem->player.cardList.m_cardType.TypeNum;
			if (sctSpecial235 == typeNum)
				v235Users.push_back(pTItem->userInfo.baseInfo.userId);
			else if (sct3OfAKind == typeNum)
				v3OfAKindUsers.push_back(pTItem->userInfo.baseInfo.userId);
		}
	}

	if (v235Users.size() > 0 && v3OfAKindUsers.size() > 0)
	{
		for (int i = (int)v235Users.size() - 1; i >= 0; --i)
			winUsers[v235Users[i]] = 1;
	}
	else
	{
		NF_SHARE_PTR<CTableUser> pTLastMax = NULL;
		for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
		{
			NF_SHARE_PTR<CTableUser> pTItem = iter->second;
			if (pTItem->CanBet())
			{
				int userId = pTItem->userInfo.baseInfo.userId;
				if (0 == winUsers.size())
				{
					winUsers[userId] = 1;
					pTLastMax = pTItem;
				}
				else
				{
					int cmpValue = pTItem->player.cardList.m_cardType.CompareCardType(pTLastMax->player.cardList.m_cardType);
					if (0 == cmpValue)
					{
						winUsers[userId] = 1;
					}
					else if (cmpValue > 0)
					{
						winUsers.clear();
						winUsers[userId] = 1;
						pTLastMax = pTItem;
					}
				}
			}
		}
	}

	for (auto it = winUsers.begin(); it != winUsers.end(); ++it)
	{
		LogDebug("winUsers", "table=%d userId=%d", m_handle, it->first);
	}
}

int NFGameTable::GetVsCardUserId(int selfId)
{
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTItem = iter->second;
		if (pTItem->CanBet())
		{
			int userId = pTItem->userInfo.baseInfo.userId;
			if (userId != selfId && !InWinUsers(userId))
			{
				return userId;
			}
		}
	}

	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTItem = iter->second;
		if (pTItem->CanBet())
		{
			int userId = pTItem->userInfo.baseInfo.userId;
			if (userId != selfId && pTItem->bSeeCard)
			{
				return userId;
			}
		}
	}

	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTItem = iter->second;
		if (pTItem->CanBet())
		{
			int userId = pTItem->userInfo.baseInfo.userId;
			if (userId != selfId && !pTItem->bSeeCard)
			{
				return userId;
			}
		}
	}

	return INVALID_USERID;
}



void NFGameTable::DoRobotBet(NF_SHARE_PTR<CTableUser> pTUser)
{
	if (INVALID_TIME_COUNT == pTUser->robotInfo.CurTimeCount)
	{
		pTUser->robotInfo.CurTimeCount = 0;
		pTUser->robotInfo.ActionTime = GetActionTimeCountByCardCount(pTUser);
	}

	++pTUser->robotInfo.CurTimeCount;
	if (pTUser->robotInfo.CurTimeCount == pTUser->robotInfo.ActionTime)
	{
		// 开始bet
		{
			int betType = pTUser->robotInfo.betType;
			if (sbtSeeCard != betType)
			{
				DoUserBet(pTUser, pTUser->robotInfo.betType, pTUser->robotInfo.hideBetValue, pTUser->robotInfo.vsUserId);
			}
			else
			{
				DoUserSeeCard(pTUser);
			}
		}

		pTUser->robotInfo.OperationClear();
	}
}

int NFGameTable::GetActionTimeCountByCardCount(NF_SHARE_PTR<CTableUser> pTUser)
{
	bool LIsShow = (pTUser->bSeeCard == 1);
	int selfId = pTUser->userInfo.baseInfo.userId;

	int ExtraSeeCardRate = 0;
	int ExtraFollowRate = 0;
	int ExtraAddBetRate = 0;
	int ExtraVsCardRate = 0;
	int ExtraFoldCardRate = 0;

	if (InWinUsers(selfId))
	{
		// win
		int LRandom = NFUtil::GetRandomRange(0, 100 - 1);

		// 计算额外概率
		{

			//// 机器人赢时四轮及以上再比牌
			//int extraVsRound = GetRandomRange(3, 5);
			//if (m_betRound <= extraVsRound)
			//{
			//    LogInfo("##############", "[是最大]机器人赢时四轮及以上再比牌");
			//    ExtraVsCardRate -= CONFIG_AREA->WinCfgOperaRate[eotVsCard];
			//    ExtraFollowRate += CONFIG_AREA->WinCfgOperaRate[eotVsCard];
			//}
		}


		int LOperaType = ((NFCConfigAreaModule*)m_pConfigAreaModule)->GetChooseOpera(true, LRandom, ExtraSeeCardRate, ExtraFollowRate, ExtraAddBetRate, ExtraVsCardRate, ExtraFoldCardRate);
		if ((eotSeeCard == LOperaType) && LIsShow)
			LOperaType = eotAddBet;
		else if ((eotFoldCard == LOperaType) && !LIsShow)
			LOperaType = eotSeeCard;

		//不看牌最多跟注3轮
		if (!LIsShow && m_betRound >= 3)
		{
			LOperaType = eotSeeCard;
			//LogInfo("##############", "[是最大]不看牌最多跟注3轮");
		}

		//散牌最多跟注4轮
		if (pTUser->player.cardList.m_cardType.TypeNum == sctHighCard && m_betRound >= 4)
		{
			//LogInfo("##############", "[是最大]散牌最多跟注4轮");
			if (!LIsShow)
				LOperaType = eotSeeCard;
			else
				LOperaType = eotVsCard;
		}

		//对子最多跟注6轮
		if (pTUser->player.cardList.m_cardType.TypeNum == sct1Pair && m_betRound >= 6)
		{
			//LogInfo("##############", "[是最大]对子最多跟注6轮");
			if (!LIsShow)
				LOperaType = eotSeeCard;
			else
				LOperaType = eotVsCard;
		}

		if (eotFoldCard == LOperaType)
		{
			pTUser->robotInfo.betType = sbtFold;
			pTUser->robotInfo.hideBetValue = 0;
			pTUser->robotInfo.vsUserId = 0;
		}
		else if (eotSeeCard == LOperaType)
		{
			pTUser->robotInfo.betType = sbtSeeCard;
		}
		else if (eotFollow == LOperaType)
		{
			pTUser->robotInfo.betType = sbtCall;
			pTUser->robotInfo.hideBetValue = 0;
			pTUser->robotInfo.vsUserId = 0;
		}
		else if (eotAddBet == LOperaType)
		{
			try
			{
				if (m_isInPin)
					ThrowException(10, "血拼状态不允许加");

				bool bFind = false;
				int hideBetValue = 0;
				for (vector<int>::iterator it = m_betRule.raiseBet.begin(); it != m_betRule.raiseBet.end(); ++it)
				{
					if (*it > m_hideMinBet)
					{
						bFind = true;
						hideBetValue = *it;
						break;
					}
				}
				if (!bFind)
					ThrowException(11, "下注额度错误");
				if (hideBetValue <= m_hideMinBet)
					ThrowException(12, "加注要大于跟的额度");

				pTUser->robotInfo.betType = sbtRaise;
				pTUser->robotInfo.hideBetValue = hideBetValue;
				pTUser->robotInfo.vsUserId = 0;
			}
			catch (CException)
			{
				pTUser->robotInfo.betType = sbtCall;
				pTUser->robotInfo.hideBetValue = 0;
				pTUser->robotInfo.vsUserId = 0;
			}

		}
		else // vs or openCard
		{
			try
			{
				int vsUserId = GetVsCardUserId(selfId);
				if (m_isInPin)
					ThrowException(10, "血拼状态不允许比牌");
				if (!CanVs())
					ThrowException(11, "还没到比牌的轮数");
				// 不看也能比。
				if (vsUserId == selfId)
					ThrowException(12, "不能和自己比");
				NF_SHARE_PTR<CTableUser> pTVsWho = FindUserById(vsUserId);
				if (!pTVsWho)
					ThrowException(12, "玩家不存在");
				if (!pTVsWho->CanBet())
					ThrowException(12, "玩家不在游戏中");
				if (m_betRound < m_tableRule.pkRound + 1)
				{
					ThrowException(14, "还没到比牌轮数");
				}

				pTUser->robotInfo.betType = sbtVsCard;
				pTUser->robotInfo.hideBetValue = 0;
				pTUser->robotInfo.vsUserId = vsUserId;
			}
			catch (CException)
			{
				pTUser->robotInfo.betType = sbtCall;
				pTUser->robotInfo.hideBetValue = 0;
				pTUser->robotInfo.vsUserId = 0;
			}
		}
	}
	else
	{
		// lose
		int typeNum = pTUser->player.cardList.m_cardType.TypeNum;
		if (LIsShow && (sctSpecial235 == typeNum || sctHighCard == typeNum) && pTUser->robotInfo.lastAction != sbtSeeCard)
		{
			pTUser->robotInfo.betType = sbtFold;
			pTUser->robotInfo.hideBetValue = 0;
			pTUser->robotInfo.vsUserId = 0;
		}
		else
		{
			int LRandom = NFUtil::GetRandomRange(0, 100 - 1);

			// 计算额外概率
			{

			}

			int LOperaType = m_pConfigAreaModule->GetChooseOpera(false, LRandom, ExtraSeeCardRate, ExtraFollowRate, ExtraAddBetRate, ExtraVsCardRate, ExtraFoldCardRate);
			if ((eotSeeCard == LOperaType) && LIsShow)
				LOperaType = eotFollow;
			else if ((eotFoldCard == LOperaType || eotVsCard == LOperaType) && !LIsShow)
				LOperaType = eotSeeCard;

			//不看牌最多跟注3轮
			if (!LIsShow && m_betRound >= 3)
			{
				LOperaType = eotSeeCard;
				//LogInfo("##############", "[不是最大]不看牌最多跟注3轮");
			}

			if (eotFoldCard == LOperaType)
			{
				pTUser->robotInfo.betType = sbtFold;
				pTUser->robotInfo.hideBetValue = 0;
				pTUser->robotInfo.vsUserId = 0;
			}
			else if (eotSeeCard == LOperaType)
			{
				pTUser->robotInfo.betType = sbtSeeCard;
			}
			else if (eotFollow == LOperaType)
			{
				pTUser->robotInfo.betType = sbtCall;
				pTUser->robotInfo.hideBetValue = 0;
				pTUser->robotInfo.vsUserId = 0;
			}
			else if (eotAddBet == LOperaType)
			{
				try
				{
					if (m_isInPin)
						ThrowException(10, "血拼状态不允许加");

					bool bFind = false;
					int hideBetValue = 0;
					for (vector<int>::iterator it = m_betRule.raiseBet.begin(); it != m_betRule.raiseBet.end(); ++it)
					{
						if (*it > m_hideMinBet)
						{
							bFind = true;
							hideBetValue = *it;
							break;
						}
					}
					if (!bFind)
						ThrowException(11, "下注额度错误");
					if (hideBetValue <= m_hideMinBet)
						ThrowException(12, "加注要大于跟的额度");

					pTUser->robotInfo.betType = sbtRaise;
					pTUser->robotInfo.hideBetValue = hideBetValue;
					pTUser->robotInfo.vsUserId = 0;
				}
				catch (CException)
				{
					pTUser->robotInfo.betType = sbtCall;
					pTUser->robotInfo.hideBetValue = 0;
					pTUser->robotInfo.vsUserId = 0;
				}

			}
			else // vs or openCard
			{
				try
				{
					int vsUserId = GetVsCardUserId(selfId);
					if (m_isInPin)
						ThrowException(10, "血拼状态不允许比牌");
					if (!CanVs())
						ThrowException(11, "还没到比牌的轮数");
					// 不看也能比。
					if (vsUserId == selfId)
						ThrowException(12, "不能和自己比");
					NF_SHARE_PTR<CTableUser> pTVsWho = FindUserById(vsUserId);
					if (!pTVsWho)
						ThrowException(12, "玩家不存在");
					if (!pTVsWho->CanBet())
						ThrowException(12, "玩家不在游戏中");
					if (m_betRound < m_tableRule.pkRound + 1)
					{
						ThrowException(14, "还没到比牌轮数");
					}

					pTUser->robotInfo.betType = sbtVsCard;
					pTUser->robotInfo.hideBetValue = 0;
					pTUser->robotInfo.vsUserId = vsUserId;
				}
				catch (CException)
				{
					pTUser->robotInfo.betType = sbtCall;
					pTUser->robotInfo.hideBetValue = 0;
					pTUser->robotInfo.vsUserId = 0;
				}
			}
		}
	}

	// 刚比过牌不弃牌
	if (pTUser->robotInfo.lastAction == sbtVsCard && pTUser->robotInfo.betType == sbtFold)
	{
		//LogInfo("##############", "刚比过牌不弃牌!!!!");
		pTUser->robotInfo.betType = sbtCall;
		pTUser->robotInfo.hideBetValue = 0;
		pTUser->robotInfo.vsUserId = 0;
	}

	//LogInfo("##############", "操作为 [%d, %d]", pTUser->robotInfo.lastAction, pTUser->robotInfo.betType);
	pTUser->robotInfo.lastAction = pTUser->robotInfo.betType;

	int maxTime = (int)((m_decTimeCount - 2) / 2);
	int Result = NFUtil::GetRandomRange(0, maxTime);

	if (Result >= 4)
		Result = 4;
	if (Result < 2)
		Result = 2;

	return Result;
}

void NFGameTable::SendTableStateNotify()
{
	// 	CPluto* pu = new CPluto();
	// 	(*pu).Encode(MSGID_CLIENT_TSTATE_CHANGE_NOTIFY) << m_tstate << EndPluto;
	// 	SendBroadTablePluto(pu, INVALID_USERID);
}

void NFGameTable::SendUserStateNotify(int userId, int tuserState, int64_t& bean)
{
	// 	CPluto* pu = new CPluto();
	// 	(*pu).Encode(MSGID_CLIENT_OTHER_STATE_NOTIFY) << userId << tuserState << bean << EndPluto;
	// 	SendBroadTablePluto(pu, userId);
}

void NFGameTable::SendGameResultNotify()
{
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		// 上报失败也记录分数
		int64_t incZSCore = pTUser->roundIncBean;
		AddUserTotalScore(pTUser->userInfo.baseInfo.userId, incZSCore);
		//LogInfo("CGameTable::SendGameResultNotify", "userId = %d, incZscore = %lld", pTUser->userInfo.baseInfo.userId, incZSCore);
	}

	int maxLen = (int)m_userList.size();
	for (int i = 0; i < maxLen; ++i)
	{
		NF_SHARE_PTR<CTableUser> pTSendTo = m_userList[i];
		int selfId = pTSendTo->userInfo.baseInfo.userId;
		if (tusNomal == pTSendTo->ustate)
		{
			// 			CPluto* pu = new CPluto();
			// 			(*pu).Encode(MSGID_CLIENT_G_RESULT_NOTIFY) << m_resultType;
			// 			uint16_t len = (uint16_t)m_sitUserList.size();
			// 			(*pu) << len;
			// 			for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
			// 			{
			// 				NF_SHARE_PTR<CTableUser> pTUser = iter->second;
			// 				int dataId = pTUser->userInfo.baseInfo.userId;
			// 
			// 				(*pu) << pTUser->userInfo.baseInfo.userId << pTUser->userInfo.baseInfo.level << pTUser->userInfo.baseInfo.score
			// 					<< pTUser->userInfo.baseInfo.bean << pTUser->roundIncBean << GetUserTotalScore(pTUser->userInfo.baseInfo.userId) << "{}";
			// 				bool showCard = false;
			// 				if (pTSendTo->isRobot == false)
			// 				{
			// 					// 自己的牌可见， 和自己vs过的牌可见, 开牌玩家可见
			// 					showCard = (selfId == dataId) || (pTSendTo->vsUserIds.find(dataId) != pTSendTo->vsUserIds.end());
			// 				}
			// 				if (showCard)
			// 				{
			// 					pTUser->player.cardList.GetGameCard()->WriteToPluto(*pu);
			// 					(*pu) << pTUser->player.cardList.m_cardType.TypeNum;
			// 				}
			// 				else
			// 				{
			// 					(*pu) << (uint16_t)0 << (int32_t)sctNone;
			// 				}
			// 				(*pu) << pTUser->loseRound << pTUser->winRound << pTUser->hustraightFlushCount << pTUser->sct3OfAKindCount;
			// 			}
			// 			(*pu) << EndPluto;
			// 			SendPlutoToUser(pu, pTSendTo);
		}
	}
}

int NFGameTable::CanPin()
{
	return (m_hasPin && m_betRound >= m_pConfigAreaModule->pin_bet_round);
}
int NFGameTable::CanVs()
{
	return (m_betRound >= m_pConfigAreaModule->vs_bet_round);
}

bool NFGameTable::CheckHaveMenCard()
{
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (pTUser->bSeeCard == 0)
		{
			return true;
		}
	}
	return false;
}

void NFGameTable::collectTypeNum()
{
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (pTUser->player.cardList.m_cardType.TypeNum == sctStraightFlush)
		{
			pTUser->hustraightFlushCount++;
		}
		else if (pTUser->player.cardList.m_cardType.TypeNum == sct3OfAKind)
		{
			pTUser->sct3OfAKindCount++;
		}
	}
}

int NFGameTable::NormalCardStatNum()
{
	int norMum = 0;
	for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<CTableUser> pTUser = iter->second;
		if (pTUser->cardState != ucsFold)
		{
			norMum++;
		}
	}
	return norMum;
}

void NFGameTable::UnlockOrlockUser(NF_SHARE_PTR<CTableUser> pTUser, int unlockOrlock)
{

	if (m_pNFIGameServerToDBModule)
	{
		//创建任务
		NF_SHARE_PTR<CAreaTaskItemBase> readUserItask = std::make_shared<CAreaTaskLockOrUnlockUser>(unlockOrlock, pTUser->userInfo.baseInfo.userId);
		m_pQsTaskScheduleModule->AddTask(readUserItask);

		int32_t gameRoomId = m_pConfigAreaModule->gameRoomId;
		int32_t userId = pTUser->userInfo.baseInfo.userId;
		int32_t type = unlockOrlock;
		ProcUserLockGameRoomToDb(readUserItask->GetTaskId(), gameRoomId, userId, type);

	}
	else
	{
		LogError("CGameTable::UnlockUser", "!mbDbmgr");
	}

}


int NFGameTable::GetGameType()
{
	if (m_betRule.maxTotalBet == 10000)
		return 1;
	else if (m_betRule.maxTotalBet == 20000)
		return 2;
	else if (m_betRule.maxTotalBet == 50000)
		return 3;
	else if (m_betRule.maxTotalBet == 100000)
		return 4;

	return 0;
}

//////////////////////////////////////////////////////////////////////////
static NFCTableManagerModule* gStaticGameTableMgr = NULL;

NFCTableManagerModule::NFCTableManagerModule(NFIPluginManager* p) : m_tablelist(), m_activeTableCount(0), m_sitAllUserList(), m_userId2CreateTable(), m_arrangeTableTime(), m_graph(), m_arrangeTableLen(0)
{
	pPluginManager = p;

}

NFCTableManagerModule::~NFCTableManagerModule()
{
//		CLEAR_POINTER_MAP(m_arrangeUserMap);
//		CLEAR_POINTER_CONTAINER(m_tablelist);
	for (int i = 0; i < MAX_FILE_HANDLE_COUNT; i++)
	{
		delete m_arrangeTableAry[i];
	}
	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		auto tem = m_tablelist.back();
		//delete tem->;
		m_tablelist.pop_back();
		//m_tablelist.push_back(new NFGameTable(i));
	}
}

bool NFCTableManagerModule::Awake()
{

	return true;
}
bool NFCTableManagerModule::Execute()
{
	for (auto tableItem : m_tablelist)
	{
		if (tableItem->GetTablePassTick() > 1000)
		{
			tableItem->AddTableMsTick(1000);
			tableItem->Update(1);
		}
		tableItem->FixedUpdate(0);
	}
	RunTime();
	return true;
}
bool NFCTableManagerModule::Shut()
{
	return true;
}
bool NFCTableManagerModule::Init()
{
	m_tablelist.reserve(MAX_TABLE_HANDLE_COUNT);
	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		m_tablelist.push_back(NF_SHARE_PTR<NFGameTable>(NF_NEW NFGameTable(i, this)));
		//m_tablelist.push_back(new NFGameTable(i));
	}
	for (int i = 0; i < MAX_FILE_HANDLE_COUNT; i++)
	{
		m_arrangeTableAry[i] = new SArrangeTableItem();
	}
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();
	//m_pWorldGameAreaModule = pPluginManager->FindModule<NFIWorldGameAreaModule>();
	//m_pNetCommonModule = pPluginManager->FindModule<NFINetCommonModule>();
	m_pGameLogicModule = pPluginManager->FindModule<NFIGameLogicModule>();
	m_pRobotModule = pPluginManager->FindModule<NFIRobotModule>();
	m_pConfigAreaModule = pPluginManager->FindModule<NFIConfigAreaModule>();
	m_pTableManagerModule = pPluginManager->FindModule<NFITableManagerModule>();

	m_pNetClientModule = pPluginManager->FindModule<NFINetClientModule>();

	m_pQsTaskScheduleModule = pPluginManager->FindModule<NFIQsTaskScheduleModule>();


	return true;
}
bool NFCTableManagerModule::AfterInit()
{
	//m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFGamespace::EG_BACCARAT, NFMsg::MSGID_AREA_READ_USERINFO_CALLBACK, this, &NFCTableManagerModule::OnMsGidReadUserInfoCallback);

	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, NFMsg::MSGID_AREA_READ_USERINFO_CALLBACK, this, &NFCTableManagerModule::OnNodeDbGidReadUserInfoCallback); //o
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, NFMsg::MSGID_AREA_REPORT_SCORE_CALLBACK, this, &NFCTableManagerModule::OnMsGidScoreReportCallback);
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, NFMsg::MSGID_AREA_GAME_CONTROL_CALLBACK, this, &NFCTableManagerModule::OnMsGidGameControlCallback);//o
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, NFMsg::MSGID_AREA_MATCH_REPORT_CALLBACK, this, &NFCTableManagerModule::OnMsGidMatchReportCallback);//o
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, NFMsg::MSGID_AREA_LOCK_GAMEROOM_CALLBACK, this, &NFCTableManagerModule::OnMsGidLockGameRoomCallback);//o


	return true;
}

void NFCTableManagerModule::RunTime()
{
	if (m_arrangeTableTime.GetPassMsTick() >= 997)
	{
		ArrangeTableForRealUser();
	}

	m_activeTableCount = 0;
	for (vector<NF_SHARE_PTR<NFGameTable>>::iterator iter = m_tablelist.begin(); iter != m_tablelist.end(); ++iter)
	{
		if ((*iter)->CheckRunTime())
			m_activeTableCount++;
	}
}

void NFCTableManagerModule::OnUserOffline(int userId)
{
	int chairIndex = 0;
	NF_SHARE_PTR<NFGameTable> pTable = GetUserTable(userId, chairIndex);
	if (!pTable)
		return;

	pTable->LeaveTable(userId);
}

bool NFCTableManagerModule::FindSitUser(int userId, int& retTableHandle, int&retChairIndex)
{
	map<int, uint32_t>::iterator iter = m_sitAllUserList.find(userId);
	if (m_sitAllUserList.end() == iter)
		return false;

	uint32_t value = iter->second;
	retTableHandle = value & 0xFFFF;
	retChairIndex = (value >> 16) & 0xFFFF;

	return true;
}

void NFCTableManagerModule::ArrangeTableForNearUser()
{
	for (list<int>::iterator itNode = m_pConfigAreaModule->near_node.begin(); itNode != m_pConfigAreaModule->near_node.end(); ++itNode)
	{
		if (m_pGameLogicModule->GetYaoyaoUserListIgnoreSelScore(m_graph.m_userList) < MIN_TABLE_USER_COUNT)
			return;
		m_graph.UpdateGraph(*itNode);
		if (m_graph.NearList.size() > 0)
		{
			if (!UpdateCanArrangeTableList())
				return;
			for (list<SArrangeUserList*>::iterator itUser = m_graph.NearList.begin(); itUser != m_graph.NearList.end(); ++itUser)
			{
				SArrangeUserList* pItemUser = *itUser;
				{
					// 先看距离比较近是否够1桌，够就分下
					int userNumPerTable = MAX_TABLE_USER_COUNT;
					if (pItemUser->len < userNumPerTable)
						userNumPerTable = pItemUser->len;
					int arrangedTables = pItemUser->len / userNumPerTable;
					// 必须倒着循环UserIndex，AddUserToArrange会删除User
					int userIndex = pItemUser->len - 1;
					for (int i = 0; i < arrangedTables; i++)
					{
						// 都是空桌，不需要判断user的ignoreTable了
						SArrangeTableItem* pItem = FindEmptyTable(pItemUser->selScore, userNumPerTable, MAX_TABLE_USER_COUNT, -1, NULL);
						if (!pItem)
						{
							LogWarning("CGameTableMgr::ArrangeTableForNearUser", "FindEmptyTable failed");
							break;
						}
						for (int chairIndex = 0; chairIndex < userNumPerTable; chairIndex++)
						{
							AddUserToArrange(pItemUser, pItem, userIndex, pItemUser->UserAry[userIndex]);
							--userIndex;
						}
					}
				}
			}
			EnterTableForArrangeUser();
		}
	}
}

void NFCTableManagerModule::ArrangeTableForRealUser()
{
	 m_arrangeTableTime.SetNowTime();
	 {
	 	//  不同规则的人不在一起 
	 	// ArrangeTableForNearUser();
	 
	 	// 获取可以分配的用户列表
	 	if (m_pGameLogicModule->GetYaoyaoUserList(m_arrangeUserMap) < 1)
	 	{
	 		return;
	 	}
	 	//LogInfo("CGameTableMgr::ArrangeTableForRealUser", "开始分配");
	 	// 随机下列表
	 	for (map<int, SArrangeUserList*>::iterator it = m_arrangeUserMap.begin(); it != m_arrangeUserMap.end(); ++it)
	 	{
	 		SArrangeUserList* itemMap = it->second;
	 		for (int i = 0; i < itemMap->len; i++)
	 		{
	 			int index = rand() % itemMap->len;
	 			if (index != i)
	 			{
					NF_SHARE_PTR <SUserInfo> tmp = itemMap->UserAry[i];
	 				itemMap->UserAry[i] = itemMap->UserAry[index];
	 				itemMap->UserAry[index] = tmp;
	 			}
	 		}
	 	}
	 }
	 if (!UpdateCanArrangeTableList())
	 {
	 	LogInfo("CGameTableMgr::ArrangeTableForRealUser", "没找到可用的桌子");
	 	return;
	 }
	 int zhaojianIndex = 0;
	 for (map<int, SArrangeUserList*>::iterator it = m_arrangeUserMap.begin(); it != m_arrangeUserMap.end(); ++it, ++zhaojianIndex)
	 {
	 	SArrangeUserList* itemMap = it->second;
	 	{
	 		// 如果超过补人时间, 随机选1个有人桌去补人
	 		uint32_t maxWait = m_pConfigAreaModule->fillchair_sec * MS_PER_SEC;
	 		int userCount = itemMap->SortIfTimeout(maxWait);
	 		if (userCount > 0)
	 		{
	 			// 必须倒着循环UserIndex，AddUserToArrange会删除User
	 			for (int i = userCount - 1; i >= 0; --i)
	 			{
					NF_SHARE_PTR <SUserInfo> pUser = itemMap->UserAry[i];
	 
	 				// 查找有人的桌子
	 				SArrangeTableItem* pItem = FindEmptyTable(itemMap->selScore, 1, MAX_TABLE_USER_COUNT - 1, pUser->activeInfo.lastSitTableHandle, pUser);
	 				// 单人失败并不代表其他人也失败，因为有ignoreTable
	 				if (pItem)
	 				{
	 					AddUserToArrange(itemMap, pItem, i, pUser);
	 				}
	 			}
	 		}
	 
	 		//estimate_ip为0时，不考虑IP段相同不能一桌的情况，否则考虑IP段相同不能一桌的情况
	 		if (0 == m_pConfigAreaModule->estimate_ip)
	 		{
	 			// 如果人还剩余够1桌 分配新桌
	 			userCount = itemMap->SortIfTimeout(maxWait);
	 			if (userCount >= MIN_TABLE_USER_COUNT)
	 			{
	 				int userNumPerTable = MAX_TABLE_USER_COUNT;
	 				if (userCount < userNumPerTable)
	 					userNumPerTable = userCount;
	 				int arrangedTables = userCount / userNumPerTable;
	 				// 必须倒着循环UserIndex，AddUserToArrange会删除User
	 				int userIndex = userCount - 1;
	 				for (int i = 0; i < arrangedTables; i++)
	 				{
	 					// 都是空桌，不需要判断user的ignoreTable了
	 					SArrangeTableItem* pItem = FindEmptyTable(itemMap->selScore, userNumPerTable, MAX_TABLE_USER_COUNT, -1, NULL);
	 					if (!pItem)
	 					{
	 						LogWarning("CGameTableMgr::ArrangeTableForRealUser", "FindEmptyTable failed");
	 						break;
	 					}
	 					for (int chairIndex = 0; chairIndex < userNumPerTable; chairIndex++)
	 					{
	 						AddUserToArrange(itemMap, pItem, userIndex, itemMap->UserAry[userIndex]);
	 						--userIndex;
	 					}
	 				}
	 			}
	 		}
	 		else
	 		{
	 			////zhaojian test 如果剩余的人还够一桌，那么就分配给他们空桌，如果是相同IP不分配在同一张桌子上
	 			userCount = itemMap->SortIfTimeout(maxWait);
	 			if (userCount >= MIN_TABLE_USER_COUNT)
	 			{
	 				int userNumPerTable = MAX_TABLE_USER_COUNT;
	 				if (userCount < userNumPerTable)
	 					userNumPerTable = userCount;
	 				int arrangedTables = userCount / userNumPerTable;
	 				// 必须倒着循环UserIndex，AddUserToArrange会删除User
	 				int userIndex = userCount - 1;
	 				for (int tmpTableIndex = 0; tmpTableIndex < arrangedTables;)
	 				{
	 					// 都是空桌，不需要判断user的ignoreTable了
	 					SArrangeTableItem* pItem = FindEmptyTable(itemMap->selScore, userNumPerTable, MAX_TABLE_USER_COUNT, -1, NULL);
	 					if (!pItem)
	 					{
	 						LogWarning("CGameTableMgr::ArrangeTableForRealUser", "FindEmptyTable failed");
	 						break;
	 					}
	 					int arrangeUserCount = 0;
	 					vector<int> userIndexArray;
	 					userIndexArray.clear();
	 					for (int i = userIndex; i >= userNumPerTable - 1; --i)
	 					{
	 						for (int j = i - 1; j >= 0; --j)
	 						{
	 							if (CanSitInSameTable(itemMap->UserAry[i], itemMap->UserAry[j]))  //todo:ip段和时间的判断
	 							{
	 								++arrangeUserCount;
	 								userIndexArray.push_back(j);
	 								if (arrangeUserCount == userNumPerTable - 1)
	 								{
	 									break;
	 								}
	 							}
	 						}
	 						if (arrangeUserCount == userNumPerTable - 1)
	 						{
	 							userIndexArray.insert(userIndexArray.begin(), i);
	 							break;
	 						}
	 					}
	 					if (userNumPerTable - 1 == arrangeUserCount)
	 					{
	 						for (int chairIndex = 0; chairIndex < userNumPerTable; chairIndex++)
	 						{
	 							AddUserToArrange(itemMap, pItem, userIndexArray[chairIndex], itemMap->UserAry[userIndexArray[chairIndex]]);
	 							--userIndex;
	 						}
	 						++tmpTableIndex;
	 					}
	 					else
	 					{
	 						--userIndex;
	 					}
	 					if (userIndex < userNumPerTable)
	 					{
	 						break;
	 					}
	 				}
	 			}
	 		}
	 	}
	 	{
	 		// 如果超过加机器人时间 真人会和机器人游戏，随机加min-max机器人
	 		int curTick = GetNowMsTick();
	 		// 必须倒着循环UserIndex，AddUserToArrange会删除User
	 		for (int i = itemMap->len - 1; i >= 0; --i)
	 		{
				NF_SHARE_PTR <SUserInfo> pUser = itemMap->UserAry[i];
	 			if (pUser->activeInfo.isAddRobot && (GetMsTickDiff(pUser->activeInfo.yaoyaoTick, curTick) >= pUser->activeInfo.addRobotMs))
	 			{
	 				SArrangeTableItem* pItem = FindEmptyTable(itemMap->selScore, MAX_TABLE_USER_COUNT, MAX_TABLE_USER_COUNT, -1, NULL);
	 				if (!pItem)
	 				{
	 					LogWarning("CGameTableMgr::ArrangeTableForRealUser", "addRobot FindEmptyTable failed");
	 					break;
	 				}
	 
	 				AddUserToArrange(itemMap, pItem, i, pUser);
	 				int robotCount = NFUtil::GetRandomRange(MIN_TABLE_USER_COUNT, MAX_TABLE_USER_COUNT) - 1;
	 				for (int j = 0; j < robotCount; ++j)
	 				{
	 					AddUserToArrange(itemMap, pItem, -1, NULL);
	 				}
	 			}
	 		}
	 	}
	 }
	 
	 EnterTableForArrangeUser();
	 for (map<int, SArrangeUserList*>::iterator it = m_arrangeUserMap.begin(); it != m_arrangeUserMap.end(); ++it)
	 {
	 	SArrangeUserList* itemMap = it->second;
	 	{
	 		// 如果超过最大等待时间 通知超时
	 		uint32_t curTick = GetNowMsTick();
	 		uint32_t maxTick = m_pConfigAreaModule->realuser_max_waitsec * MS_PER_SEC;
	 		for (int i = 0; i < itemMap->len; ++i)
	 		{
				NF_SHARE_PTR <SUserInfo> pUser = itemMap->UserAry[i];
	 			if (GetMsTickDiff(pUser->activeInfo.yaoyaoTick, curTick) >= maxTick)
	 			{

					nlohmann::json jsonSend;

					jsonSend["action"] = NFMsg::MSGID_CLIENT_YAOYAO_TIMEOUT_NOTIFY;
				
					m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_YAOYAO_TIMEOUT_NOTIFY, jsonSend, pUser->self);
					//SendMsgToClient(NFMsg::MSGID_CLIENT_YAOYAO_TIMEOUT_NOTIFY, jsonSend, pUser->self);
// 	 				CMailBox* mb = WORLD_MGR->GetServer()->GetMailboxByFd(pUser->activeInfo.fd);
// 	 				if (mb)
// 	 				{
// 	 					CPluto* pu = new CPluto();
// 	 					(*pu).Encode(MSGID_CLIENT_YAOYAO_TIMEOUT_NOTIFY) << EndPluto;
// 	 					mb->PushPluto(pu);
// 	 				}
// 	 				else
// 	 				{
// 	 					LogError("CGameTableMgr::ArrangeTableForRealUser", "find mb failed");
// 	 				}
	 
	 				pUser->activeInfo.userState = EUS_AUTHED;
	 			}
	 		}
	 	}
	 }
}

bool NFCTableManagerModule::UpdateCanArrangeTableList()
{
	m_arrangeTableLen = 0;
	for (vector<NF_SHARE_PTR<NFGameTable>>::iterator iter = m_tablelist.begin(); iter != m_tablelist.end(); ++iter)
	{
		NF_SHARE_PTR<NFGameTable> item = *iter;
		SArrangeTableItem* curTb = m_arrangeTableAry[m_arrangeTableLen];

		item->ClearRobotUserIfNoRealUser();             // 防止机器人影响底分或影响新机器人替换
		if (item->IsGaming())
			curTb->curUser = item->GetCurUserCount();
		else
			curTb->curUser = item->GetRealUserCount();  // 为了替换机器人，这里换成GetRealUserCount
		if (curTb->curUser < MAX_TABLE_USER_COUNT)
		{
			if ((!item->IsGaming()) || (item->IsGaming() && GAME_CAN_ENTER_WHEN_GAMING))
			{
				curTb->canSitMaxUser = MAX_TABLE_USER_COUNT - curTb->curUser;
				curTb->arrangeLen = 0;
				curTb->tableHandle = item->GetHandle();
				if (item->GetCurUserCount() > 0)
					curTb->selScore = item->GetCurBaseScore();
				else
					curTb->selScore = -1;

				m_arrangeTableLen++;
			}
		}
	}

	if (m_arrangeTableLen < 1)
		return false;
	else
		return true;
}

void NFCTableManagerModule::EnterTableForArrangeUser()
{
	 // 补人情况下，不足开桌人数的，也加入游戏，机器人会自动补全人数。
	 for (int i = 0; i < m_arrangeTableLen; ++i)
	 {
	 	SArrangeTableItem* pItem = m_arrangeTableAry[i];
	 	if (pItem->arrangeLen > 0)
	 	{
			NF_SHARE_PTR<NFGameTable> pTable = m_tablelist[pItem->tableHandle];
	 		int emptyCount = pTable->GetEmptyChairAry(m_emptyChairAry);
	 		if (pItem->arrangeLen > emptyCount)
	 		{
	 			LogError("CGameTableMgr::EnterTableForArrangeUser", "pItem->arrangeLen > emptyCount");
	 			break;
	 		}
	 
	 		for (int j = 0; j < pItem->arrangeLen; ++j)
	 		{
				NF_SHARE_PTR <SUserInfo> pUser = pItem->arrangeUserAry[j];
	 			bool isRobot = (NULL == pUser);
	 			if (isRobot)
	 			{
	 				pUser = m_pRobotModule->AllocRobotUser(pItem->selScore);
	 				if (!pUser)
	 					break;
	 			}
	 			if (!pTable->EnterTable(pUser, m_emptyChairAry[j], isRobot))
	 			{
	 				// 加入失败需要释放robot
	 				if (isRobot)
						m_pRobotModule->FreeRobotUser(pUser->baseInfo.userId);
	 			}
	 			else
	 			{
	 				// 发送开始游戏、用户列表、可能需要完整信息
	 				pTable->SendStartGameNotify(m_emptyChairAry[j]);
	 			}
	 		}
	 	}
	 }
}

SArrangeTableItem* NFCTableManagerModule::FindEmptyTable(int selScore, int minEmptyChair, int maxEmptyChair, int ignoreHandle, NF_SHARE_PTR <SUserInfo> pUser)
{
	for (int i = 0; i < m_arrangeTableLen; i++)
	{
		SArrangeTableItem* item = m_arrangeTableAry[i];

		if (item->canSitMaxUser >= minEmptyChair && item->canSitMaxUser <= maxEmptyChair)
		{
			if (item->tableHandle != ignoreHandle)
			{
				if (item->selScore < 0 || selScore == item->selScore)
				{
					if (pUser)
					{
						int j = item->curUser;
						for (; j >= 0; --j)
						{
							NF_SHARE_PTR < SUserInfo> tableUser = item->arrangeUserAry[j];
							if (tableUser)//不为null，表示是真人不是机器人
							{
								if (0 != m_pConfigAreaModule->estimate_ip)
								{
									if (!CanSitInSameTable(item->arrangeUserAry[j], pUser))
									{
										break;
									}
								}
							}
						}
						if (j < 0)
						{
							return item;
						}
					}
					else
					{
						return item;
					}
				}
			}
		}
	}

	return NULL;
}

void NFCTableManagerModule::AddUserToArrange(SArrangeUserList* pUserList, SArrangeTableItem* pItem, int delUserIndex, NF_SHARE_PTR <SUserInfo> pUser)
{
	m_pLogModule->LogInfo("NFCTableManagerModule::AddUserToArrange----------------");
	// 对于机器人 XUser为nil   delUserIndex=-1表示不删除FArrangeUserAry中的user
	if (pItem->arrangeLen >= (MAX_TABLE_USER_COUNT - pItem->curUser))
	{
		LogError("CGameTableMgr::AddUserToArrange", "no chair");
		return;
	}

	// 以第一个入座的人selScore为准
	if (pItem->selScore < 0)
		pItem->selScore = pUserList->selScore;
	pItem->arrangeUserAry[pItem->arrangeLen] = pUser;
	--pItem->canSitMaxUser;
	if (delUserIndex >= 0)
	{
		for (int i = delUserIndex; i < pUserList->len - 1; i++)
		{
			pUserList->UserAry[i] = pUserList->UserAry[i + 1];
		}
		--pUserList->len;
	}

	pItem->arrangeLen++;
}

bool NFCTableManagerModule::CanSitInSameTable(NF_SHARE_PTR <SUserInfo> pUser1, NF_SHARE_PTR <SUserInfo> pUser2)
{
	//LogInfo("CGameTableMgr::CanSitInSameTable", "run into");
	if (pUser1 && pUser2)
	{
		// 		CMailBox* mb1 = WORLD_MGR->GetServer()->GetMailboxByFd(pUser1->activeInfo.fd);
		// 		CMailBox* mb2 = WORLD_MGR->GetServer()->GetMailboxByFd(pUser2->activeInfo.fd);
		// 		if (mb1 && mb2)
		// 		{
		// 			long ip1 = inet_addr(mb1->GetIp().c_str()) & 0xffffff;
		// 			long ip2 = inet_addr(mb2->GetIp().c_str()) & 0xffffff;
		// 			//LogInfo("CGameTableMgr::CanSitInSameTable", "ip1 = %lld, ip2 = %lld, ip1.str = %s. ip2.str = %s", ip1, ip2, mb1->GetIp().c_str(), mb2->GetIp().c_str());
		// 			if (ip1 == ip2)
		// 			{
		// 				//LogInfo("CGameTableMgr::CanSitInSameTable", "return false");
		// 				return false;
		// 			}
		// 		}
		// 		else
		// 		{
		// 			LogInfo("CGameTableMgr::CanSitInSameTable", "cannot find mb");
		// 		}
	}
	return true;
}

bool NFCTableManagerModule::AddSitUser(int userId, int tableHandle, int chairIndex)
{
	map<int, uint32_t>::iterator iter = m_sitAllUserList.find(userId);
	if (m_sitAllUserList.end() != iter)
	{
		LogError("CGameTableMgr::AddSitUser", "user exists");
		return false;
	}

	uint32_t value = (tableHandle & 0xFFFF) | (chairIndex << 16);
	m_sitAllUserList.insert(make_pair(userId, value));

	return true;
}

bool NFCTableManagerModule::RemoveSitUser(int userId)
{
	map<int, uint32_t>::iterator iter = m_sitAllUserList.find(userId);
	if (m_sitAllUserList.end() == iter)
	{
		LogError("CGameTableMgr::RemoveSitUser", "user not exists");
		return false;
	}

	// map erase iter 效率最高
	m_sitAllUserList.erase(iter);
	return true;
}

bool NFCTableManagerModule::RemoveUserCreateTable(int userId)
{
	map<int, int>::iterator iter = m_userId2CreateTable.find(userId);
	if (m_userId2CreateTable.end() == iter)
	{
		return false;
	}

	m_userId2CreateTable.erase(iter);

	LogInfo("CGameTableMgr::RemoveUserCreateTable", "userId=%d", userId);

	return true;
}

NF_SHARE_PTR<NFGameTable> NFCTableManagerModule::GetUserTable(int userId, int& chairIndex)
{
	int tableHandle = 0;
	if (FindSitUser(userId, tableHandle, chairIndex))
		return m_tablelist[tableHandle];
	else
		return NULL;
}

NF_SHARE_PTR<NFGameTable> NFCTableManagerModule::GetPUserTable(const SUserInfo& pUser, int& chairIndex)
{
	int tableHandle = pUser.activeInfo.tableHandle;
	chairIndex = pUser.activeInfo.chairIndex;

	if (tableHandle >= 0)
		return m_tablelist[tableHandle];
	else
		return NULL;
}

CTableUser::CTableUser()
{
	Clear();
}

CTableUser::~CTableUser()
{

}

void CTableUser::Clear()
{
	ustate = tusNone;
	isReady = false;
	totalBet = 0;
	enterBean = 0;
	leaveBean = 0;
	enterTime = 0;
	leaveTime = 0;
	cardState = ucsNormal;
	bSeeCard = 0;
	agreeEnd = clearInit;
	isTrust = false;
	isRobot = true;
	manualTrust = false;
	readyOrLeaveTick = GetNowMsTick();
	fillOrLeaveRobotMs = NFUtil::GetRandomRange(1 * 1000, 2 * 1000);
	userInfo.Clear();
	player.Clear();
	robotInfo.Clear();
	vsUserIds.clear();
	hasDoBet = false;
	roundIncBean = 0;
	roundRevenue = 0;
	winRound = 0;
	hustraightFlushCount = 0;
	sct3OfAKindCount = 0;
	betAry.clear();
	IsMenPai = false;
	isAbnormal = false;
	totalBet = 0;
}

bool CTableUser::CanBet()
{
	return (ustate >= tusNomal && ucsNormal == cardState);
}

TRobotInfo::TRobotInfo()
{
	Clear();
}

void TRobotInfo::OperationClear()
{
	CurTimeCount = INVALID_TIME_COUNT;
	ActionTime = INVALID_TIME_COUNT;
	betType = sbtNone;
	hideBetValue = 0;
	vsUserId = INVALID_USERID;
}

void TRobotInfo::Clear()
{
	CurTimeCount = INVALID_TIME_COUNT;
	ActionTime = INVALID_TIME_COUNT;
	betType = sbtNone;
	hideBetValue = 0;
	vsUserId = INVALID_USERID;
	lastAction = sbtNone;
}

SArrangeTableItem::SArrangeTableItem() : tableHandle(-1), selScore(-1), curUser(0), canSitMaxUser(0), arrangeLen(0)
{
	memset(arrangeUserAry, 0, sizeof(arrangeUserAry));
}

SArrangeUserList::SArrangeUserList()
{
	Clear();
}

void SArrangeUserList::Clear()
{
	len = 0;
	selScore = -1;
}

int SArrangeUserList::SortIfTimeout(uint32_t msTime)
{
	int Result = 0;
	uint32_t curMsTick = GetNowMsTick();
	for (int i = 0; i < len; ++i)
	{
		NF_SHARE_PTR <SUserInfo> pUser = UserAry[i];
		if (GetMsTickDiff(pUser->activeInfo.yaoyaoTick, curMsTick) >= msTime)
		{
			pUser->tmpInt = 1;      // 超时为1
			++Result;
		}
		else
		{
			pUser->tmpInt = 0;
		}
	}
	// 只有1和0的数组排序, 1排到前面
	if (Result > 0)
	{
		for (int i = 0; i < len - 1; ++i)
		{
			if (UserAry[i]->tmpInt != 1)
			{
				int maxIndex = i;
				for (int j = i + 1; j < len; ++j)
				{
					if (1 == UserAry[j]->tmpInt)
					{
						maxIndex = j;
						break;
					}
				}
				if (maxIndex != i)
				{
					NF_SHARE_PTR <SUserInfo> tmpPUserInfo;
					tmpPUserInfo = UserAry[i];
					UserAry[i] = UserAry[maxIndex];
					UserAry[maxIndex] = tmpPUserInfo;
				}
				else
				{
					// 没有为1的元素了
					break;
				}
			}
		}
	}

	return Result;
}

CNearUserGraph::CNearUserGraph() : m_userList(), m_graph(), m_pCurNear(NULL), NearList()
{

}

CNearUserGraph::~CNearUserGraph()
{
	Clear();

	if (m_pCurNear)
		delete m_pCurNear;
}

void CNearUserGraph::Clear()
{
	m_userList.Clear();
	//CLEAR_POINTER_CONTAINER(m_graph);
	//CLEAR_POINTER_CONTAINER(NearList);
}

void CNearUserGraph::UpdateGraph(int distance)
{
	Clear();
	// 最少多少人同时yaoyao
	//if (GetYaoyaoUserListIgnoreSelScore(m_userList) < MIN_TABLE_USER_COUNT)
	//	return;
	// 初始化节点
	m_graph.reserve(m_userList.len);
	for (int i = 0; i < m_userList.len; ++i)
	{
		SGraphItem* pItem = new SGraphItem();
		pItem->value = m_userList.UserAry[i];
		m_graph.push_back(pItem);
	}
	// 计算距离，生成图
	int len = (int)m_graph.size();
	for (int i = 0; i < len; ++i)
	{
		SGraphItem* pI = m_graph[i];
		for (int j = i + 1; j < len; ++j)
		{
			SGraphItem* pJ = m_graph[j];
			//if (get_distance(pI->value->activeInfo.weiDu, pI->value->activeInfo.jingDu, pJ->value->activeInfo.weiDu, pJ->value->activeInfo.jingDu) <= distance)
				pI->edge.push_back(pJ);
		}
	}
	// DFS
	for (vector<SGraphItem*>::iterator it = m_graph.begin(); it != m_graph.end(); ++it)
	{
		SGraphItem* pItem = *it;
		if (0 == pItem->color)
		{
			if (!m_pCurNear)
				m_pCurNear = new SArrangeUserList();
			m_pCurNear->Clear();

			DfsVisit(pItem);
			// 是否找到符合条件的树
			if (m_pCurNear->len >= MIN_TABLE_USER_COUNT)
			{
				NearList.push_back(m_pCurNear);
				m_pCurNear = NULL;
			}
		}
	}
}

void CNearUserGraph::DfsVisit(SGraphItem* pItem)
{
	// visit 只要调用DfsVisit的都是白色节点。
	{
		m_pCurNear->UserAry[m_pCurNear->len++] = pItem->value;
		if (m_pCurNear->selScore < 0)
			m_pCurNear->selScore = pItem->value->activeInfo.selScore;
		else if (m_pCurNear->selScore > pItem->value->activeInfo.selScore)  // 距离近的取最小选择的底分。
			m_pCurNear->selScore = pItem->value->activeInfo.selScore;
	}
	pItem->color = 1;

	// edge
	for (list<SGraphItem*>::iterator it = pItem->edge.begin(); it != pItem->edge.end(); ++it)
	{
		SGraphItem* pEdge = *it;
		if (0 == pEdge->color)
			DfsVisit(pEdge);
	}

	// visited
	pItem->color = 2;
}

SGraphItem::SGraphItem() : value(NULL), edge(), color(0)
{

}


SInfoLogItem::SInfoLogItem() : userId(-1), channelId(-1), chairId(-1), startBean(0), endBean(0), totalBet(0), startCards(""), endCards(""), username(""), nickname("")
{

}

SActionLogItem::SActionLogItem() : userId(-1), betBean(0), cardType("[]"), cards("[]"), currTimes(0), tstate(0)
{

}

SMatchLog::SMatchLog() : matchId(0), typeId(0), userCount(0), startTime(0), endTime(0), handle(0), selScore(0), tax(0), channelId(""), infoLog(), actionLog()
{

}

void SMatchLog::Clear()
{
	matchId = 0;
	typeId = 0;
	userCount = 0;
	startTime = 0;
	endTime = 0;
	handle = 0;
	selScore = 0;
	tax = 0;
	channelId = "";
	//	CLEAR_POINTER_MAP(infoLog);
		//CLEAR_POINTER_CONTAINER(actionLog);
}

// 
// void SMatchLog::WriteToPluto(nlohmann::json& p)
// {
// // 	p << matchId << typeId << userCount << startTime << endTime << handle << selScore << tax << channelId;
// // 
// // 	//infoLog
// // 	uint16_t len = (uint16_t)infoLog.size();
// // 	p << len;
// // 	for (map<int, SInfoLogItem*>::iterator it = infoLog.begin(); it != infoLog.end(); it++)
// // 	{
// // 		SInfoLogItem *item = it->second;
// // 		p << item->userId << item->channelId << item->chairId << item->startBean << item->endBean << item->totalBet << item->SysRect << item->startCards << item->endCards << item->username << item->nickname;
// // 	}
// // 
// // 	//actionLog
// // 	len = (uint16_t)actionLog.size();
// // 	p << len;
// // 	for (uint16_t i = 0; i < len; ++i) {
// // 		SActionLogItem *item = actionLog[i];
// // 		p << item->userId << item->betBean << 0 << item->cardType << item->cards << item->currTimes << item->tstate;
// // 	}
// 	try
// 	{
// 		nlohmann::json jsonStruct;
// 
// 		jsonStruct["matchId"] = matchId;
// 		jsonStruct["typeId"] = typeId;
// 		jsonStruct["userCount"] = userCount;
// 		jsonStruct["startTime"] = startTime;
// 		jsonStruct["endTime"] = endTime;
// 		jsonStruct["handle"] = handle;
// 		jsonStruct["selScore"] = selScore;
// 		jsonStruct["tax"] = tax;
// 		jsonStruct["channelId"] = channelId;
// 		
// 		nlohmann::json jsonStructObject = nlohmann::json::object({ {"baseInfo",jsonStruct} });
// 
// 		p.insert(jsonStructObject.begin(), jsonStructObject.end());
// 	}
// 	catch (const nlohmann::detail::exception& ex)
// 	{
// 		NFASSERT(0, ex.what(), __FILE__, __FUNCTION__);
// 	}
// }

int  NFGameTable::ProcMatchLogReportToDb(uint32_t taskId, uint32_t serverId, int32_t gameRoomId, int32_t isLockGame,
	uint64_t matchId, uint32_t typeId, uint32_t userCount, uint64_t startTime,
	uint64_t endTime, uint32_t handle, uint32_t selScore, uint32_t tax,
	const char* strChannelId, uint32_t winuser)
{
	 	LogInfo(" CGameTable::ProcMatchLogReportToDb", "start");
	 	//if (p->size() != 16)
	 	//{
	 	//	LogInfo("CWorldDbMgr::MatchLogReport", "p->size()=%d", p->size());
	 	//	return -1;
	 	//}
	 
	 	int index = 0;
	 	// 	uint32_t taskId = (*p)[index++]->vv.u32;
	 	// 	uint32_t serverId = (*p)[index++]->vv.u32;
	 	// 	int32_t gameRoomId = (*p)[index++]->vv.i32;
	 	// 	int32_t isLockGame = (*p)[index++]->vv.i32;
	 	// 
	 	// 	uint64_t matchId = (*p)[index++]->vv.u64;
	 	// 	uint32_t typeId = (*p)[index++]->vv.u32;
	 	// 	uint32_t userCount = (*p)[index++]->vv.u32;
	 	// 	uint64_t startTime = (*p)[index++]->vv.u64;
	 	// 
	 	// 	uint64_t endTime = (*p)[index++]->vv.u64;
	 	// 	uint32_t handle = (*p)[index++]->vv.u32;
	 	// 	uint32_t selScore = (*p)[index++]->vv.u32;
	 	// 	uint32_t tax = (*p)[index++]->vv.u32;
	 	// 	
	 	// 	const char* strChannelId = VOBJECT_GET_STR((*p)[index++]);
	 	uint64_t gameTime = endTime - startTime;
	 	string strCardValue = "\"";
	 
	 	//T_VECTOR_OBJECT* pAry = (*p)[index++]->vv.oOrAry;
	 	char buffer[300];
	 	char cardsBuf[20];
	 
	 	string strData2("[");
	 	//int blen = (int)pAry->size();
	 	//uint16_t len = (uint16_t)m_matchLog.infoLog.size();
	 	//p << len;
	 	int i = 0;
	 	for (map<int, SInfoLogItem*>::iterator it = m_matchLog.infoLog.begin(); it != m_matchLog.infoLog.end(); it++)
	 	{
	 		SInfoLogItem *item = it->second;
	 		//	p << item->userId << item->channelId << item->chairId << item->startBean << item->endBean << item->totalBet << item->SysRect << item->startCards << item->endCards << item->username << item->nickname;
	 		//}
	 
	 		//for (int i = 0; i < len; i++)
	 		//{
	 		//T_VECTOR_OBJECT* item = (*pAry)[i]->vv.oOrAry;
	 		uint32_t userId = item->userId;
	 		int32_t channelId = item->channelId;
	 		uint32_t chairId = item->chairId;
	 		int64_t startBean = item->startBean;
	 		int64_t endBean = item->endBean;
	 		uint64_t totalBet = item->totalBet;
	 		uint64_t SysRect = item->SysRect;
	 		const char* startCards = item->startCards.c_str();
	 		const char* endCards = item->endCards.c_str();
	 		const char* username = item->username.c_str();
	 		const char* nickname = item->nickname.c_str();
	 
	 		if (i != 0)
	 		{
	 			strData2 += ",";
	 			strCardValue += ",";
	 		}
	 
	 		string strCards(startCards);
	 		strCardValue += strCards;
	 		snprintf(buffer, sizeof(buffer), "{\"ChannelID\": %d, \"ChairID\": %u, \"UserID\": %u, \"KindID\": %u, \"ServerID\": %u, \"TableID\": %u, \"Score\": %lld, \"SysRecv\": %u, \"AllBet\": %llu, \"ValidBet\": %llu, \"PlayType\": 0, \"PlayTime\": %llu, \"EndGameTime\": %llu, \"Account\": \"%s\", \"NickName\": \"%s\"}", channelId, chairId, userId, 10205, typeId, handle, endBean - startBean, SysRect, totalBet, totalBet, endTime - startTime, endTime, username, nickname);
	 		strData2 += buffer;
	 		i++;
	 	}
	 	strData2 += "]";
	 	strCardValue += "\"";
	 
	 	//pAry = (*p)[index++]->vv.oOrAry;
	 
	 	//win userid
	 	char winBuff[50];
	 	//uint32_t winuser = (*p)[index++]->vv.i32;
	 	snprintf(winBuff, sizeof(winBuff), "%d", winuser);
	 	string strWinUser(winBuff);
	 
	 	string opValue("\"[");
	 
	 	//actionLog
	 	int len = (uint16_t)m_matchLog.actionLog.size();
	 	//p << len;
	 	//for (uint16_t i = 0; i < len; ++i) {
	 
	 	//p << item->userId << item->betBean << 0 << item->cardType << item->cards << item->currTimes << item->tstate;
	 	//}
	 	//int alen = (int)pAry->size();
	 	for (int i = 0; i < len; i++)
	 	{
	 		SActionLogItem *item = m_matchLog.actionLog[i];
	 		//T_VECTOR_OBJECT* item = (*pAry)[i]->vv.oOrAry;
	 		uint32_t userId = item->userId;
	 		int64_t betBean = item->betBean;
	 		int32_t betArea = 0;
	 		const char* cardType = item->cardType.c_str();
	 		const char* cards = item->cards.c_str();
	 		int32_t currTime = item->currTimes;
	 		int32_t tstate = item->tstate;
	 		if (i != 0)
	 			opValue += ",";
	 
	 		snprintf(buffer, sizeof(buffer), "[%u, %lld, %d, %s, %s, %d, %d]", userId, betBean, betArea, cardType, cards, currTime, tstate);
	 		opValue += buffer;
	 	}
	 	opValue += ",";
	 	opValue += strWinUser;
	 	opValue += "]\"";
	 
	 	snprintf(buffer, sizeof(buffer), "%lld", matchId);
	 	string strMatchId(buffer);
	 	snprintf(buffer, sizeof(buffer), "%u", typeId);
	 	string strTypeId(buffer);
	 	snprintf(buffer, sizeof(buffer), "%llu", gameTime);
	 	string strGameTime(buffer);
	 	snprintf(buffer, sizeof(buffer), "%d", userCount);
	 	string strUserCount(buffer);
	 	snprintf(buffer, sizeof(buffer), "%u", handle);
	 	string strHandle(buffer);
	 	snprintf(buffer, sizeof(buffer), "%u", tax);
	 	string strTax(buffer);
	 	snprintf(buffer, sizeof(buffer), "%lld", startTime);
	 	string strStartTime(buffer);
	 	snprintf(buffer, sizeof(buffer), "%lld", endTime);
	 	string strEndTime(buffer);
	 
	 
	 
	 
	 	string strData("{\"GameTime\": ");
	 	strData += strGameTime;
	 
	 	strData += ", \"EndTime\": ";
	 	strData += strEndTime;
	 
	 	strData += ", \"TableID\": ";
	 	strData += strHandle;
	 
	 	strData += ", \"KindID\": 10205";
	 	strData += ", \"PlayerChannels\": \"";
	 	strData += strChannelId;
	 	strData += "\"";
	 
	 	strData += ", \"ServerID\": ";
	 	strData += strTypeId;
	 
	 	strData += ", \"RoundCount\":0 ";
	 
	 	strData += ", \"UserCount\": ";
	 	strData += strUserCount;
	 
	 	strData += ", \"TaxCount\": ";
	 	strData += strTax;
	 
	 	strData += ", \"CardValue\": ";
	 	strData += strCardValue;
	 
	 	strData += ", \"OpValue\": ";
	 	strData += opValue;
	 
	 	strData += ", \"GameStartTime\": ";
	 	strData += strStartTime;
	 	//win userid
	 
	 	//strData += ", \"WinerID\": ";
	 	//strData += strWinUser;
	 
	 	strData += "}";
	 
	 	string postStr = "{\"data\":" + strData + ", \"data2\":" + strData2 + "}";
	 	LogInfo("CGameTable::ProcMatchLogReportToDb", "!!!!!!!!!!!!!!!!!!!!!!!!!! postStr=%s -- sizeof:%d", postStr.c_str() , postStr.size());
	 	//string retStr;
	 	//http_post_with_json_header(m_match_log_url.c_str(), postStr.c_str(), retStr);
	 	//CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
	 	if (m_pNFIGameServerToDBModule)
	 	{	
			//创建任务
			NF_SHARE_PTR<CAreaTaskItemBase> task = make_shared<CAreaTaskMatchLog>();
			m_pQsTaskScheduleModule->AddTask(task);
			//LogInfo("CWorldLogin:::ProcUserLockGameRoomToDb", "userId=%d;fd=%d;usercount=%d;taskcount=%d", userId, srcFd, m_fd2userInfo.size(), m_taskList.size());
			LogInfo("CGameTable::ProcMatchLogReportToDb", "创建任务taskid:%d, postStr=%s", taskId, postStr.c_str());
			int32_t gameLock = 0;
			int32_t gameRoomId = m_pConfigAreaModule->gameRoomId;
			short dataLen = postStr.size();
			m_pNFIGameServerToDBModule->SendDataToDb(NFMsg::MSGID_DBMGR_MATCH_LOG_TODB, taskId, gameRoomId, dataLen, postStr.c_str());


	 		
	 	}
	 	else
	 	{
	 		LogError("CGameTable::ProcMatchLogReportToDb", "!mbDbmgr");
	 	}
	return 0;
}


int   NFGameTable::ProcUserScoreReportToDb(uint32_t taskId, uint32_t serverId, int32_t gameRoomId, int32_t isLockGame,
	string tableNum, string openSeriesNum, string gameStartMsStamp, int32_t basescore,
	int32_t roundFee, int32_t vipRoomType, int32_t isVipRoomEnd)
{
	LogInfo("CGameTable::ProcUserScoreReportToDb", "start");
	int index = 0;
	// 	uint32_t taskId = (*p)[index++]->vv.u32;   1
	// 	uint32_t serverId = (*p)[index++]->vv.u32; 2
	// 	int32_t gameRoomId = (*p)[index++]->vv.i32; 3
	// 	int32_t isLockGame = (*p)[index++]->vv.i32; 4
	// 	string& tableNum = *(*p)[index++]->vv.s;    5
	// 	string& openSeriesNum = *(*p)[index++]->vv.s; 6
	// 	string& gameStartMsStamp = *(*p)[index++]->vv.s; 7
	// 	int32_t basescore = (*p)[index++]->vv.i32;   8
	// 	int32_t roundFee = (*p)[index++]->vv.i32;    9
	// 	int32_t vipRoomType = (*p)[index++]->vv.i32; 10
	// 	int32_t isVipRoomEnd = (*p)[index++]->vv.i32; 11
	//T_VECTOR_OBJECT* pAry = (*p)[index++]->vv.oOrAry;
	//if (pAry->size() < 1)
	//{
	//	LogError("CWorldDbMgr::ScoreReport", "pAry->size() < 1");
	//	return -1;
	//}

	int32_t retCode = 0;
	string retMsg = "";
	//int32_t retCode = 0;
	//string retMsg = "";
	//vector<SCisScoreReportRetItem*> retList;
	try
	{
		char buffer[300];
		bool isLandWin = false;
		string strData("[");

		int i = 0;
		//uint16_t len = (uint16_t)m_sitUserList.size();
		//(*pu) << len;
		for (map<int, NF_SHARE_PTR<CTableUser>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
		{
			NF_SHARE_PTR<CTableUser> pTItem = iter->second;
			int userId = pTItem->userInfo.baseInfo.userId;

			if (pTItem->roundIncBean > 0)
			{
				int64_t tIncBean = pTItem->roundIncBean;
				pTItem->roundIncBean = (int64_t)(pTItem->roundIncBean * (1000 - m_pConfigAreaModule->revenue) / 1000);
				pTItem->roundRevenue = tIncBean - pTItem->roundIncBean;
			}

			//(*pu) << userId << pTItem->roundIncBean << pTItem->roundRevenue << pTItem->totalBet << pTItem->totalBet;

			// 	int len = (int)pAry->size();
			// 	for (int i = 0; i < len; i++)
			// 	{
			// 		T_VECTOR_OBJECT* item = (*pAry)[i]->vv.oOrAry;
			int32_t itemUserId = userId;
			int64_t bean = pTItem->roundIncBean;
			int64_t revenue = pTItem->roundRevenue;
			int64_t totalBet = pTItem->totalBet;
			int64_t validBet = pTItem->totalBet;

			if (i != 0)
				strData += ",";

			snprintf(buffer, sizeof(buffer), "{\"userId\": %d, \"bean\":%lld, \"revenue\":%lld,\"totalBet\":%lld,\"validBet\":%lld}", itemUserId, bean, revenue, totalBet, validBet);
			strData += buffer;
			i++;
		}
		strData += "]";

		int winRole = 2;
		snprintf(buffer, sizeof(buffer), "{\"winnerRole\": %d, \"anteNum\": %d, \"multipleNum\": %d, \"roundFee\": %d, \"vipRoomType\": %d, \"isVipRoomEnd\": %d, \"tableNum\": \"%s\"}",
			winRole, basescore, 1, roundFee, vipRoomType, isVipRoomEnd, tableNum.c_str());
		string strData2(buffer);
		const int C_GAME_ID = 8; // 游戏id
		uint64_t stamp = GetTimeStampInt64Ms();
		snprintf(buffer, sizeof(buffer), "%llu", stamp);
		string strStamp(buffer);
		snprintf(buffer, sizeof(buffer), "%u", serverId);
		string strServerId(buffer);
		snprintf(buffer, sizeof(buffer), "%d", C_GAME_ID);
		string strGameId(buffer);
		snprintf(buffer, sizeof(buffer), "%d", gameRoomId);
		string strGameRoomId(buffer);
		snprintf(buffer, sizeof(buffer), "%d", isLockGame);
		string strIsLockGameRoom(buffer);
		string checkCode;
		string m_cis_key = "goisnfgsf34-9f25";
		GetStrMd5(strGameId + strGameRoomId + openSeriesNum + strData + strData2 + strStamp + m_cis_key, checkCode);
		string postData("action=ReportGameResult&gameId=");
		postData += strGameId;
		postData += "&gameServerId=";
		postData += strServerId;
		postData += "&gameRoomId=";
		postData += strGameRoomId;
		postData += "&isLockGameRoom=";
		postData += strIsLockGameRoom;
		postData += "&openSeriesNum=";
		postData += openSeriesNum;
		postData += "&data=";
		postData += strData;
		postData += "&data2=";
		postData += strData2;
		postData += "&startTime=";
		postData += gameStartMsStamp;
		postData += "&timestamp=";
		postData += strStamp;
		postData += "&checkCode=";
		postData += checkCode;
		//string jsStr;
		//http_post(m_cis_url.c_str(), postData.c_str(), jsStr);
		//LogInfo("CWorldDbMgr::ScoreReport", "结果上报接收到的数据:%s", postData.c_str());


  // 		CMailBox* mbDbmgr = WORLD_MGR->GetMailboxByServerID(SERVER_DBMGR);
  // 		if (mbDbmgr)
  // 		{
  // 			if (!mbDbmgr->IsConnected())
  // 			{
  // 				LogWarning("CGameTable::ProcUserScoreReportToDb", "!mbDbmgr->IsConnected()");
  // 				//直接登录失败
  // 				//ClientUpdateUserInfoResponse(srcFd, 100, "服务器维护中");
  // 				return -1;
  // 			}
  // 			CPluto* pu = new CPluto();
  // 			(*pu).Encode(MSGID_DBMGR_REPORT_SCORE_TODB) << taskId << CONFIG_AREA->gameRoomId << postData << EndPluto;
  // 			mbDbmgr->PushPluto(pu);
		if (m_pNFIGameServerToDBModule)
		{
			short dataLen = postData.size();
			m_pNFIGameServerToDBModule->SendReadUserInfo(taskId, gameRoomId, dataLen, postData.c_str());

		}
		else
		{
			m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
		}
//	}
// 		else
// 		{
// 		ThrowException(3, "读取mbDbmgr失败");
// 		}
}
catch (CException & ex)
{
	retCode = ex.GetCode();
	retMsg = ex.GetMsg();

	LogError("CWorldDbMgr::ScoreReport", "code=%d, error: %s ", ex.GetCode(), ex.GetMsg().c_str());
}



return 0;
}

//MSGID_AREA_REPORT_SCORE_CALLBACK



int   NFGameTable::ProcUserLockGameRoomToDb(uint32_t taskId, int32_t gameRoomId, int32_t userId, int32_t type)
{
	LogInfo(" CGameTable::ProcUserLockGameRoomToDb", "start");

	int index = 0;

	int32_t retCode = 0;
	string retMsg = "";

	char buffer[1256] = {0};
	uint64_t stamp = GetTimeStampInt64Ms();
	snprintf(buffer, sizeof(buffer), "%ld", stamp);
	string strTimeStamp(buffer);
	snprintf(buffer, sizeof(buffer), "%d", gameRoomId);
	string strGameRoomId(buffer);
	snprintf(buffer, sizeof(buffer), "%d", userId);
	string strUserId(buffer);
	snprintf(buffer, sizeof(buffer), "%d", type);
	string strType(buffer);
	string checkCode;
	string m_cis_key = "goisnfgsf34-9f25";
	GetStrMd5(strUserId + strGameRoomId + strType + strTimeStamp + m_cis_key, checkCode);

	string postData("action=LockGameRoom&userId=");
	postData += strUserId;
	postData += "&gameRoomId=";
	postData += strGameRoomId;
	postData += "&type=";
	postData += strType;
	postData += "&timestamp=";
	postData += strTimeStamp;
	postData += "&checkCode=";
	postData += checkCode;

	//string jsStr;
	LogInfo("CWorldDbMgr::LockOrUnLockUser", "发送的数据:%s", postData.c_str());
	//CCalcTimeTick ctime;

	//LogInfo("CWorldDbMgr::ScoreReport", "------------------------用户上锁/解锁用时%dMs------------------", useTime);

  	if (m_pNFIGameServerToDBModule)
  	{
		LogInfo(" CGameTable::ProcUserLockGameRoomToDb", "任务taskid:%d,上送的数据:%s", taskId, postData.c_str());
		int32_t gameLock = 0;
		int32_t gameRoomId = m_pConfigAreaModule->gameRoomId;

		if (m_pNFIGameServerToDBModule)
		{
			short dataLen = postData.size();
			m_pNFIGameServerToDBModule->SendDataToDb(NFMsg::MSGID_DBMGR_LOCK_GAMEROOM_TODB, taskId, gameRoomId, dataLen, postData.c_str());

		}
		else
		{
			m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
		}
  		
  	}
  	else
  	{
  		LogError(" CGameTable::ProcUserLockGameRoomToDb", "!mbDbmgr");
  	}

	return 0;

}

//MSGID_AREA_LOCK_GAMEROOM_CALLBACK



void NFCTableManagerModule::OnNodeDbGidReadUserInfoCallback(const NFSOCK nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen)
{
	NFGamespace::DB_UserBaseInfo baseInfo;

	baseInfo.CopyFrom(msg);
	int index = 0;
	uint32_t taskId = baseInfo.taskid;

	//NF_SHARE_PTR<CAreaTaskItemBase> task = make_shared<CAreaTaskReadUserInfo>(NFMsg::MSGID_CLIENT_LOGIN, 1330001);
	//NF_SHARE_PTR < CAreaTaskReadUserInfo> task = dynamic_cast<NF_SHARE_PTR<CAreaTaskReadUserInfo>>(m_pQsTaskScheduleModule->PopDbTask(taskId));
	//CAreaTaskItemBase* task =  m_pQsTaskScheduleModule->PopDbTask(taskId).get();
	NF_SHARE_PTR<CAreaTaskReadUserInfo> task = NFUtil::Cast<CAreaTaskReadUserInfo, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
	if (!task)
	{
		m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
		return;
	}

	// 自动释放内存
	//auto_new1_ptr<CAreaTaskReadUserInfo> atask(task);


	int32_t retCode = baseInfo.retCode;

	string& retErrorMsg = baseInfo.retErrorMsg;


	if (NFMsg::MSGID_ROBOT_READ_USERINFO == task->GetMsgId())
	{
		//robot curindex
		int userid = task->GetClientUserId();
		m_pRobotModule->ReadUserInfoCallback(retCode, retErrorMsg.c_str(), userid, baseInfo);
		return;
	}
	NFGUID clientFd = task->GetGUId();
	NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByGuid(clientFd);
	//SUserInfo* pUser = (SUserInfo*) (m_pGameLogicModule->GetUserInfoByUserId(baseInfo.userId)).get();
	if (!pUser)
	{
		m_pLogModule->LogInfo("find user failed", __FUNCTION__, __LINE__);
		return;
	}
	
	if (task->GetMsgId() == NFMsg::MSGID_CLIENT_LOGIN)
	{
		if (retCode == 101)
		{
			nlohmann::json jsonSend;
			jsonSend["action"] = NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY;
			jsonSend["code"] = 62;
			m_pGameLogicModule->SendMsgToClient(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, jsonSend, clientFd);
// 
// 			NF_SHARE_PTR<CPluto> share_pu = make_shared<CPluto>();
// 			(*share_pu).Encode(MSGID_CLIENT_FORCE_LEAVE_NOTIFY) << int32_t(ERROR_CODE_DISTANCE_LOGIN) << EndPluto;
// 			m_pNetCommonModule->SendMail(share_pu, clientFd);
// 
// 			m_pWorldGameAreaModule->AddCloseFd(clientFd);
// 			m_pNetCommonModule->RemoveUserByFd(clientFd);
			return;
		}

		// 登录返回
		if (0 != retCode)
		{
			auto ret = m_pGameLogicModule->ClientLoginResponse(clientFd, retCode, retErrorMsg.c_str());
			return;
		}
		else
		{
			// 机器人不让登录，否则会错乱
			if (m_pRobotModule->IsRobot(baseInfo.userId))
			{
				// vs下汉字编译的问题
#ifndef _WIN32
				return m_pGameLogicModule->ClientLoginResponse(clientFd, 1003, "帐号不存在");
#endif
			}

			if (EUS_NONE != pUser->activeInfo.userState)
			{
				m_pLogModule->LogInfo("EUS_NONE != pUser->ativeInfo.userState", __FUNCTION__, __LINE__);
				return;
			}

			auto retUser = m_pGameLogicModule->FindUserByUserId(baseInfo.userId);

			if (retUser)
			{
				auto pOldUser = m_pGameLogicModule->GetUserInfoByUserId(baseInfo.userId);
				NFGUID fdOld = pOldUser->self;
				NFMsg::RoleOfflineNotify xMsg;
				//const NFGUID& xGuild = m_pKernelModule->GetPropertyObject(self, NFrame::Player::GuildID());
				//NFGUID self = pUser->self;

				*xMsg.mutable_self() = NFINetModule::NFToPB(fdOld);
				*xMsg.mutable_guild() = NFINetModule::NFToPB(fdOld);
				xMsg.set_game(pPluginManager->GetAppID());
				xMsg.set_proxy(0);


				std::string strMsg;
				if (!xMsg.SerializeToString(&strMsg))
				{
					m_pLogModule->LogError("Serlize strMsg fail ", __FUNCTION__, __LINE__);
					return;
				}

				m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::MSGID_CLIENT_LOGIN_RESP, strMsg.c_str(), fdOld);
			}
			//添加帐号到对应关系
			pUser->baseInfo.CopyFrom(baseInfo);
			pUser->activeInfo.userState = EUS_AUTHED;
			m_pGameLogicModule->AddUserToUserId(baseInfo.userId, pUser);

			// 检测断线返回
			bool isOfflineRet = false;
			NF_SHARE_PTR<NFGameTable> pTable = nullptr;
			int chairIndex = 0;
			{
				int userId = pUser->baseInfo.userId;
				pTable = m_pTableManagerModule->GetUserTable(userId, chairIndex);
				if (pTable)
				{
					isOfflineRet = pTable->EnterTable(pUser, chairIndex, false);
					if (!isOfflineRet)
					{
						m_pLogModule->LogError("EnterTable failed", __FUNCTION__, __LINE__);
					}
				}

			}
			if (!isOfflineRet)
			{
				auto ret = m_pGameLogicModule->ClientLoginResponse(clientFd, retCode, retErrorMsg.c_str());
				return;
			}
			else
			{
				// 清空下注信息
				//NF_SHARE_PTR<NFTableUser> pTableUser = pTable->FindUserById(pUser->baseInfo.userId);
				//if (pTableUser)
				//{
				//    pTable->SubBetBeanOfTableUser(pTableUser);
				//}
				// 先通知登录成功
				m_pGameLogicModule->ClientLoginResponse(clientFd, retCode, retErrorMsg.c_str());
				// 通知开始游戏
				pTable->SendStartGameNotify(chairIndex);
				std::ostringstream logStr;
				logStr << "offline return success userId = " << pUser->baseInfo.userId << " table = " << pTable->GetHandle() << " chairIndex = " << chairIndex;
				m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

				return;
			}
		}
	}

	if (task->GetMsgId() == NFMsg::MSGID_CLIENT_LOGIN)
	{
		// 刷新用户信息返回
		if (0 != retCode)
		{
			m_pGameLogicModule->ClientUpdateUserInfoResponse(clientFd, retCode, retErrorMsg.c_str());
			return;
		}
		else
		{
			//刷新用户信息
			pUser->baseInfo.CopyFrom(baseInfo);
			int chairIndex = 0;
			NF_SHARE_PTR<NFGameTable> pTable = GetPUserTable(*pUser, chairIndex);
			if (pTable)
				pTable->ClientUpdateUserInfo(baseInfo, chairIndex);

			m_pGameLogicModule->ClientUpdateUserInfoResponse(clientFd, retCode, retErrorMsg.c_str());

			return;
		}
	}
	// 
	// 	std::ostringstream logStr;
	// 	logStr << "task->GetMsgId() not found " << task->GetMsgId();
	// 	m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);

	return;
}
void NFCTableManagerModule::OnMsGidLockGameRoomCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGamespace::DB_LoackGameRoom p;
	p.CopyFrom(msg);

	int index = 0;
	uint32_t taskId = p.taskid;


	NF_SHARE_PTR<CAreaTaskLockOrUnlockUser> task = NFUtil::Cast<CAreaTaskLockOrUnlockUser, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
	if (!task)
	{
		m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
		return;
	}
	int32_t retCode = p.retCode;
	string& retErrorMsg = p.retErrorMsg;
	int32_t userid = task->GetClientUserId();
	if (0 != retCode)
	{
		NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByUserId(userid);
		//SUserInfo * userInfo = FindUserById(userid);
		if (pUser)
		{
			nlohmann::json jsonSend;
			jsonSend["action"] = NFMsg::MSGID_CLIENT_LOGIN_RESP;
			jsonSend["userId"] = pUser->baseInfo.userId;

			std::string sendJsonString = jsonSend.dump();
			m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::MSGID_CLIENT_OTHER_PLACE_LOGIN, sendJsonString.c_str(), pUser->self);
		}
		else
		{
			m_pLogModule->LogError("rocLockOrUnlockReportCallBack user is null", __FUNCTION__, __LINE__);
			//LogError("3333 ProcLockOrUnlockReportCallBack user is null", "fd:%d", userid);
		}
		std::ostringstream stream;
		stream << "CWorldGameArea::ProcLockOrUnlockReportCallBack" << nSockIndex;
		m_pLogModule->LogError(stream, __FUNCTION__, __LINE__);
		return;
	}


	return;
}


void NFCTableManagerModule::OnMsGidMatchReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{

	int index = 0;
	int32_t retCode;
	string retErrorMsg;
	int32_t tableHandle;

	NF_SHARE_PTR<NFGameTable> pTable = GetTableByHandle(tableHandle);
	if (!pTable)
	{
		m_pLogModule->LogError("find table failed!", __FUNCTION__, __LINE__);
		return;
	}

	if (retCode != 0)
	{
		//todo
		//pTable->OnMatchReportError(retCode, retErrorMsg.c_str());
	}

	return;
}

void NFCTableManagerModule::OnMsGidScoreReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGamespace::DB_ReportScore p;
	uint32_t jsonLen = p.CopyFrom(msg);
	int index = 0;
	uint32_t taskId = p.taskid;
	//CAreaTaskReportScore* task = dynamic_cast<CAreaTaskReportScore*>(m_pNetCommonModule->PopDbTask(taskId));
	NF_SHARE_PTR<CAreaTaskReportScore> task = NFUtil::Cast<CAreaTaskReportScore, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));

	if (!task)
	{
		m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
		return;
	}
	// 自动释放内存
	//auto_new1_ptr<CAreaTaskReportScore> atask(task);

	NF_SHARE_PTR<NFGameTable> pTable = GetTableByHandle(task->GetTableHandle());
	if (!pTable)
	{
		m_pLogModule->LogError("find table failed!", __FUNCTION__, __LINE__);
		return;
	}

	int32_t retCode;
	string retErrorMsg;
	if (0 != retCode)
		pTable->OnReportScoreError(retCode, retErrorMsg.c_str());
	else
	{
		nlohmann::json pJson;
		try
		{
			pJson = nlohmann::json::parse(msg + jsonLen);
		}
		catch (const nlohmann::detail::exception& ex)
		{
			m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
			return;
		}
		pTable->OnReportScoreSuccess(pJson, index);
	}

	return;
}

void NFCTableManagerModule::OnMsGidGameControlCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	//if (p->size() != 5)
	//{
	//    m_pLogModule->LogInfo("p->size() error", __FUNCTION__, __LINE__);
	//    return -1;
	//}

	int index = 0;
	NFGamespace::DB_Gamecontrol p;
	p.CopyFrom(msg);
	uint32_t taskId = p.taskid;

	//CAreaTaskGameControl* task = dynamic_cast<CAreaTaskGameControl*>(m_pNetCommonModule->PopDbTask(taskId));
	NF_SHARE_PTR<CAreaTaskGameControl> task = NFUtil::Cast<CAreaTaskGameControl, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
	if (!task)
	{
		m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
		return;
	}

	// 自动释放内存
	//auto_new1_ptr<CAreaTaskGameControl> atask(task);
	NF_SHARE_PTR<NFGameTable> pTable = m_pTableManagerModule->GetTableByHandle(task->GetTableHandle());
	if (!pTable)
	{
		m_pLogModule->LogError("find table failed!", __FUNCTION__, __LINE__);
		return;
	}

	int32_t retCode = p.retCode;
	string& retErrorMsg = p.retErrorMsg;
	uint32_t contorlValue = p.controlValue;

	string strcontrolLimit = p.controlLimit;

	int64_t contorlLimit = stoll(strcontrolLimit);

	pTable->SetControlValue(contorlValue);
	pTable->SetControlLimit(contorlLimit);

	std::ostringstream logStr;
	logStr << "controlValue = " << contorlValue << ", score = " << contorlLimit;
	m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

	return;
}

#endif