#ifndef NF_GAME_TABLE_CPP
#define NF_GAME_TABLE_CPP

#include <string>
#include <memory>
#include <map>

#include "NFComm/NFPluginModule/NFIControlCardModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFServer/NFBaccaratGameLogicPlugin/NFGameLogicModule/NFBaccaratExpendLogic.h"
#include "NFComm/NFPluginModule/NFITableManagerModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "NFComm/NFPluginModule/NFPlatform.h"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "GameTableState/TableStateIdle.hpp"
#include "GameTableState/TableStateDeal.hpp"
#include "GameTableState/TableStateWaitGameStart.hpp"
#include "GameTableState/TableStateWagerOne.hpp"
#include "GameTableState/TableStateWagerTwo.hpp"
#include "GameTableState/TableStateFillCard.hpp"
#include "GameTableState/TableStateCalculateResult.hpp"
#include "GameTableState/TableStateWaitForNextGame.hpp"
#include "GameTableState/TableStateWithAnimation.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFQsTaskStruct.hpp"
#include "StructHPP/NFTableUser.hpp"
#include "NFGameTable.h"

using namespace std;

NFGameTable::NFGameTable()
{
}

NFGameTable::NFGameTable(int32_t handle, NFITableManagerModule * p) : m_handle(handle), m_isActive(false), m_endGameTick(0), m_lastRunTick(), m_userList(), m_sitUserList(), m_controlValue(1)
{
    m_pTableManagerModule = p;
    this->m_handle = handle;

	m_MachineContinueState = TableState::TABLE_CONTINUE;
	m_MachineDefaultState = TableState::TABLE_IDLE;
	m_MachineError = TableState::TABLE_ERROR;

    AddElement(TableState::TABLE_IDLE, std::make_shared<TableStateIdle>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_WAIT_GAME_START, std::make_shared<TableStateWaitGameStart>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_WAGER_1, std::make_shared<TableStateWagerOne>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_WAGER_2, std::make_shared<TableStateWagerTwo>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_DEAL, std::make_shared<TableStateDeal>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_FILL_CARD, std::make_shared<TableStateFillCard>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_CALCULATE_RESULT, std::make_shared<TableStateCalculateResult>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_WAIT_FOR_NEXT_GAME, std::make_shared<TableStateWaitForNextGame>(this, m_pTableManagerModule->GetPluginManager()));
    AddElement(TableState::TABLE_WITH_ANIMATION, std::make_shared<TableStateWithAnimation>(this, m_pTableManagerModule->GetPluginManager()));
}

NFGameTable::~NFGameTable()
{
    m_sitUserList.clear();
    m_userList.clear();
}

void NFGameTable::Init()
{
    if (m_pTableManagerModule->GetPluginManager() == nullptr)
        return;
	m_pNFIGameServerToDBModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIGameServerToDBModule>();

    m_pLogModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFILogModule>();
    m_pRobotMoudle = m_pTableManagerModule->GetPluginManager()->FindModule<NFIRobotModule>();
	m_pQsTaskScheduleModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIQsTaskScheduleModule>();
	m_pNetClientModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFINetClientModule>();
    m_pGameLogicModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIGameLogicModule>();
    m_pConfigAreaModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIConfigAreaModule>();
    m_pControlCardModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIControlCardModule>();
    //m_pWorldGameAreaModule = m_pTableManagerModule->GetPluginManager()->FindModule<NFIWorldGameAreaModule>();
}

void NFGameTable::AfterInit()
{
	if (!CheckMachineConfig())
	{
		NFASSERT(0, 
			"请检查桌子状态机的配置 : m_MachineContinueState == -1 || m_MachineDefaultState == -1",
			__FILE__, 
			__FUNCTION__);
	}
    for (int i = 0; i < MAX_TABLE_USER_COUNT + 1; i++)           //要加一，因为当人数满了且没有庄家的极限情况可以加一个系统机器人
    {
        m_userList.push_back(make_shared<NFTableUser>());
    }

	int index = 0;
	for (int i = 0; i < 8; i++)
	{
		for (int value = CARD_A; value <= CARD_K; ++value)
		{
			for (int color = sccDiamond; color <= sccSpade; ++color)
			{
				m_deckOfCards[index++] = value * 10 + color;
			}
		}
	}

    RandomInitTableRecord();
    RoundStopClearData();
    m_regions.init();                                           //初始化下注区域

	m_lastRunTick.Init();
	m_SystemTick.Init();
    NoUserClearData();
}


std::map<int32_t, NF_SHARE_PTR<NFITableUserModule>>& NFGameTable::GetUserList()
{
    return m_sitUserList;
}

const int32_t NFGameTable::GetHandle() const
{
    return m_handle;
}

uint32_t NFGameTable::GetTablePassTick()
{
	return m_lastRunTick.GetTimePassMillionSecond();
}

void NFGameTable::AddTableMsTick(uint32_t ms)
{
	m_lastRunTick.SetNowTime();
}

int NFGameTable::GetTimeCount()
{
    return this->m_decTimeCount;
}

void NFGameTable::SetTimeCount(int32_t value)
{
	this->m_decTimeCount = value;
}

void NFGameTable::MinusTimeCount(int value)
{
    this->m_decTimeCount -= value;
}

int64_t NFGameTable::GetAllBetBean()
{
    return m_regions.getBetAllSum();
}

void NFGameTable::SubBetBeanById(int id, int sum)
{
    m_regions.SubBetItem(id, sum);
}

void NFGameTable::SubBetBeanOfTableUser(NF_SHARE_PTR<NFTableUser> pTableUser)
{
    for (auto temp : pTableUser->GetBetVector())
    {
        SubBetBeanById(temp->betId, temp->betNum);
    }

    pTableUser->ClearBetItems();

    m_matchLog.RemoveLogByUserId(pTableUser->GetSUserInfo().baseInfo.userId);
}

void NFGameTable::CheckOfflineOfTableUser()
{
    map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iterBegin = m_sitUserList.begin();
	for (; iterBegin != m_sitUserList.end(); iterBegin++)
	{
		
		NF_SHARE_PTR<NFTableUser> tempTableUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(iterBegin->second);
		if (tempTableUser == nullptr) continue;
		if (tempTableUser->GetSUserInfo().baseInfo.userId == -1 || tempTableUser->GetIsRobot()) continue;

		NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
		if (!dbData || dbData->eState != ConnectDataState::NORMAL)
		{
			std::string log = "CheckOfflineOfTableUser  id == ";
			log.append(lexical_cast<std::string>(tempTableUser->GetSUserInfo().baseInfo.userId));
			m_pLogModule->LogInfo(log, __FUNCTION__, __LINE__);
			tempTableUser->addNoOperationNum(5); // 增加未操作次数，便于下局清除
		}
	}
}

int64_t NFGameTable::GetBeanByRegionId(int betId)
{
    return m_regions.GetRegionAllBetById(betId);
}

int NFGameTable::GetCardValueByCardID(CardID id, int index)
{
    NF_SHARE_PTR<CPlayerCard> card = m_regions.GetCardsOfCard_ID(id);

    if (card)
        return card->getCardList()[index];

    return -1;
}

bool NFGameTable::ControlCard()
{
    CardRecordItem cardStr = m_pControlCardModule->ControlCardOfTable(m_controlValue, this);
    //CardRecordItem cardStr = m_pControlCardModule->ControlCardOfTable(0, this);
    if (cardStr.dealerCardVec.size() == 0)
    {
        m_pLogModule->LogWarning("No CARDS fit the area，out of control.");
        return false;
    }
    string str = "---------------------- Control Card type --------------------";
    str.append(cardStr.GetCardTypeStr());
    m_pLogModule->LogInfo(str);

    NF_SHARE_PTR<CPlayerCard> dealerPlayerCard = m_regions.GetCardsOfCard_ID(CardID::DEALER_ID);
    //if (!dealerPlayerCard) assert(0);

    NF_SHARE_PTR<CPlayerCard> notDealerPlayerCard = m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID);
    //if (!notDealerPlayerCard) assert(0);

    dealerPlayerCard->SetCardList(cardStr.GetDealCardList());
    notDealerPlayerCard->SetCardList(cardStr.GetNotDealCardList());

    return true;
}


bool NFGameTable::IsChairIndexValid(int chairIndex)
{
    return (chairIndex >= 0) && (chairIndex < MAX_TABLE_USER_COUNT);
}

bool NFGameTable::HasFd(NF_SHARE_PTR<NFTableUser> pTUser)
{
    return (pTUser->GetUsTate() != TableSitState::tusNone && pTUser->GetUsTate() <= TableSitState::tusNomal && !pTUser->GetIsRobot());
}

BetRegions& NFGameTable::GetBetRegions()
{
    return  m_regions;
}

void NFGameTable::SetControlValue(int controlValue)
{
    m_controlValue = controlValue;
    std::string logStr = "######get contorl value : ";
    logStr.append(lexical_cast<string>(controlValue));
    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
}

int NFGameTable::GetControlValue() const
{
    return this->m_controlValue;
}

void NFGameTable::SetScore(uint32_t score)
{
    this->m_score = score;
    std::string logStr = "######get score value : ";
    logStr.append(lexical_cast<string>(score));
    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
}

uint32_t NFGameTable::GetScore()
{
    return this->m_score;
}

NF_SHARE_PTR<NFTableUser> NFGameTable::FindUserByChairIndex(int chairIndex)
{
    if (!IsChairIndexValid(chairIndex))
        return NULL;

	NF_SHARE_PTR<NFTableUser> pUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[chairIndex]);

    return pUser;
}

void NFGameTable::DealCard()
{
	Riffle();
	// 庄家的牌
	DealCardOfCardID(CardID::DEALER_ID, GetCardOfGroup(0));

	// 闲家的牌
	DealCardOfCardID(CardID::NOT_DEALER_ID, GetCardOfGroup(1));
}

void NFGameTable::FillCard()
{
	DealCardOfCardID(CardID::DEALER_ID, GetCardOfGroup(2));
	DealCardOfCardID(CardID::NOT_DEALER_ID, GetCardOfGroup(3));

	NF_SHARE_PTR<NFIItemFillProcessModule> notDealerProcess = m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetFillProcessModule(CardID::NOT_DEALER_ID);
	if (!notDealerProcess) assert(0);

	NF_SHARE_PTR<NFIItemFillProcessModule> dealerProcess = m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetFillProcessModule(CardID::DEALER_ID);
	if (!dealerProcess) assert(0);

	// 补闲家的牌
	if (notDealerProcess->FillCardProcess(this))
		FillCardOfGardID(CardID::NOT_DEALER_ID, GetCardOfGroup(NFUtil::GetRandomRange(4, DECK_CARDS_COUNT - 1)));

	// 补庄家的牌
	if (dealerProcess->FillCardProcess(this))
		FillCardOfGardID(CardID::DEALER_ID, GetCardOfGroup(NFUtil::GetRandomRange(5, DECK_CARDS_COUNT - 1)));

}

void NFGameTable::FillCardOfGardID(CardID card_id, int value)
{
    NF_SHARE_PTR<CPlayerCard> card = m_regions.GetCardsOfCard_ID(card_id);

    //if (!card) assert(0);

    if (card)
        card->FillCard(value);
}

void NFGameTable::DealCardOfCardID(CardID card_id, int value)
{
    NF_SHARE_PTR<CPlayerCard> card = m_regions.GetCardsOfCard_ID(card_id);

    //if (!card) assert(0);

    if (card)
        card->AddCard(value);
}

void NFGameTable::SendDealCardDataToClient()
{
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_G_DEALCARD_NOTIFY;
	
	nlohmann::json jsonArray = nlohmann::json::array();
	for (auto playerCard : m_regions.cardCroup)
	{
		nlohmann::json item;
		item["card_id"] = playerCard->getSelfID();
		playerCard->WriteToPluto(item);

		jsonArray.push_back(item);
	}
	nlohmann::json arrayObj = nlohmann::json::object({ {"DealCardsInfo",jsonArray} });
	jsonSend.insert(arrayObj.begin(), jsonSend.end());
	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::MSGID_CLIENT_G_DEALCARD_NOTIFY,jsonSend, INVALID_USERID, tusWaitNextRound, m_sitUserList);
}

int NFGameTable::GetRandomRobotBetAreaByProportion()
{
    int betArea = -1;
	int randomNum = NFUtil::GetRandomRange(0, 900);

    /*if (randomNum < 100)
    betArea = 0;
    else
    betArea = 800 / randomNum;

    return betArea;
    */
    if (randomNum <= 225) // 庄
        return 7;

    if (randomNum > 225 && randomNum <= 450) // 闲
        return 8;

    // 450 / 14 = 32
    // 450 + 32 * 1
    if (m_gameCount % 5 == 0)
    {
        if (randomNum > 450 && randomNum <= 482)  // 完美对子
            return 4;
    }


    // 482+ 32 * 3
    if (randomNum > 482 && randomNum <= 578)        // 小
        return 5;

    // 578 + 32 * 4
    if (randomNum > 578 && randomNum <= 706)    // 大
        return 2;

    // 706 + 32 * 1
    if (m_gameCount % 2 == 0)
    {
        if (randomNum > 706 && randomNum <= 738)    //和
            return 1;

    }

    // 738 + 32 * 1
    if (m_gameCount % 3 == 0)
    {
        if (randomNum > 738 && randomNum < 770) //庄对
            return 6;
    }


    // 770 + 32 * 1
    if (m_gameCount % 3 == 0)           //闲对
    {
        if (randomNum > 770 && randomNum < 802)
            return 3;
    }

    if (NFUtil::GetRandomRange(0, 10) < 5)
        return 0;
    else
    {
        if (NFUtil::GetRandomRange(0, 10) < 5)
            return 7;
        else
            return 8;
    }
}

int NFGameTable::GetRandomRobotBetBean()
{
    // 贵宾场 1 5 10 20 50 100 富豪场 100 200 500 1000 2000 5000 

    int randomPos = NFUtil::GetRandomRange(0, 100);

    if (m_pTableManagerModule->GetSelScoreByHandle(m_handle) == 1)
    {
        if (randomPos < 30)
            return 1 * 100;

        if (randomPos >= 30 && randomPos < 60)
            return 5 * 100;

        if (randomPos >= 60 && randomPos < 70)
            return 10 * 100;

        if (randomPos >= 70 && randomPos < 80)
            return  20 * 100;

        if (randomPos >= 80 && randomPos < 95)
            return 50 * 100;

        return 100 * 100;
    }
    else
    {
        if (randomPos < 30)
            return 100 * 100;

        if (randomPos >= 30 && randomPos < 60)
            return 200 * 100;

        if (randomPos >= 60 && randomPos < 70)
            return 500 * 100;

        if (randomPos >= 70 && randomPos < 80)
            return  1000 * 100;

        if (randomPos >= 80 && randomPos < 95)
            return 2000 * 100;

        return 5000 * 100;
    }
}

void NFGameTable::RandomRobotBetTime()
{

    if (m_pConfigAreaModule->m_enableCheat == 1)
    {
        if (NFUtil::GetRandomRange(0, 10) > 5)
        {
            int betIntervalTime = NFUtil::GetRandomRange(200, 500);
            if (m_lastRunTick.GetTimePassMillionSecond() >= (uint32_t)betIntervalTime)
            {
                int ctlUpTime = NFUtil::GetRandomRange(18, 20);
                int ctlDownTime = NFUtil::GetRandomRange(2, 4);
                if (m_decTimeCount < ctlUpTime && m_decTimeCount > ctlDownTime)
                {
                    RobotBet();
                }
            }
        }
    }
    else
    {
        m_pLogModule->LogInfo("g_config_area->m_enableCheat", __FUNCTION__, __LINE__);
    }
}

void NFGameTable::RobotBet()
{
    if (m_pConfigAreaModule->m_enableCheat == 1)
    {
        //进行下注
        int vecSize = (int)this->m_RobotList.size();

        if (vecSize < 1)
        {
            m_pLogModule->LogWarning("Dong't have any robot!!!!!!!!!!!!!!!!!!!", __FUNCTION__, __LINE__);
            return;
        }

        int randomPos = NFUtil::GetRandomRange(0, vecSize - 1);

		NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast< NFTableUser, NFITableUserModule>(m_RobotList.at(randomPos));

        int betId = GetRandomRobotBetAreaByProportion();
        int betNum = GetRandomRobotBetBean();

        //检查注数合法性
        if (pTUser == NULL)
        {
            return;
        }

        BetRegion * region = this->m_regions.getRegionById(betId);
        if (region == NULL)
        {
            return;
        }
        pTUser->initNoOperationNum();

        pTUser->AddBet(betNum, betId);
        region->AddRobotBetNum(betNum);
        int64_t allSum = this->m_regions.getBetAllSum() + this->m_regions.getRobotBetAllSum();

        //发送下注广播包
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_OTHER_BET_NOTIFY;
		jsonSend["userId"] = pTUser->GetSUserInfo().baseInfo.userId;

		nlohmann::json jsonArray = nlohmann::json::array();
		nlohmann::json betItem;
		betItem["id"] = betId;
		betItem["betSum"] = betNum;
		jsonArray.push_back(betItem);

		jsonSend.insert(jsonArray.begin(), jsonArray.end());

		m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::MSGID_CLIENT_OTHER_BET_NOTIFY, jsonSend, INVALID_USERID, tusWaitNextRound, m_sitUserList);
    }
}

void NFGameTable::WriteRobotBetInfoToPluto(nlohmann::json & j)
{
    std::map<int, int> tempMap; // TODO
    for (auto temp : m_sitUserList)
    {

		NF_SHARE_PTR<NFTableUser> tempUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(temp.second);
        if (tempUser->GetIsRobot())
        {
            for (auto temp : tempUser->GetBetVector())
            {
                if (tempMap.find(temp->betId) != tempMap.end())
                    tempMap[temp->betId] += temp->betNum;
                else
                    tempMap.insert(make_pair(temp->betId, temp->betNum));
            }
        }

    }

	nlohmann::json jsonArray = nlohmann::json::array();

    if (tempMap.size() == 0)
    {
		nlohmann::json item;
		item["betId"] = lexical_cast<std::string>(-1);
		item["betSum"] = lexical_cast<std::string>(-1);
		jsonArray.push_back(item);

    }
    else
    {
        for (auto temp : tempMap)
        {
			nlohmann::json item;
			item["betId"] = lexical_cast<std::string>(temp.first);
			item["betSum"] = lexical_cast<std::string>(temp.second);
			jsonArray.push_back(item);
        }
    }

	nlohmann::json jsonObject = nlohmann::json::object({ {"robotBetArray",jsonArray} });

	j.insert(jsonObject.begin(), jsonObject.end());
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

	NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[chairIndex]);
	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_CHAT_RESP;
		jsonSend["code"] = 0;
		jsonSend["msg"] = "";
		jsonSend["chatMsg"] = xChat.chatMsg;

		m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_CHAT_RESP, jsonSend,pTUser->GetSUserInfo().self);
	}

	{
		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::MSGID_CLIENT_OTHER_CHAT_NOTIFY;
		jsonSend["userId"] = pTUser->GetSUserInfo().baseInfo.userId;
		jsonSend["userName"] = pTUser->GetSUserInfo().baseInfo.userName;
		jsonSend["chatCode"] = xChat.chatType;
		jsonSend["chatMsg"] = std::to_string(xChat.chatMsg);

		int userId = pTUser->GetSUserInfo().baseInfo.userId;
		m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::MSGID_CLIENT_OTHER_CHAT_NOTIFY, jsonSend, userId, tusWaitNextRound, m_sitUserList);
	}


    return 0;
}

int NFGameTable::ProcClientBet(const nlohmann::json& p, int chairIndex)
{
    if (!IsChairIndexValid(chairIndex)) // TODO
        return -1;

	NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[chairIndex]);
	if (pTUser == NULL)
	{
		m_pLogModule->LogError("find table user failed!", __FUNCTION__, __LINE__);
		return -1;
	}

    //记录没有操作的次数（清零）
    pTUser->initNoOperationNum();
	int32_t retCode = 0;
	std::string retMsg = "";
	NFGamespace::ClientBet xBet;
	try
	{
		xBet = p;
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
	}

	if (GetCurStateEnum() > 3 || GetCurStateEnum() < 2)
	{
		// TODO 
		//ThrowException(ERROR_CODE_NOTBETTINGTIME, ERROR_CODE_DESCRIPTION_LIST[ERROR_CODE_NOTBETTINGTIME]);
	}

	if ((pTUser->GetBetSum() + xBet.betNum) > pTUser->GetSUserInfo().baseInfo.bean)
	{
		//ThrowException(ERROR_CODE_KICK_LESSBEAN, ERROR_CODE_DESCRIPTION_LIST[ERROR_CODE_KICK_LESSBEAN]);
	}
	this->DoUserBet(pTUser, xBet.id, xBet.betNum);

    ////发送响应包
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_G_BET_RESP;
	jsonSend["code"] = retCode;
	jsonSend["msg"] = retMsg;
	m_pTableManagerModule->SendMailToTableUser(NFMsg::MSGID_CLIENT_G_BET_RESP, jsonSend, pTUser->GetSUserInfo().self);
    return 0;
}

void NFGameTable::ClientUpdateUserInfo(NFGamespace::DB_UserBaseInfo& baseInfo, int chairIndex)
{
    if (!IsChairIndexValid(chairIndex))
    {
        m_pLogModule->LogError("chairIndex error", __FUNCTION__, __LINE__);
        return;
    }
	NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[chairIndex]);
    if (tusNone == pTUser->GetUsTate() || baseInfo.userId != pTUser->GetSUserInfo().baseInfo.userId)
    {
        m_pLogModule->LogError("no user or userId error", __FUNCTION__, __LINE__);
        return;
    }

    pTUser->GetSUserInfo().baseInfo.CopyFrom(baseInfo);
    SendUserStateNotify(baseInfo.userId, pTUser->GetUsTate());
}

bool NFGameTable::BeforeStartGame()
{
    //清除已经断线的用户 // TODO
    std::vector<NF_SHARE_PTR<NFTableUser>> closeUserVector;
    std::map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin();
    for (; iter != m_sitUserList.end(); iter++)
    {
		NF_SHARE_PTR<NFTableUser> tempUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
        if (!m_pGameLogicModule->FindUserByUserId(tempUser->GetSUserInfo().baseInfo.userId))
        {
            closeUserVector.push_back(tempUser);
        }
    }

	if (closeUserVector.size() != 0)
	{
		for (auto tempUser : closeUserVector)
		{
			std::ostringstream logClose;
			logClose << "清理断线用户 userId = " << tempUser->GetSUserInfo().baseInfo.userId;
			m_pLogModule->LogInfo(logClose, __FUNCTION__, __LINE__);
			if (tempUser->GetSUserInfo().baseInfo.userId < 0)
				continue;
			ClearUser(*tempUser);
		}
	}
    ////--------------

    if (this->m_userList.size() < MIN_TABLE_USER_COUNT)
    {
        return false;
    }
    else
    {
        //游戏开始时间
        uint64_t stamp = NFUtil::GetTimeStampInt64Ms();
        snprintf(m_gameStartMsStamp, sizeof(m_gameStartMsStamp), "%llu", stamp);

        //TODO 读取配置
        this->m_tabelConfig.readFromConfig();

    }
    m_gameCount++;

    //游戏开始前清理游戏豆不足用户、长期不操作用户 
    int len = (int)m_userList.size();
    for (int i = 0; i < len; i++)
    {
		NF_SHARE_PTR<NFTableUser> pItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[i]);
        if (pItem->GetUsTate() != tusNone)
        {
            //std::ostringstream logStr;
            //logStr << "游戏开始前清理游戏豆不足用户、长期不操作用户，userid = " << pItem->GetSUserInfo().baseInfo.userId << " 不操作次数 : " << pItem->GetNoOperationNum();
            //m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
            if (pItem->GetNoOperationNum() == 4)
            {
                SendForceLeaveNotify(pItem, 17, "您已连续四局不操作");
                //std::ostringstream logStrWar;
                //logStrWar << "---------------- 长时间不操作，警告 ---------------------，userid = " << pItem->GetSUserInfo().baseInfo.userId;
                //m_pLogModule->LogInfo(logStrWar, __FUNCTION__, __LINE__);
            }
            if (pItem->GetNoOperationNum() >= TIMES_NO_OPERATOR)
            {
                ///StartReportUserLog(pItem);
               
                SendForceLeaveNotify(pItem, 10, "您长时间不操作，已被踢出");
                ClearUser(*pItem);
                //std::ostringstream logStrWar;
                //logStrWar << "---------------- 长时间不操作，剔除 ---------------------，userid = " << pItem->GetSUserInfo().baseInfo.userId;
                continue;
            }
            pItem->addNoOperationNum();
            //LogInfo("pTUser->addNoOperationNum()", " userId %d ----------------------- add no operator number:%d",
            //    pItem->GetSUserInfo().baseInfo.userId, pItem->GetNoOperationNum());
        }
    }
    return true;
}

void NFGameTable::InitMatchLog()
{
	uint64_t stamp = NFGetTimeS();
    m_matchLog.Clear();
    m_matchLog.startTime = stamp;
    uint64_t matchId = (uint64_t)(4 * pow(10, 17) + m_handle * pow(10, 13)) + stamp;
    m_matchLog.matchId = matchId;
    for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
    {
		NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
        if (pTUser->GetSUserInfo().baseInfo.userId == -1) continue;
        if (m_matchLog.channelId.length() != 0)
            m_matchLog.channelId += ",";
        m_matchLog.channelId += to_string(pTUser->GetSUserInfo().baseInfo.channelId);
    }
}

void NFGameTable::RandomRobotBet()
{
    if (m_pConfigAreaModule->m_enableCheat == 1)
    {
        //计算本局机器人可下注的金额
        int tempPercent = NFUtil::GetRandomRange(m_pConfigAreaModule->m_robotBetMinPercent, m_pConfigAreaModule->m_robotBetMaxPercent);

        this->m_RobotBetMaxBean = NFUtil::GetRandomRange(200, 2000) *tempPercent / 100;

        //计算每局机器人数量
        //int maxRobotNum = (int)(this->m_RobotBetMaxBean / m_pConfigAreaModule->user_bet_maxnum + 1);
        ////int maxRobotNum = GetRandomRange(1, 10);
        //if (maxRobotNum > 10)
        //    maxRobotNum = 10;

        for (int i = 0; i < 1; ++i)
        {
            int emptyIndex = this->GetEmptyChairIndex();
            if (emptyIndex < 0)
            {
                m_pLogModule->LogError("emptyIndex < 0", __FUNCTION__, __LINE__);
                return;
            }

			NF_SHARE_PTR<SUserInfo> pUserToAdded = m_pRobotMoudle->AllocRobotUser(m_pTableManagerModule->GetSelScoreByHandle(m_handle));
            if (pUserToAdded)
            {
                if (!this->EnterTable(pUserToAdded, emptyIndex, true))
                {
                    m_pRobotMoudle->FreeRobotUser(pUserToAdded->baseInfo.userId);
                    return;
                }
				NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[emptyIndex]);
                if (pTUser != NULL && pTUser->GetIsRobot())
                {
                    std::ostringstream logStr;
                    logStr << "加入机器人 RobotID = " << pTUser->GetSUserInfo().baseInfo.userId;
                    m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
                    this->m_RobotList.push_back(pTUser);
                }
            }
            else
            {
                return;
            }
        }
    }
}

int NFGameTable::GetEmptyChairIndex()
{
    int maxLen = (int)m_userList.size();
    // 优先进入空位置
    for (int i = 0; i < maxLen; ++i)
    {
		NF_SHARE_PTR<NFTableUser> pItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[i]);
        if (tusNone == pItem->GetUsTate() && !pItem->GetIsRobot())
        {
            return i;
        }
    }

    return -1;
}


void NFGameTable::DoUserBet(NF_SHARE_PTR<NFTableUser>& pTUser, int betId, int betNum)
{
    if (pTUser->GetBetSum() >= pTUser->GetSUserInfo().baseInfo.bean) // TODO
        return;

	BetRegion* region = this->m_regions.getRegionById(betId);
	if (region == nullptr)
	{
		return;
		//ThrowException(ERROR_CODE_NOSUCHREGION, ERROR_CODE_DESCRIPTION_LIST[ERROR_CODE_NOSUCHREGION]); TODO
	}
		

    if (!m_pGameLogicModule->FindUserByUserId(pTUser->GetSUserInfo().baseInfo.userId))
        return;

	pTUser->AddBet(betNum, betId);
	region->AddBetNum(betNum);

    int64_t allSum = this->m_regions.getBetAllSum() + this->m_regions.getRobotBetAllSum();

    //// TODO 记录日志
    if (!pTUser->GetIsRobot() && betNum > 0)
    {
        int userId = pTUser->GetSUserInfo().baseInfo.userId;
        std::map<int, std::vector<NF_SHARE_PTR<SActionLogItem>>>::iterator iterItem = m_matchLog.actionLog.find(userId);
        bool isHaveBetId = false;
        size_t index = -1;

        uint64_t currentTimeStamp = NFUtil::GetTimeStampInt64Sec();

        if (iterItem != m_matchLog.actionLog.end())
        {
            NF_SHARE_PTR<SActionLogItem> item = make_shared<SActionLogItem>();
            item->userId = userId;
            item->betBean = betNum;
            item->betArea = betId;
            item->timestamp = currentTimeStamp - m_matchLog.startTime;
            iterItem->second.push_back(item);
        }
        else
        {
            std::vector<NF_SHARE_PTR<SActionLogItem>> actionLogVec;
            NF_SHARE_PTR<SActionLogItem> item = make_shared<SActionLogItem>();
            item->userId = userId;
            item->betBean = betNum;
            item->betArea = betId;
            item->timestamp = currentTimeStamp - m_matchLog.startTime;
            actionLogVec.push_back(item);
            m_matchLog.actionLog.insert(std::make_pair(userId, actionLogVec));
        }
    }

    ///发送下注广播包
	try
	{
		nlohmann::json jsonSend;

		jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_OTHER_BET_NOTIFY;
		jsonSend["userId"] = pTUser->GetSUserInfo().baseInfo.userId;

		nlohmann::json betItemObject;
		betItemObject["id"] = betId;
		betItemObject["betSum"] = betNum;

		nlohmann::json jsonObject = nlohmann::json::object({ {"otherBets",betItemObject} });

		jsonSend.insert(jsonObject.begin(), jsonObject.end());

		m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::baccarat_MSGID_CLIENT_OTHER_BET_NOTIFY, jsonSend, INVALID_USERID, tusWaitNextRound, m_sitUserList);
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
	}
}

// 离开桌子
bool NFGameTable::LeaveTable(const int userId)                                                // 离开座位，可能引起断线
{
    m_pGameLogicModule->DelUserFromLeaveTableList(userId);
    bool isOffline = false;
    NF_SHARE_PTR<NFTableUser> pTUser = OnlyNomalLeaveTable(userId, isOffline);

    if (!pTUser)
        return false;

    if (isOffline)
    {
        pTUser->SetUsState(tusOffline);
        pTUser->SetOfflineTick(m_SystemTick.GetTimePassMillionSecond());
    }
    else
    {

        //检查是否满足游戏人数
        if (this->m_sitUserList.size() < MIN_TABLE_USER_COUNT)
        {

            //this->SendBroadTableWaitingUsers();
        }
    }
    return true;
}

// 正常离开桌子
NF_SHARE_PTR<NFTableUser> NFGameTable::OnlyNomalLeaveTable(int userId, bool& mayOffline)               // 只可以正常离开，不会引起断线
{
    mayOffline = false;
    NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(FindUserById(userId));
    if (!pTUser)
    {
        m_pLogModule->LogError("not find user", __FUNCTION__, __LINE__);
        return nullptr;
    }
    if (pTUser->GetUsTate() > tusNomal)
    {
        m_pLogModule->LogError("user state not normal", __FUNCTION__, __LINE__);
        return NULL;
    }

    if (IsGaming())
    {
        if (!CanLeaveWhenGaming(pTUser))
            mayOffline = true;
    }

    if (!mayOffline)
    {
        // StartReportUserLog(pTUser);
        ClearUser(*pTUser);
    }

    return pTUser;
}

// 根据id查找用户
NF_SHARE_PTR<NFTableUser> NFGameTable::FindUserById(int userId)                  // 根据id查找用户，入座的用户
{
    map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.find(userId);
    if (m_sitUserList.end() == iter)
        return NULL;

	auto temp = NFUtil::Cast<NFTableUser, NFITableUserModule>( iter->second);
	return temp;
}

// 是否正在游戏
bool NFGameTable::IsGaming()
{
    TableState currentState = GetCurStateEnum<TableState>();
    return ((currentState > TableState::TABLE_WAIT_GAME_START && currentState < TableState::TABLE_WAIT_FOR_NEXT_GAME)
        || currentState == TableState::TABLE_WITH_ANIMATION || currentState == TableState::TABLE_CONTINUE);
}

// 是否在游戏中途离开
bool NFGameTable::CanLeaveWhenGaming(NF_SHARE_PTR<NFITableUserModule> pTUser)
{
	if(!pTUser)
	{
		return false;
	}
	NF_SHARE_PTR<NFTableUser> pTuser = NFUtil::Cast<NFTableUser, NFITableUserModule>(pTUser);
    // 百家乐：没下注可以直接离开
    // 梭哈：下局才进入游戏可以离开
    TableState currentState = GetCurStateEnum<TableState>();
    if ((currentState >= TableState::TABLE_WAGER_1 && currentState <= TableState::TABLE_CALCULATE_RESULT) || currentState == TableState::TABLE_WITH_ANIMATION)
    {
        if (pTuser->GetBetSum() > 0)
            return false;//(pTUser->ustate>=tusWaitNextRound);//(pTUser->ustate < tusNomal);
        else
            return true;
    }
    else
        return true;
}

// 清理用户数据
void NFGameTable::NoUserClearData()
{
    // 桌子的人都离开，清理信息，比如统计信息
    m_gameCount = 0;
    EnterDefaultState();
    //m_tstate = TableState::TABLE_IDLE;
    m_RankList_bet.clear();
    m_RankList_win.clear();
    m_tabelConfig.Clear();

}

// 每回合清除数据
void NFGameTable::RoundStopClearData()
{
    // 每局结束清理信息，比如断线和逃跑玩家、游戏状态
	m_endGameTick = m_SystemTick.GetTimePassMillionSecond();



    this->m_addedStock = 0;
    this->m_revenueAmount = 0;
    this->m_controlValue = -1;
    this->m_getControlTimes = 0;

    // 清理游戏数据
    for (auto &temp : m_userList)
    {
		auto item = NFUtil::Cast<NFTableUser, NFITableUserModule>(temp);
        //NF_SHARE_PTR<CTableUser> pItem = m_userList[i];
        //LogInfo("清理玩家数据", "userID = %d  状态 %d", item->userInfo.baseInfo.userId, item->ustate);
        if (-1 == item->GetSUserInfo().baseInfo.userId)
            continue;
        if (item->GetUsTate() != tusNone)
        {

            item->SetIsReady(0);
            item->SetIsTrust(false);
            item->SetDisCardCount(0);
            item->SetLastDisCardTick(0);
            item->SetEnterTableTick(m_SystemTick.GetTimePassMillionSecond());
            item->ClearPlayerUserinfo();
            item->ClearBetItems();
            item->SetRoundIncBean(0);
            item->SetRoundRevenun(0);

            //item->ClearBetItems();
            if (tusWaitNextRound == item->GetUsTate())
            {

                SendBroadTableRecord(item->GetSUserInfo().self);

                item->SetUsState(tusNomal);

                SendUserStateNotify(item->GetSUserInfo().baseInfo.userId, item->GetUsTate());
            }
        }
    }
    this->m_regions.clear();

    for (size_t i = 0; i < m_RobotList.size(); i++)
    {
		auto tmp = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_RobotList[i]);
        ClearUser(*tmp);

    }
    this->m_RobotList.clear();
    this->m_RobotBetMaxBean = 0;

    int len = (int)m_userList.size();
    // 清理断线用户
    for (int i = 0; i < len; i++)
    {
        NF_SHARE_PTR<NFTableUser> pItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[i]);
        if (pItem->GetUsTate() > tusNomal)
        {
            std::ostringstream logClose;
            logClose << "清理断线用户 userId = " << pItem->GetSUserInfo().baseInfo.userId;
            m_pLogModule->LogInfo(logClose, __FUNCTION__, __LINE__);
            ClearUser(*pItem);
        }
    }
}

void NFGameTable::SendAllInfoToUser(std::shared_ptr<SUserInfo> toPUser, std::shared_ptr<SUserInfo> ignoreUser)
{
	uint16_t len = (uint16_t)this->m_sitUserList.size(); // TODO

	if (ignoreUser)
	{
		for (auto &kvp : this->m_sitUserList)
		{
			auto tableUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(kvp.second);

			if (tableUser->GetSUserInfo().baseInfo.userId == ignoreUser->baseInfo.userId)
			{
				len--;
			}
		}
	}

	NF_SHARE_PTR<NFTableUser> toTableUser = this->FindUserById(toPUser->baseInfo.userId);
	if (!toTableUser)
	{
		m_pLogModule->LogError("not found user by userid", __FUNCTION__, __LINE__);
		return;
	}

	nlohmann::json jsonSend;
	try
	{
		jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_TABLEALLINFO;
		jsonSend["tableId"] = this->m_handle;
		jsonSend["tableState"] = this->m_tstate;
		jsonSend["decTime"] = this->m_decTimeCount;
		jsonSend["betAllSum"] = (uint64_t)toTableUser->GetBetAllSum();

		this->m_regions.writeBetSumToPluto(jsonSend);
		toTableUser->WriteBetItemsToPluto(jsonSend);

		jsonSend["userState"] = toTableUser->GetUsTate();

		//牌的信息
		nlohmann::json jsonCardArray;

		nlohmann::json dealerJson;
		dealerJson["card_id"] = m_regions.GetCardsOfCard_ID(CardID::DEALER_ID)->getSelfID();
		m_regions.GetCardsOfCard_ID(CardID::DEALER_ID)->WriteToPluto(dealerJson);
		jsonCardArray.push_back(dealerJson);

		nlohmann::json notDealerJson;
		notDealerJson["card_id"] = m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID)->getSelfID();
		m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID)->WriteToPluto(notDealerJson);
		jsonCardArray.push_back(notDealerJson);

		nlohmann::json jsonCardAryObj = nlohmann::json::object({ {"cardList",jsonCardArray} });

		jsonSend.insert(jsonCardAryObj.begin(), jsonCardAryObj.end());
	}
	catch (const nlohmann::detail::exception & ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
		return;
	}

	m_pTableManagerModule->SendMailToTableUser(NFMsg::baccarat_MSGID_CLIENT_TABLEALLINFO, jsonSend, toTableUser->GetSUserInfo().self);

	SendTableStateToOneUser(*toTableUser, GetCurStateEnum<TableState>(), tusWaitNextRound);
	SendBroadTableRecord(toTableUser->GetSUserInfo().self);
}

void NFGameTable::ActiveTableToGameState()
{
	m_lastRunTick.SetNowTime();
	m_endGameTick = m_SystemTick.GetTimePassMillionSecond();
    for (vector<NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_userList.begin(); iter != m_userList.end(); ++iter)
    {
		auto temp = NFUtil::Cast<NFTableUser, NFITableUserModule>(*iter);
        temp->SetReadyOrLeaveTick(m_SystemTick.GetTimePassMillionSecond());
    }

    if (!m_isActive && GetCurStateEnum() == TableState::TABLE_IDLE)
    {
        m_isActive = true;
        if (GetCurStateEnum() == TableState::TABLE_IDLE)
        {
            this->m_decTimeCount = 3;
            ChangeTabelStateTo(TableState::TABLE_WAIT_GAME_START);
        }
    }

}

void NFGameTable::AddUser(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex, int isRobot)
{
    pUser->activeInfo.userState = EUS_INTABLE;
    pUser->activeInfo.tableHandle = m_handle;
    pUser->activeInfo.chairIndex = chairIndex;

    int userId = pUser->baseInfo.userId;
    if (m_sitUserList.find(userId) == m_sitUserList.end())
    {
		m_userList[chairIndex]->GetSUserInfo().self = pUser->self;
        m_sitUserList.insert(make_pair(userId, m_userList[chairIndex]));
        m_pTableManagerModule->AddSitUser(userId, m_handle, chairIndex);

        if (!isRobot)
        {
            //用户进入桌子, 给用户上锁

            std::ostringstream logStr;
            logStr << "user enter table lock!!!!!!, userId = " << userId << " isRobot " << isRobot;
            m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
            //给 用户上锁
            m_pGameLogicModule->GameMsgSenderLock(m_handle, userId, m_pConfigAreaModule->gameRoomId);
        }
        else{
            m_pLogModule->LogInfo("robot !!!!!!!!!!!!!!!!!!!!!", __FUNCTION__, __LINE__);
        }
    }
}


bool NFGameTable::EnterTable(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex, bool isRobot)
{

    NF_SHARE_PTR<NFTableUser> pTOldUser = FindUserById(pUser->baseInfo.userId);
    NF_SHARE_PTR<NFTableUser> pTSitUser = FindUserByChairIndex(chairIndex);
    if (!pTSitUser)
    {
        m_pLogModule->LogError("cannot find chairIndex", __FUNCTION__, __LINE__);
        return false;
    }

    // 真人可以替换机器人，机器人不能替换真人。
    if (!isRobot /*&& !IsGaming()*/)
    {
        if (tusNone != pTSitUser->GetUsTate() && pTSitUser->GetIsRobot())
        {
            for (vector<NF_SHARE_PTR<NFITableUserModule>>::iterator it = m_RobotList.begin(); it != m_RobotList.end(); ++it)
            {
				auto temp = NFUtil::Cast<NFTableUser, NFITableUserModule>(*it);
                if ((temp)->GetSUserInfo().baseInfo.userId == pTSitUser->GetSUserInfo().baseInfo.userId)
                {
                    std::ostringstream logStr;
                    logStr << "------------------------------ remove robot userId = " << pUser->baseInfo.userId
                        << " robot Id = " << pTSitUser->GetSUserInfo().baseInfo.userId << "  chairIndex = " << chairIndex;
                    m_pLogModule->LogWarning(logStr, __FUNCTION__, __LINE__);
                    m_RobotList.erase(it++);
                    break;
                }
            }

            std::ostringstream logStr;
            logStr << "replace robot realUser = " << pUser->baseInfo.userId << " robot = " << pTSitUser->GetSUserInfo().baseInfo.userId;
            m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
            ClearUser(*pTSitUser);
        }
        else{
            std::ostringstream logStr;
            logStr << "not change user robot ?? userId == " << pTSitUser->GetSUserInfo().baseInfo.userId;
            m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
        }
    }

    if (pTOldUser)
    {
        //// 判断是否可以返回
        if (pTOldUser->GetUsTate() < tusWaitNextRound)
        {
            std::string logStr = "user exists userId = ";
            logStr.append(lexical_cast<string>(pTOldUser->GetSUserInfo().baseInfo.userId));
            m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
            return false;
        }
        if (pTOldUser->GetSUserInfo().baseInfo.userId != pTSitUser->GetSUserInfo().baseInfo.userId)
        {
            m_pLogModule->LogError("offline return chairindex errror", __FUNCTION__, __LINE__);
            return false;
        }


        std::string logStr = "offline return success userid = ";
        logStr.append(lexical_cast<string>(pUser->baseInfo.userId));
        m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
        AddUser(pUser, chairIndex, isRobot);
        if (this->IsGaming())
        {
            std::string logStrr = "pTOldUser->ustate = tusWaitNextRound = ";
            logStrr.append(lexical_cast<string>(pUser->baseInfo.userId));
            m_pLogModule->LogInfo(logStrr, __FUNCTION__, __LINE__);
            pTOldUser->SetUsState(tusNomal);
        }
        else
        {
            pTOldUser->SetUsState(tusNomal);
        }
        ActiveTableToGameState();
        pTOldUser->CopyUserInfoFrom(*pUser);
        //发送桌子信息
        m_pLogModule->LogInfo(lexical_cast<std::string>(pTOldUser->GetBetAllSum()), __FUNCTION__, __LINE__);
        pTOldUser->initNoOperationNum();

        return true;
    }
    else
    {
        if (pTSitUser->GetUsTate() != tusNone)
        {
            std::string logStr = "this chair have a user, charindex = ";
            logStr.append(lexical_cast<string>(chairIndex));
            m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
            return false;
        }

        if (IsGaming())
        {
            if (!GAME_CAN_ENTER_WHEN_GAMING)
            {
                std::string logStr = "this table is gaming,can not enter!!!!  table handle = ";
                logStr.append(lexical_cast<string>(m_handle));
                m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
                return false;
            }

            if (GAME_CAN_DIRECT_GAME)
            {
                pTSitUser->SetUsState(tusNomal);
            }
            else
            {
                pTSitUser->SetUsState(tusWaitNextRound);

                std::ostringstream logStr;
                logStr << "wait the next game!! userId = " << pUser->baseInfo.userId << " table hanle = " << m_handle << " chairIndex = " << chairIndex;
                m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
            }
        }
        else
        {
            pTSitUser->SetUsState(tusNomal);
        }

        ActiveTableToGameState();

        AddUser(pUser, chairIndex, isRobot);
        pTSitUser->CopyUserInfoFrom(*pUser);
        pTSitUser->SetIsRobot(isRobot);
        pTSitUser->SetEnterTableTick(m_SystemTick.GetTimePassMillionSecond());
        if (isRobot)
        {
            /*std::ostringstream logStr;
            logStr << "robot enter table!!!!!!! user id = " << pUser->baseInfo.userId;
            m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);*/
        }
        else
        {
            //std::ostringstream logStr;
            //logStr << "not robot enter table!!!!!!! user id = " << pUser->baseInfo.userId;
            //m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
            SendBroadToUserOtherEnterRoom(*pUser);
        }

        // 记录进入桌子时的金币(断线重连则不更新)
        if (pTSitUser->GetEnterBean() == 0)
        {
            pTSitUser->SetEnterBean(pTSitUser->GetSUserInfo().baseInfo.bean);

            pTSitUser->SetEnterTime(NFGetTimeS()); 
        }
        pTSitUser->initNoOperationNum();
        if (pTSitUser->GetUsTate() == tusWaitNextRound)
        {
            SendTableStateToOneUser(*pTSitUser, TableState::TABLE_IDLE, -1);
            SendBroadTableRecord(pTSitUser->GetSUserInfo().self);
        }
        // 发送排行榜
        UpdateAndSendRankListDate();

        return true;
    }
}

// 剔除用户
void NFGameTable::ClearUser(NFTableUser& pTUser)
{
    int userId = pTUser.GetSUserInfo().baseInfo.userId;
    m_sitUserList.erase(userId);
    m_pTableManagerModule->RemoveSitUser(userId);
    if (pTUser.GetIsRobot())
        m_pRobotMoudle->FreeRobotUser(userId);
    else
    {
        //用户离线/关进程,但是 桌子不在游戏中,那么发解锁包
        std::ostringstream logStrunLocck;
        logStrunLocck << "IsUnLock ? userid = " << userId;
        m_pLogModule->LogInfo(logStrunLocck, __FUNCTION__, __LINE__);

        if (-1 != userId)
        {
            if ((!IsGaming() && !pTUser.GetIsAbnormal()) || pTUser.GetBetSum() <= 0)
            {
                // 结算异常不解锁
                std::ostringstream logunlockNot;
                logunlockNot << "unlock! userId = " << userId << " RoomId = " << m_pConfigAreaModule->gameRoomId;
                m_pLogModule->LogInfo(logunlockNot, __FUNCTION__, __LINE__);
                m_pGameLogicModule->GameMsgSenderUnlock(m_handle, userId, m_pConfigAreaModule->gameRoomId);
            }
            else
            {
                if (IsGaming())
                {
                    std::string logStr = "gameing!!!! usreid = ";
                    logStr.append(lexical_cast<string>(userId));
                    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
                }

                if (pTUser.GetIsAbnormal())
                {
                    m_pLogModule->LogInfo("account error", __FUNCTION__, __LINE__);
                }
                else
                {
                    m_pLogModule->LogInfo("account normal", __FUNCTION__, __LINE__);
                }
            }

            
        }
        SendBroadToUserOtherLeaveRoom(pTUser.GetSUserInfo().baseInfo.userName);
    }


	NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByUserId(userId);
    if (pUser)
    {
        pUser->activeInfo.userState = EUS_AUTHED;
        pUser->activeInfo.tableHandle = -1;
        pUser->activeInfo.chairIndex = -1;
    }
    pTUser.Clear();

    UpdateAndSendRankListDate();

    if (GetRealUserCount() <= 0)
    {
        //m_isActive = false;
        NoUserClearData();
    }

    std::ostringstream logStr;
    logStr << "client userId = " << userId << " left user count = " << m_sitUserList.size();
    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
}

void NFGameTable::StartReportScore()
{
	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		m_pLogModule->LogError("DBServer Error!!!", __FUNCTION__, __LINE__);
		return;
	}

    //创建任务
	NF_SHARE_PTR<CAreaTaskItemBase> task = make_shared<CAreaTaskReportScore>(m_handle);
	m_pQsTaskScheduleModule->AddTask(task);

	uint32_t areaId = m_pConfigAreaModule->gameRoomId;
    char openSeriesNum[100];
    uint64_t stamp = NFUtil::GetTimeStampInt64Ms();
    snprintf(openSeriesNum, sizeof(openSeriesNum), "%uA%dA%llu", areaId, m_handle, stamp);

    uint16_t len = 0;
    for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
    {
        NF_SHARE_PTR<NFTableUser> pTItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
        if (pTItem->GetUsTate() >= tusWaitNextRound  && !pTItem->GetIsRobot() && pTItem->GetSUserInfo().baseInfo.userId && pTItem->GetBetVector().size() > 0)
            len++;
    }

    for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
    {
        NF_SHARE_PTR<NFTableUser> pTItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
        if (pTItem->GetUsTate() >= tusWaitNextRound  && pTItem->GetSUserInfo().baseInfo.userId  && pTItem->GetBetVector().size() > 0)
        {
            if (pTItem->GetIsRobot())continue;
            //计算游戏结果
            int tempAllBean = 0;
            for (auto tempBetItem : pTItem->GetBetVector())
            {
                int tempBeanForArea = tempBetItem->betNum;
                map<int, float>::iterator it = resultForArea.find(tempBetItem->betId);
                if (it != resultForArea.end())
                {
                    int tempValue = (int)((float)tempBeanForArea * it->second);
                    tempAllBean += tempValue;
                }
                else  //输
                {

                    if (IsHaveResultArea(WagerAreaType::DOGFALL))
                    {
                        if (tempBetItem->betId == WagerAreaType::DEALER || tempBetItem->betId == WagerAreaType::NOT_DEALEAR)
                        {
                            continue;
                        }
                    }
                    tempAllBean -= tempBeanForArea;
                }
            }
            if (tempAllBean > 0)
            {
                pTItem->SetRoundIncBean(tempAllBean);
                pTItem->SetRoundRevenun((int64_t)(tempAllBean * (1000 - m_pConfigAreaModule->revenue) / 1000));
            }
            else
            {
                pTItem->SetRoundIncBean(tempAllBean);
                pTItem->SetRoundRevenun(0);
            }
        }
    }

    ReportSocreLog(task->GetTaskId(), areaId, m_pConfigAreaModule->gameRoomId, 0, string(openSeriesNum), string(m_gameStartMsStamp));
}

void NFGameTable::ReportSocreLog(uint32_t taskId, uint32_t areaId, int32_t gameRoomId, int32_t isLockGame, string openSeriesNum, string gameStartMsStamp)
{
    int32_t retCode = 0;
    string retMsg = "";
    char buffer[1024];
    string strData("[");

    int count = 0;
    for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
    {
        NF_SHARE_PTR<NFTableUser> pTItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
        if (pTItem->GetUsTate() >= tusWaitNextRound  && pTItem->GetSUserInfo().baseInfo.userId  && pTItem->GetBetVector().size() > 0)
        {
            if (pTItem->GetIsRobot())continue;

            std::ostringstream logStr;
            logStr << "userID = " << pTItem->GetSUserInfo().baseInfo.userId << ";RoundIncBean = " << pTItem->GetRoundIncBean()
                << ";RoundRevenue = " << pTItem->GetRoundRevenue() << ";AllBet = " << pTItem->GetActiveAllBet() << ";ValidBet = " << pTItem->GetActiveValidBet();
            m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

            if (count != 0)
                strData += ",";

            count++;
            snprintf(buffer, sizeof(buffer), "{\"userId\": %d, \"bean\":%lld, \"revenue\":%lld,\"totalBet\":%lld,\"validBet\":%lld}", pTItem->GetSUserInfo().baseInfo.userId, pTItem->GetRoundIncBean(), pTItem->GetRoundRevenue(), pTItem->GetActiveAllBet(), pTItem->GetActiveValidBet());
            strData += buffer;
        }
    }
    strData += "]";


	uint64_t stamp = NFUtil::GetTimeStampInt64Sec();
    snprintf(buffer, sizeof(buffer), "%llu", stamp);
    string strStamp(buffer);
    snprintf(buffer, sizeof(buffer), "%u", areaId);
    string strServerId(buffer);
    snprintf(buffer, sizeof(buffer), "%d", C_GAME_ID);
    string strGameId(buffer);
    string checkCode = MD5(strGameId + "50003" + openSeriesNum + strData + strStamp + m_pConfigAreaModule->m_cis_key).toStr();
    string postData("action=ReportGameResult&gameId=");
    postData += strGameId;
    postData += "&gameServerId=";
    postData += strServerId;
    postData += "&gameRoomId=";
    postData += "50003";
    postData += "&openSeriesNum=";
    postData += openSeriesNum;
    postData += "&data=";
    postData += strData;
    postData += "&startTime=";
    postData += gameStartMsStamp;
    postData += "&timestamp=";
    postData += strStamp;
    postData += "&checkCode=";
    postData += checkCode;

    m_pLogModule->LogInfo(postData, __FUNCTION__, __LINE__);

	if (m_pNFIGameServerToDBModule)
	{
		short dataLen = postData.size();
		m_pNFIGameServerToDBModule->SendDataToDb( NFMsg::MSGID_DBMGR_REPORT_SCORE_TODB,taskId, gameRoomId, dataLen, postData.c_str());

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}
}

void NFGameTable::StartReportMatchLog()
{
	NF_SHARE_PTR<CAreaTaskItemBase> task = make_shared<CAreaTaskMatchLog>();
	m_pQsTaskScheduleModule->AddTask(task);

    uint32_t areaId = -1;
    int32_t gameRoomId = m_pConfigAreaModule->gameRoomId;
    int32_t isLockGame = 0;

    m_matchLog.userCount = GetRealUserCount();
    m_matchLog.handle = m_handle;
    m_matchLog.selScore = m_pTableManagerModule->GetSelScoreByHandle(m_handle);
    m_matchLog.tax = this->m_tabelConfig.sys_revenue_scale;
    m_matchLog.bankerCards = this->m_regions.GetCardsOfCard_ID(CardID::DEALER_ID)->GetCardsStr();
    m_matchLog.notBankerCards = this->m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID)->GetCardsStr();
    std::string resultForAreaStr = "";

    std::map<int, std::vector<std::shared_ptr<BetItem>>> mapBet;
    for (auto temp : m_matchLog.actionLog)
    {
        for (auto value : temp.second)
        {
            std::shared_ptr<BetItem> bet = make_shared<BetItem>();
            bet->betId = value->betArea;
            if (resultForArea.find(value->betArea) == resultForArea.end())
            {
                if (resultForArea.find(WagerAreaType::DOGFALL) != resultForArea.end())
                {
                    if (value->betArea != WagerAreaType::DEALER && value->betArea != WagerAreaType::NOT_DEALEAR)
                        bet->betNum = -(value->betBean);
                }
                else{
                    bet->betNum = -(value->betBean);
                }
                
            }
            else
            {
                bet->betNum = int(value->betBean * resultForArea[value->betArea]);
            }
            if (mapBet.find(value->userId) == mapBet.end())
            {
                std::vector<std::shared_ptr<BetItem>> tempVector;
                tempVector.push_back(bet);
                mapBet.insert(make_pair(value->userId, tempVector));
            }
            else
            {
                bool isFind = false;
                for (auto tempPair : mapBet)
                {
                    for (size_t i = 0; i < tempPair.second.size(); i++)
                    {
                        if (tempPair.second[i]->betId == bet->betId)
                        {
                            int tempValue = tempPair.second[i]->betNum + bet->betNum;
                            tempPair.second[i]->betNum = tempValue;

                            isFind = true;
                        }
                    }
                }
                if (!isFind)
                {
                    mapBet[value->userId].push_back(bet);
                }
            }
        }
    }


    auto mapBetIter = mapBet.begin();
    for (; mapBetIter != mapBet.end(); /*mapBetIter++*/)
    {
        std::string resultForAreaItem = "[";
        resultForAreaItem.append(std::to_string(mapBetIter->first));
        resultForAreaItem.append(",");
        for (size_t i = 0; i < mapBetIter->second.size(); i++)
        {
            resultForAreaItem.append(std::to_string(mapBetIter->second[i]->betId));
            resultForAreaItem.append(":");
            resultForAreaItem.append(std::to_string(mapBetIter->second[i]->betNum));
            if (i + 1 < mapBetIter->second.size())
            {
                resultForAreaItem.append(",");
            }
        }
        resultForAreaItem.append("]");

        if (++mapBetIter != mapBet.end())
            resultForAreaItem.append(",");
        resultForAreaStr.append(resultForAreaItem);
    }

    m_matchLog.resultForArea = resultForAreaStr;

    ReportMatchLog(task->GetTaskId(), areaId, gameRoomId, isLockGame);

    m_matchLog.Clear();
}

void NFGameTable::ReportMatchLog(uint32_t taskId, uint32_t areaId, int32_t gameRoomId, int32_t isLockGame)
{
    uint64_t gameTime = m_matchLog.endTime - m_matchLog.startTime;
    char buffer[400];
    string strUserLog("[");
    for (map<int, NF_SHARE_PTR<SInfoLogItem>>::iterator it = m_matchLog.infoLog.begin(); it != m_matchLog.infoLog.end(); it++)
    {
        NF_SHARE_PTR<SInfoLogItem> item = it->second;
       
        
        snprintf(buffer, sizeof(buffer), "{\"ChannelID\": %d, \"ChairID\": %u, \"UserID\": %u, \"KindID\": %u, \"ServerID\": %d, \"TableID\": %u, \"Score\": %d, \"SysRecv\": %u, \"AllBet\": %d, \"ValidBet\": %d, \"PlayType\": 0, \"PlayTime\": %llu,\"EndGameTime\": %llu, \"Account\": \"%s\", \"NickName\": \"%s\"}", item->channelId, item->chairId, item->userId, 10020, m_matchLog.selScore, m_matchLog.handle, item->score, m_matchLog.tax, item->allBet, item->validBet, m_matchLog.endTime - m_matchLog.startTime, m_matchLog.endTime, item->username.c_str(), item->nickname.c_str());
        
        if (it != m_matchLog.infoLog.begin())
        {
            strUserLog += ",";
        }
        
        strUserLog += buffer;
    }

    strUserLog += "]";

    string opValue("\"");
    auto iterActionLog = m_matchLog.actionLog.begin();

    for (; iterActionLog != m_matchLog.actionLog.end(); iterActionLog++)
    {
        std::vector<NF_SHARE_PTR<SActionLogItem>> actionLogVec = iterActionLog->second;
        std::string tempStr = "";

        for (size_t i = 0; i < actionLogVec.size(); i++)
        {
            NF_SHARE_PTR<SActionLogItem> item = actionLogVec[i];
            tempStr += std::to_string(item->timestamp) + ":" + std::to_string(item->betArea) + ":" + std::to_string(item->betBean);
            
            if (i + 1 < actionLogVec.size())
                tempStr += ",";
        }
        if (iterActionLog != m_matchLog.actionLog.begin())
        {
            opValue += ",";
        }
        snprintf(buffer, sizeof(buffer), "[%u-%s]", iterActionLog->first, tempStr.c_str());
        opValue += buffer;
    }

    opValue += ";";
    opValue += m_matchLog.resultForArea;
    opValue += ";";
    opValue += m_matchLog.resultForAll;
    opValue += "\"";

    snprintf(buffer, sizeof(buffer), "%lld", m_matchLog.matchId);
    string strMatchId(buffer);
    snprintf(buffer, sizeof(buffer), "%llu", gameTime);
    string strGameTime(buffer);
    snprintf(buffer, sizeof(buffer), "%d", m_matchLog.userCount);
    string strUserCount(buffer);
    snprintf(buffer, sizeof(buffer), "%u", m_matchLog.handle);
    string strHandle(buffer);
    snprintf(buffer, sizeof(buffer), "%u", m_matchLog.tax);
    string strTax(buffer);
    snprintf(buffer, sizeof(buffer), "%lld", m_matchLog.startTime);
    string strStartTime(buffer);
    snprintf(buffer, sizeof(buffer), "%lld", m_matchLog.endTime);
    string strEndTime(buffer);

    string strMatchLog("{\"GameTime\": ");
    strMatchLog += strGameTime;

    strMatchLog += ", \"EndTime\": ";
    strMatchLog += strEndTime;

    strMatchLog += ", \"TableID\": ";
    strMatchLog += strHandle;

    strMatchLog += ", \"KindID\": 10020";
    strMatchLog += ", \"PlayerChannels\": \"";
    strMatchLog += m_matchLog.channelId;
    strMatchLog += "\"";

    strMatchLog += ", \"ServerID\":" + std::to_string(m_matchLog.selScore);
    strMatchLog += ", \"RoundCount\":0 ";

    strMatchLog += ", \"UserCount\": ";
    strMatchLog += strUserCount;

    strMatchLog += ", \"TaxCount\": ";
    strMatchLog += strTax;

    strMatchLog += ", \"CardValue\": ";
    //strMatchLog += ", \"CardValue\":\"\" ";
    strMatchLog += "\"[";
    strMatchLog += m_matchLog.bankerCards;
    strMatchLog += "],[";
    strMatchLog += m_matchLog.notBankerCards;
    strMatchLog += "]\"";

    strMatchLog += ", \"OpValue\": ";
    strMatchLog += opValue;

    strMatchLog += ", \"GameStartTime\": ";
    strMatchLog += strStartTime;
    strMatchLog += "}";

    string postStr = "{\"data\":" + strMatchLog + ", \"data2\":" + strUserLog + "}";

    std::ostringstream strLog;
    strLog << "MatchLog ===== " << postStr;
    m_pLogModule->LogInfo(strLog, __FUNCTION__, __LINE__);


	if (m_pNFIGameServerToDBModule)
	{
		short dataLen = postStr.size();
		m_pNFIGameServerToDBModule->SendDataToDb( NFMsg::MSGID_DBMGR_MATCH_LOG_TODB,taskId, gameRoomId, dataLen, postStr.c_str());

	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}

}

void NFGameTable::OnMatchReportError(int code, const char* errorMsg)
{
    std::ostringstream strLog;						// TODO
    strLog << "OnMatchReportError:" << code << errorMsg;
    m_pLogModule->LogError(strLog, __FUNCTION__, __LINE__);

    for (auto kv : this->m_sitUserList)
    {
        NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(kv.second);

        if (!pTUser->GetIsRobot())
        {
            // 上报失败，不允许解锁
            pTUser->SetActiveAllBet(true);
            if (pTUser->GetBetAllSum() > 0)
                SendForceLeaveNotify(pTUser, 16, "上报异常");
        }
    }
}

void NFGameTable::OnReportScoreError(int code, const char* errorMsg)
{
	m_pLogModule->LogInfo("------------------------------ OnReportScoreError", __FUNCTION__, __LINE__);
	if (TableState::TABLE_CALCULATE_RESULT != GetCurStateEnum())
	{
		std::string logStr("table state error == ");
		logStr.append(lexical_cast<string>(GetCurStateEnum()));
		m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
		return;
	}

	std::ostringstream logStr;
	logStr << "code = " << code << " errorMsg = " << errorMsg;
	m_pLogModule->LogWarning(logStr, __FUNCTION__, __LINE__);

	for (auto kv : this->m_sitUserList)
	{

		NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(kv.second);

		if (pTUser->GetUsTate() < tusWaitNextRound)
			continue;
		if (pTUser->GetBetVector().size() <= 0)
			continue;
		// 机器人不上报游戏结果
		if (pTUser->GetIsRobot())
			continue;
		else
		{
			// 上报失败，不允许解锁
			pTUser->SetIsAbnormal(true);
			//if (pTUser->GetBetAllSum() > 0)
				//SendForceLeaveNotify(pTUser, ERROR_CODE_REPORT_ERROR, ERROR_CODE_DESCRIPTION_LIST[ERROR_CODE_REPORT_ERROR]);
		}

		// 是否下注
		int isUserBet = 0;
		if (pTUser->GetBetSum() > 0)
			isUserBet = 1;

		// 上报失败自己计算信息
		pTUser->AddUserInfoBean(pTUser->GetRoundIncBean());
		// 同步内存信息
		SUserInfo* pUser = m_pGameLogicModule->GetUserInfoByUserId(pTUser->GetSUserInfo().baseInfo.userId).get();
		if (pUser)
		{
			pUser->baseInfo.bean = pTUser->GetSUserInfo().baseInfo.bean;
			pUser->baseInfo.totalBetCount = pTUser->GetBetSum();
			pUser->baseInfo.winCount = pTUser->GetRoundIncBean();
		}
		NF_SHARE_PTR<SInfoLogItem> item = make_shared<SInfoLogItem>();
		m_matchLog.infoLog.insert(make_pair(pTUser->GetSUserInfo().baseInfo.userId, item));
		item->userId = pTUser->GetSUserInfo().baseInfo.userId;
		item->channelId = pTUser->GetSUserInfo().baseInfo.channelId;
		item->username = pTUser->GetSUserInfo().baseInfo.userName;
		item->nickname = pTUser->GetSUserInfo().baseInfo.nickName;
		item->chairId = pTUser->GetSUserInfo().activeInfo.chairIndex;
		item->startBean = pTUser->GetSUserInfo().baseInfo.bean;
		item->endBean = pTUser->GetSUserInfo().baseInfo.bean;
		item->score = pTUser->GetSUserInfo().activeInfo.score;
		item->allBet = pTUser->GetSUserInfo().activeInfo.allBet;
		item->validBet = pTUser->GetSUserInfo().activeInfo.validBet;
		item->startCards = this->m_regions.GetCardsOfCard_ID(CardID::DEALER_ID)->GetCardsStr();
		item->endCards = this->m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID)->GetCardsStr();

		std::string logStr = "userId = ";
		logStr.append(lexical_cast<string>(pTUser->GetSUserInfo().baseInfo.userId));
		m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

		// 记录玩家日志
		uint64_t stamp = NFUtil::GetTimeStampInt64Sec();
		m_matchLog.endTime = stamp;
	}

	StartReportMatchLog();
	RoundStopClearData();

}

void NFGameTable::OnReportScoreSuccess(const nlohmann::json& p, int index)
{
     m_pLogModule->LogInfo("------------------------------ OnReportScoreSuccess", __FUNCTION__, __LINE__);
     if (TableState::TABLE_CALCULATE_RESULT != GetCurStateEnum<TableState>())
     {
         std::string logStr("table state error == ");
         logStr.append(lexical_cast<string>(GetCurStateEnum()));
         m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
         return;
     }
	 
     SCisScoreReportRetItem tmpReportItem;
	 nlohmann::json pAryJson = p[""].get<nlohmann::json>();
      for (nlohmann::json item : pAryJson)
      {
          int indexItem = 0;
          tmpReportItem.ReadFromVObj(item, indexItem);
 	 
          if (tmpReportItem.userId > 0)
          {
              NF_SHARE_PTR<NFTableUser> pTUser = FindUserById(tmpReportItem.userId);
              if (!pTUser)
              {
                  std::string logStr = "find failed userId = ";
                  logStr.append(lexical_cast<string>(tmpReportItem.userId));
                  m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
              }
              else
              {
				  nlohmann::json aJs;
				  int64_t recentBet = 0;
				  int64_t winCount = 0;
				  try
				  {
					  aJs = nlohmann::json::parse(tmpReportItem.expands);
					  recentBet = aJs["recentBet"].get<int64_t>();
					  winCount = aJs["winCount"].get<int64_t>();
				  }
				  catch (const nlohmann::detail::exception & ex)
				  {
					  m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
				  }
 	 
                  if (pTUser->GetIsRobot())
                  {
                      pTUser->AddUserInfoBean(pTUser->GetRoundIncBean());
                  }
                  else
                  {
                      pTUser->SetRoundIncBean(tmpReportItem.incBean);
                      pTUser->SetUserInfoBean(tmpReportItem.bean);
                  }
                  pTUser->SetUserInfoTotalBetCount(recentBet);
                  pTUser->SetUserInfoWinCount(winCount);
 	 
                  if (!pTUser->GetIsRobot())
                  {
                      // 记录结果日志
                      NF_SHARE_PTR<SInfoLogItem> item = make_shared<SInfoLogItem>();
                      m_matchLog.infoLog.insert(make_pair(pTUser->GetSUserInfo().baseInfo.userId, item));
                      item->channelId = pTUser->GetSUserInfo().baseInfo.channelId;
                      item->chairId = pTUser->GetSUserInfo().activeInfo.chairIndex;
                      item->username = pTUser->GetSUserInfo().baseInfo.userName;
                      item->nickname = pTUser->GetSUserInfo().baseInfo.nickName;
                      item->userId = pTUser->GetSUserInfo().baseInfo.userId;
                      item->startBean = pTUser->GetSUserInfo().baseInfo.bean;
                      item->endBean = tmpReportItem.bean;
                      item->startCards = m_regions.GetCardsOfCard_ID(CardID::DEALER_ID)->GetCardsStr();
                      item->endCards = m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID)->GetCardsStr();
                      //m_pLogModule->LogInfo(lexical_cast<std::string>(pTUser->GetSUserInfo().activeInfo.chairIndex), __FUNCTION__, __LINE__);
                      item->score = pTUser->GetSUserInfo().activeInfo.score;
                      //m_pLogModule->LogInfo(lexical_cast<std::string>(pTUser->GetSUserInfo().activeInfo.allBet), __FUNCTION__, __LINE__);
                      item->allBet = pTUser->GetSUserInfo().activeInfo.allBet;
                      //m_pLogModule->LogInfo(lexical_cast<std::string>(pTUser->GetSUserInfo().activeInfo.validBet), __FUNCTION__, __LINE__);
                      item->validBet = pTUser->GetSUserInfo().activeInfo.validBet;
 	 
                      std::ostringstream logStr;
                      logStr << " userId = " << pTUser->GetSUserInfo().baseInfo.userId;
                      m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
                      uint64_t stamp = NFUtil::GetTimeStampInt64Sec();
                      m_matchLog.endTime = stamp;
                  }
 	 
                  // 同步内存信息
                  NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByUserId(tmpReportItem.userId);
                  if (pUser)
                  {
                      pUser->baseInfo.bean = tmpReportItem.bean;
                      pUser->baseInfo.totalBetCount = recentBet;
                      pUser->baseInfo.winCount = winCount;
                  }
              }
          }
          else
          {
              this->m_tabelConfig.sys_now_stock = tmpReportItem.bean;
              this->m_tabelConfig.sys_revenue_amount = tmpReportItem.score;
              this->m_tabelConfig.sys_revenue_scale = tmpReportItem.experience;
          }
      }
	 
     StartReportMatchLog();
     RoundStopClearData();
}

void NFGameTable::ClearRobotUserIfNoRealUser()
{
    if (m_sitUserList.size() > 0 && 0 == GetRealUserCount())
    {
        // 如果发现剩余都是机器人，删除所有机器人 循环m_userList是因为ClearUser里面m_sitUserList会改变
        for (vector<NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_userList.begin(); iter != m_userList.end(); ++iter)
        {
			NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(*iter);
            if (pTUser->GetUsTate() != tusNone)
            {
                std::ostringstream logStr;
                logStr << "handle = " << m_handle << " robotId = " << pTUser->GetSUserInfo().baseInfo.userId;
                m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
                ClearUser(*pTUser);
            }
        }
    }
}

int NFGameTable::GetEmptyChairAry(int* chairAry)
{
    int len = 0;
    int maxLen = (int)m_userList.size();
    // 优先进入空位置
    for (int i = 0; i < maxLen; ++i)
    {
		NF_SHARE_PTR<NFTableUser> pItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[i]);
        if (tusNone == pItem->GetUsTate())
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
			NF_SHARE_PTR<NFTableUser> pItem = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[i]);
            if (tusNone != pItem->GetUsTate() && pItem->GetIsRobot())
            {
                chairAry[len] = i;
                ++len;
            }
        }
    }
    return len;
}

// 获取真实玩家数量
int NFGameTable::GetRealUserCount()
{
    int ret = 0;
    for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
    {
		auto temp = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
        if (!temp->GetIsRobot())
            ++ret;
    }

    return ret;
}

// 获取当前玩家数量
int NFGameTable::GetCurUserCount() const
{
    return (int)m_sitUserList.size();
}

bool CmpByValue(const NF_SHARE_PTR<RankingUser> lhs, const NF_SHARE_PTR<RankingUser> rhs)
{
    return lhs->bean > rhs->bean;
}
// 更新排行榜
void NFGameTable::UpdateAndSendRankListDate()
{
    if (GetCurStateEnum() == TABLE_IDLE) return;

    size_t maxRankingListCount = 5; // 排行榜显示前5名
    std::vector<NF_SHARE_PTR<RankingUser>> rankingListVector;
    rankingListVector.reserve(5);
	for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = m_sitUserList.begin(); iter != m_sitUserList.end(); ++iter)
	{
		NF_SHARE_PTR<NFTableUser> tmepUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(iter->second);
		if (tmepUser->GetSUserInfo().baseInfo.userId != -1)
		{
			NF_SHARE_PTR<RankingUser> tempRankUser = make_shared<RankingUser>();
			tempRankUser->bean = tmepUser->GetSUserInfo().baseInfo.bean;
			tempRankUser->userName = tmepUser->GetSUserInfo().baseInfo.userName;
			rankingListVector.push_back(tempRankUser);
		}
	}

	if (rankingListVector.size() < maxRankingListCount)
	{
		size_t residueCount = maxRankingListCount - rankingListVector.size();

		for (size_t i = 0; i < residueCount; i++)
		{
			NF_SHARE_PTR<RankingUser> tempRankUser = make_shared<RankingUser>();
			tempRankUser->bean = NFUtil::GetRandomRange(5000, 5000001);
			tempRankUser->userName = "qs" + std::to_string(NFUtil::GetRandomRange(1000000, 2000000));
			rankingListVector.push_back(tempRankUser);
		}
	}

    sort(rankingListVector.begin(), rankingListVector.end(), CmpByValue);

	nlohmann::json jsonSend;

	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_RANK_LIST_INFO;

	nlohmann::json jArray = nlohmann::json::array();

	for (size_t i = 0; i < maxRankingListCount; i++)
	{
		nlohmann::json tempObject;
		tempObject["username"] = rankingListVector[i]->userName;
		tempObject["userBean"] = rankingListVector[i]->bean;
		tempObject["ranking"] = (uint32_t)(i + 1);

		jArray.push_back(tempObject);
	}

	nlohmann::json jObject = nlohmann::json::object({ {"rankingList",jArray} });

	jsonSend.insert(jObject.begin(), jObject.end());

	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::baccarat_MSGID_CLIENT_RANK_LIST_INFO, jsonSend, INVALID_USERID, tusNone, m_sitUserList);
}

void NFGameTable::ResultAreaAndRecord()
{
	CalculateResultOfBetArea(resultForArea);

    RecordTableRecord();
}

void NFGameTable::CalcualteResult()
{
    ResultAreaAndRecord();

    SendResultAreaToUserAtTheSameTable();

    SendResultDataToClient();

    UpdateAndSendRankListDate();
}

void NFGameTable::RecordTableRecord()
{
    if (resultForArea.size() <= 0) return;
    map<int, float>::iterator iter = resultForArea.begin();
    for (; iter != resultForArea.end(); iter++)
    {
        PushBackTableRecord(iter->first, 1);
    }
}

void NFGameTable::SendResultAreaToUserAtTheSameTable()
{
	nlohmann::json jsonSend;

	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_G_FILLCARD_NOTIFY;

    m_regions.WritePlayerCardToPluto(jsonSend);

	nlohmann::json areaJsonArray = nlohmann::json::array();
    // 输赢区域
    map<int, float>::iterator iter = resultForArea.begin();
    for (; iter != resultForArea.end(); iter++)
    {
		areaJsonArray.push_back(iter->first);
    }
	
	nlohmann::json areaJsonObj = nlohmann::json::object({"winAreaList",areaJsonArray });

	jsonSend.insert(areaJsonArray.begin(), areaJsonObj.end());

	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::baccarat_MSGID_CLIENT_G_FILLCARD_NOTIFY, jsonSend, INVALID_USERID, tusWaitNextRound, m_sitUserList);
}

void NFGameTable::SendResultDataToClient()
{
    if (GetCurStateEnum() == TABLE_IDLE) return;

    std::map<int, int> resultForAll;    //记录区域总投注

    for (auto temp : m_sitUserList)
    {
        NF_SHARE_PTR<NFTableUser> tempTableUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(temp.second);
        if (tempTableUser->GetUsTate() != tusNomal)
            continue;

        if (tempTableUser->GetIsRobot())
        {
            for (auto tempBetItem : tempTableUser->GetBetVector())
            {
                if (resultForAll.find(tempBetItem->betId) == resultForAll.end())
                {
                    resultForAll.insert(make_pair(tempBetItem->betId, tempBetItem->betNum));
                }
                else
                {
                    resultForAll[tempBetItem->betId] += tempBetItem->betNum;
                }
            }
            continue;
        }

        int tempAllBean = 0;
        for (auto tempBetItem : tempTableUser->GetBetVector())
        {
            if (resultForAll.find(tempBetItem->betId) == resultForAll.end())
            {
                resultForAll.insert(make_pair(tempBetItem->betId, tempBetItem->betNum));
            }
            else
            {
                resultForAll[tempBetItem->betId] += tempBetItem->betNum;
            }

            int tempBeanForArea = tempBetItem->betNum;
            // 该betid 为 赢
            map<int, float>::iterator it = resultForArea.find(tempBetItem->betId);

            if (it != resultForArea.end())
            {
                int tempValue = (int)((float)tempBeanForArea * it->second);
                tempAllBean += tempValue;
            }
            else  //输
            {
                if (resultForArea.find(WagerAreaType::DOGFALL) != resultForArea.end())
                {
                    if (tempBetItem->betId == WagerAreaType::DEALER || tempBetItem->betId == WagerAreaType::NOT_DEALEAR)
                        continue;
                }
                tempAllBean -= tempBeanForArea;
            }
        }

		if (!m_pGameLogicModule->FindUserByUserId(tempTableUser->GetSUserInfo().baseInfo.userId))
			continue;

		tempTableUser->AddUserInfoBean(tempAllBean);

		tempTableUser->SetActiveScore(tempAllBean);
		tempTableUser->SetActiveAllBet(tempTableUser->GetBetAllSum());

		if (IsHaveResultArea(WagerAreaType::DOGFALL))
		{
			int32_t validBet = tempTableUser->GetBetAllSum()
				- tempTableUser->GetBetSumById(WagerAreaType::DEALER)
				- tempTableUser->GetBetSumById(WagerAreaType::NOT_DEALEAR);
			tempTableUser->SetActiveValidBet(validBet);
		}
		else
		{
			tempTableUser->SetActiveValidBet(tempTableUser->GetBetAllSum());
		}

		nlohmann::json jsonSend;
		jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_G_GAME_RESULT_NOTIFY;
		jsonSend["obtainGoldCoins"] = tempAllBean;
		jsonSend["AllBean"] = tempTableUser->GetSUserInfo().baseInfo.bean;

		m_pGameLogicModule->SendMsgToClient(NFMsg::baccarat_MSGID_CLIENT_G_GAME_RESULT_NOTIFY, jsonSend, tempTableUser->GetSUserInfo().self);
	}

	std::string resultForBetAllStr = "[";
	auto iter = resultForAll.begin();
	for (; iter != resultForAll.end();)
	{
		resultForBetAllStr += std::to_string(iter->first) + ":" + std::to_string(iter->second);
		if (++iter != resultForAll.end())
		{
			resultForBetAllStr += ",";
		}
	}
	resultForBetAllStr += "]";

	m_matchLog.resultForAll = resultForBetAllStr;
}

void NFGameTable::GetGameControlValue()
{
	if (m_pNFIGameServerToDBModule)
	{
		NF_SHARE_PTR<CAreaTaskItemBase> task = make_shared<CAreaTaskGameControl>(m_handle);
		m_pQsTaskScheduleModule->AddTask(task);
		m_pNFIGameServerToDBModule->SendGameControl(task->GetTaskId(), m_pConfigAreaModule->gameRoomId);
	}
	else
	{
		m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
	}
}

void NFGameTable::SendStartGameNotify(int chairIndex)
{
    if (!IsChairIndexValid(chairIndex))
    {
        m_pLogModule->LogError("chairindex error", __FUNCTION__, __LINE__);
        return;
    }
	
    NF_SHARE_PTR<NFTableUser> pTUser = NFUtil::Cast<NFTableUser, NFITableUserModule>(m_userList[chairIndex]);
    if (pTUser->GetIsRobot())
        return;

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_BEGINGAME_NOTIFY;
	jsonSend["selScore"] = m_pTableManagerModule->GetSelScoreByHandle(m_handle);
	jsonSend["SelfBetAllBean"] = pTUser->GetBetAllSum();
    m_pTableManagerModule->SendMailToTableUser(NFMsg::baccarat_MSGID_CLIENT_BEGINGAME_NOTIFY, jsonSend, pTUser->GetSUserInfo().self);
}

//////////////////////////////// 通知 //////////////////////////////////////
//

/*
 * 发送用户状态
 * userId : 用户的id
 * tuserState : 用户的状态
 */
void NFGameTable::SendUserStateNotify(const int userId, const int tuserState)
{
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::MSGID_CLIENT_OTHER_STATE_NOTIFY;
	jsonSend["userId"] = userId;
	jsonSend["userState"] = tuserState;

	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::MSGID_CLIENT_OTHER_STATE_NOTIFY, jsonSend, INVALID_USERID, tusNone, m_sitUserList);
}

/*
 * 发送桌子记录
 * fd : 与用户绑定的socket描述符
 */
void NFGameTable::SendBroadTableRecord(const NFGUID& fd)
{
	if (0 == GetTableRecordCount())
		RandomInitTableRecord();

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_G_GAME_RECORD_NOTIFY;
	this->WriteTableRecordToPlute(jsonSend);

	m_pTableManagerModule->SendMailToTableUser(NFMsg::baccarat_MSGID_CLIENT_G_GAME_RECORD_NOTIFY, jsonSend, fd);
}

void NFGameTable::SendUserBeanAndCardTypeAndBetBeanOfTable(const int fd)
{
	NF_SHARE_PTR<SUserInfo> pUser = m_pGameLogicModule->GetUserInfoByUserId(fd);
    NF_SHARE_PTR<NFTableUser> pTableUser = FindUserById(pUser->baseInfo.userId);

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_REOFFLINE_INFO_NOTIFY;
	jsonSend["playerBean"] = (pUser->baseInfo.bean - pTableUser->GetBetSum());

	// 玩家金币
	nlohmann::json cardTypeJsonObject;
	if (m_regions.isCardTypeEmpty())
	{
		nlohmann::json cardJsonArray;
		nlohmann::json emptyCardJson;
		emptyCardJson["cardID"] = int32_t(-1);
		emptyCardJson["cardType"] = "-1";
		cardJsonArray.push_back(emptyCardJson);
		cardTypeJsonObject = nlohmann::json::object({ {"cardTypeInfo",cardJsonArray} });
	}
	else {	
		nlohmann::json cardJsonArray;
		m_regions.WriteGameingPlayerCardToPluto(cardJsonArray);
		cardTypeJsonObject = nlohmann::json::object({ {"cardTypeInfo",cardJsonArray} });
	}
	jsonSend.insert(cardTypeJsonObject.begin(), cardTypeJsonObject.end());

	// 玩家押注信息
	nlohmann::json playerBetJsonObject;
	if (pTableUser->GetBetVector().size() == 0)
	{
		nlohmann::json cardJsonArray;
		nlohmann::json emptyCardJson;
		emptyCardJson["betId"] = lexical_cast<std::string>(-1);
		emptyCardJson["betSum"] = lexical_cast<std::string>(-1);
		cardJsonArray.push_back(emptyCardJson);
		playerBetJsonObject = nlohmann::json::object({ {"playerBetArray",cardJsonArray} });
	}
	else
	{
		nlohmann::json cardJsonArray;
		for (auto temp : pTableUser->GetBetVector())
		{
			nlohmann::json item;
			item["betId"] = lexical_cast<std::string>(temp->betId);
			item["betSum"] = lexical_cast<std::string>(temp->betNum);	
			cardJsonArray.push_back(item);
		}
		playerBetJsonObject = nlohmann::json::object({ {"cardTypeInfo",cardJsonArray} });
	}
	jsonSend.insert(playerBetJsonObject.begin(), playerBetJsonObject.end());
	
	// 机器人押注信息
	this->WriteRobotBetInfoToPluto(jsonSend);

	jsonSend["tableState"] = GetCurStateEnum();

    m_pTableManagerModule->SendMailToTableUser(NFMsg::baccarat_MSGID_CLIENT_REOFFLINE_INFO_NOTIFY, jsonSend, pUser->self);
}

/*
 * 发送其他玩家离开房间通知
 * username : 离开的玩家的名字
 */
void NFGameTable::SendBroadToUserOtherLeaveRoom(std::string username)
{
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_OTHER_CHAT_NOTIFY;
	jsonSend["userId"] = 0;
	jsonSend["userName"] = username;
	jsonSend["chatCode"] = 4;
	jsonSend["chatMsg"] = "0";
	
	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::baccarat_MSGID_CLIENT_OTHER_CHAT_NOTIFY, jsonSend, INVALID_USERID, tusWaitNextRound, m_sitUserList);
}

/*
 * 发送其他玩家进入房间通知
 *  pUser：进入房间的玩家
 */
void NFGameTable::SendBroadToUserOtherEnterRoom(const SUserInfo& pUser)
{
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_OTHER_CHAT_NOTIFY;
	jsonSend["userId"] = 0;
	jsonSend["userName"] = pUser.baseInfo.userName;
	jsonSend["chatCode"] = 4;
	jsonSend["chatMsg"] = "1";

	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::baccarat_MSGID_CLIENT_OTHER_CHAT_NOTIFY, jsonSend, pUser.baseInfo.userId, tusWaitNextRound, m_sitUserList);
}


/*
 * 指定一个桌子的状态，发送给一个指定的用户
 * toUser：要发送的指定用户
 * tableSatate：桌子的状态
 * ingoreUserState：用户的状态，处在这一状态的用户会跳过，不发包
 */
void NFGameTable::SendTableStateToOneUser(/*const*/ NFTableUser& toUser, TableState tableSatate, int ingoreUserState)
{
    if (ingoreUserState == toUser.GetUsTate())  return;
	
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_TSTATE_CHANGE_NOTIFY;
	jsonSend["tableState"] = tableSatate;
	jsonSend["decTime"] = this->m_decTimeCount;
	
	m_pTableManagerModule->SendMailToTableUser(NFMsg::baccarat_MSGID_CLIENT_TSTATE_CHANGE_NOTIFY, jsonSend, toUser.GetSUserInfo().self);
}


/*
* 指定一个桌子的状态，发送给一个指定的用户
* toUser：要发送的指定用户
* tableSatate：桌子的状态
* ingoreUserState：用户的状态，处在这一状态的用户会跳过，不发包
*/
void NFGameTable::SendTableStateToOneUser(const NF_SHARE_PTR<SUserInfo>& toUser, TableState tableSatate, int ingoreUserState)
{
    NF_SHARE_PTR<NFTableUser> toTableUser = FindUserById(toUser->baseInfo.userId);
    if (!toTableUser)
    {
        m_pLogModule->LogError("not found user by userId", __FUNCTION__, __LINE__);
        return;
    }
    SendTableStateToOneUser(*toTableUser, tableSatate, ingoreUserState);
}

/*
 * 指定一个桌子的状态， 发送给同一桌子的所有用户
 * tableState :桌子的状态
 * ingoreUserState : 用户的状态，处在这一状态的用户会跳过,不发包
 */
void NFGameTable::SendTableStateToUserAtTheSameTable(TableState tableSatate, int ingoreUserState)
{
    /*std::string logStr = "send table state !!!!! === tableState = ";
    logStr.append(lexical_cast<string>(tableSatate));
    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);*/

	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_TSTATE_CHANGE_NOTIFY;
	jsonSend["tableState"] = tableSatate;
	jsonSend["decTime"] = this->m_decTimeCount;
	m_pTableManagerModule->SendMailToUserAtTheSameTable(NFMsg::baccarat_MSGID_CLIENT_TSTATE_CHANGE_NOTIFY, jsonSend, INVALID_USERID, ingoreUserState, m_sitUserList);
}


void NFGameTable::SendForceLeaveNotify(NF_SHARE_PTR<NFTableUser>& pTUser, int code, const char* errorMsg)
{
	nlohmann::json jsonSend;
	jsonSend["action"] = NFMsg::baccarat_MSGID_CLIENT_OTHER_LEAVE_NOTIFY;
	jsonSend["userId"] = pTUser->GetSUserInfo().baseInfo.userId;
	jsonSend["code"] = code;
	jsonSend["code"] = errorMsg;

	// Send TODO
	m_pTableManagerModule->SendMailToTableUser(NFMsg::baccarat_MSGID_CLIENT_OTHER_LEAVE_NOTIFY, jsonSend, pTUser->GetSUserInfo().self);
}


//////////////////////////////// 通知 //////////////////////////////////////

/////////////////////////////// 记录 ////////////////////////////////
void NFGameTable::ClearTableRecord()
{
    m_tableRecordCount = 0;
    tableRecordQueue.empty();
}

void NFGameTable::AddTableRecordCount()
{
    m_tableRecordCount++;
}

void NFGameTable::SubTableRecordCount()
{
    m_tableRecordCount--;
}

int NFGameTable::GetTableRecordCount()
{
    return (int)tableRecordQueue.size();
}

bool NFGameTable::IsHaveResultArea(WagerAreaType typeArea)
{
    //if (resultForArea.size() == 0) assert(0);
    map<int, float>::iterator it = resultForArea.find(typeArea);
    return it != resultForArea.end();
}

void NFGameTable::CalculateResultOfBetArea(map<int, float>& result)
{
	result.clear();
	NF_SHARE_PTR<CPlayerCard> notBanker = GetBetRegions().GetCardsOfCard_ID(CardID::NOT_DEALER_ID);
	NF_SHARE_PTR<CPlayerCard> Banker = GetBetRegions().GetCardsOfCard_ID(CardID::DEALER_ID);

	if (!notBanker)
	{
		assert(0);
		return;
	}

	if (!Banker)
	{
		assert(0);
		return;
	}
	// 1. 先检查庄、闲的大小
	// 2. 检查双方的牌型
	// 闲家牌值
	int notBankerValue = notBanker->GetCardsTotalValue();

	// 庄家牌值
	int bankerValue = Banker->GetCardsTotalValue();

	/* -------------------  先检查大小 -------------------*/
	// 1 是 庄 ，2 是 闲 3 是平局

	int win_index = bankerValue > notBankerValue ? 1 : bankerValue == notBankerValue ? 3 : 2;

	if (win_index == 1) //庄赢
		result.insert(make_pair(WagerAreaType::DEALER, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(DEALER)));

	if (win_index == 2) //闲赢
		result.insert(make_pair(WagerAreaType::NOT_DEALEAR, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(NOT_DEALEAR)));

	if (win_index == 3) //和
		result.insert(make_pair(WagerAreaType::DOGFALL, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(DOGFALL)));

	if (Banker->IsPairOfCard()) //庄对
		result.insert(make_pair(WagerAreaType::DEALER_DOUBLE, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(DEALER_DOUBLE)));

	if (notBanker->IsPairOfCard()) //闲对
		result.insert(make_pair(WagerAreaType::NOT_DEALEAR_DOUBLE, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(NOT_DEALEAR_DOUBLE)));

	// 任意对子
	if (notBanker->IsPairOfCard() || Banker->IsPairOfCard())
		result.insert(make_pair(WagerAreaType::ARBITRARILY_DOUBLE, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(ARBITRARILY_DOUBLE)));

	//完美对子
	if (Banker->IsProfectPairOfCard() || notBanker->IsProfectPairOfCard())
		result.insert(make_pair(WagerAreaType::PERFECT_DOUBLE, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(PERFECT_DOUBLE)));

	// 大 / 小
	if ((notBanker->getCardCount() + Banker->getCardCount()) == 4)
		result.insert(make_pair(WagerAreaType::SMALL, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(SMALL)));
	else
		result.insert(make_pair(WagerAreaType::BIG, m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(BIG)));
}

int NFGameTable::GetCardOfGroup(size_t index)
{
	if (index >= DECK_CARDS_COUNT)
		return 0;
	return m_deckOfCards[index];
}

void NFGameTable::Riffle()
{
	srand((unsigned int)time(NULL));
	for (int i = DECK_CARDS_COUNT - 1; i >= 0; --i)
	{
		int tmepIndex = rand() % (i + 1);
		int temp = m_deckOfCards[tmepIndex];
		m_deckOfCards[tmepIndex] = m_deckOfCards[i];
		m_deckOfCards[i] = temp;
	}
}

void NFGameTable::WriteTableRecordToPlute(nlohmann::json & j)
{
	nlohmann::json jsonArray;
    for (auto temp : tableRecordQueue)
    {
		jsonArray.push_back(temp);
    }

	nlohmann::json jsonObject = nlohmann::json::object({"recordList",jsonArray});

	j.insert(jsonArray.begin(), jsonArray.end());
}

void NFGameTable::PushBackTableRecord(int betId, int count)
{
    if (betId == 1 || betId == 7 || betId == 8)
    {
        if (GetTableRecordCount() >= TABLE_RECORD_COUNT)
        {
            tableRecordQueue.pop_front();
        }
        tableRecordQueue.push_back(betId);
    }
}

void NFGameTable::RandomInitTableRecord()
{
    srand((unsigned)time(NULL));
    while (true)
    {
        int betId = 0;
        int randomIndex = NFUtil::GetRandomRange(0, 10);

        if (randomIndex < 2)
        {
            betId = 1;
        }
        else
        {
            if (NFUtil::GetRandomRange(0, 10) < 5)
            {
                betId = 8;
            }
            else { betId = 7; }
        }

        PushBackTableRecord(betId, 1);
        if (tableRecordQueue.size() >= TABLE_RECORD_COUNT)
            break;
    }
}


#endif