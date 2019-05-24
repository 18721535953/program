#include "NFCConfigAreaModule.h"
#include "NFComm/NFCore/NFIPropertyManager.h"
#include "NFComm/NFPluginModule/NFPlatform.h"
#include "NFComm/NFPluginModule/NFIClassModule.h"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFComm/NFCore/NFDateTime.hpp"

#define CTotaRate 1000
vector<int*> patterns;

NFCConfigAreaModule::NFCConfigAreaModule(NFIPluginManager* p)
{
    min_bean = 0;
    max_offline_sec = 0; 
    realuser_max_waitsec = 0;
    bet_Time = 0; 
    banker_on_minbean = 0;
    banker_out_minbean = 0;
    user_banker_model = 0; 
    usersystem_banker_model = 0;
    serial_banker_num = 0; 
    banker_lost_maxbean = 0; 
    user_bet_maxnum = 0; 
    inited = false; 
    revenue = 0;
    pPluginManager = p;
	ready_min_sec = 0;
	ready_max_sec = 0;
	if (score_list == NULL)
	{
		score_list = new SConfigScoreList();
	}
	//
	fillchair_sec = 0;
	addrobot_sec = 0;
	addrobot_rate = 100;

	estimate_ip = 0;

	pin_bet_round = 3;
	vs_bet_round = 1;
	no_see_round = 5;
	specialGold_name = "";
	allow_other_gaming = 0;
}

NFCConfigAreaModule::~NFCConfigAreaModule()
{
    //CLEAR_POINTER_CONTAINER(FSecAry);

    ClearSConfigScoreMap();
    ClearSConfigBeanRangeItem();
    ClearSConfigBeanRange();
	if (score_list)
	{
		delete score_list;
		score_list = NULL;
	}
}


bool NFCConfigAreaModule::Awake()
{
    return true;
}

bool NFCConfigAreaModule::Init()
{
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();
    //m_pNetCommonModule = pPluginManager->FindModule<NFINetCommonModule>();
	m_pElementModule = pPluginManager->FindModule<NFIElementModule>();
	m_pKernelModule = pPluginManager->FindModule<NFIKernelModule>();
	m_pClassModule = pPluginManager->FindModule<NFIClassModule>();
    m_pLogModule = pPluginManager->FindModule<NFILogModule>();

	score_list = new SConfigScoreList;
    return true;
}


bool NFCConfigAreaModule::AfterInit()
{
	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFMsg::MSGID_AREA_REFRESH_CONFIG_CALLBACK, this, &NFCConfigAreaModule::OnMsGidRefreshConfig);
	m_LastTickTimer.Init();
	return true;
}

bool NFCConfigAreaModule::Execute()
{	
    if (m_LastTickTimer.GetTimePassMillionSecond() > 300)
    {
		m_LastTickTimer.SetNowTime();
        StartRefreshConfig();
    }
    return true;
}


void NFCConfigAreaModule::InitCfg(NFGamespace::QS_GameServer_Type gameType)
{
	NF_SHARE_PTR<NFIPropertyManager> gameServerPropertyManager = m_pClassModule->GetClassPropertyManager(NFrame::GameServerIni::ThisName());
	
	std::string configId;

	if (!NFGamespace::NFCommonConfig::GetSingletonPtr()->GetGameServerTypeID(gameType, configId))
	{
		NFASSERT(gameType, "not gameType", __FILE__, __FUNCTION__);
	}
		
	if (!gameServerPropertyManager)
	{
		NFASSERT(0, "not NFrame::GameServerIni", __FILE__, __FUNCTION__);
	}

	min_bean = m_pElementModule->GetPropertyInt32(configId,NFrame::GameServerIni::min_bean());
	robot_min_bean = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::robot_min_bean());
	robot_max_bean = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::robot_max_bean());
    max_offline_sec = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::max_offline_sec());
    realuser_max_waitsec = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::realuser_max_waitsec());
    gameRoomId = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::gameRoomId());
    m_cis_key = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::cis_key());
    m_aes_key = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::aes_key());

    area_num = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::area_num());
    if (area_num < 0)
        area_num = 0;
    if (area_num > 999)
        area_num = 999;

    masterFsId = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::master_fs_id());
    revenue = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::revenue());

    {
        near_node.clear();
        string str = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::near_node());
        vector<string> spl;
        NFUtil::SplitStringToVector(str, ',', spl);
        if (spl.size() > 0)
        {
            for (vector<string>::iterator it = spl.begin(); it != spl.end(); ++it)
                near_node.push_back(atoi(it->c_str()));
        }
        std::ostringstream logStr;
        logStr << "near_node count = " << near_node.size();
        m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
    }
    area_jingweidu = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::area_jingweidu());
    {
        //读取机器人下注配置
        m_enableCheat = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::EnableCheat());

        m_robotBetMinPercent = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::robotBetMinPercent());
        m_robotBetMaxPercent = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::robotBetMaxPercent());

        string strRobot = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::robotBetProportion());
        vector<string> splRobot;
        NFUtil::SplitStringToVector(strRobot, ',', splRobot);
        if (splRobot.size() > 0)
        {
            for (vector<string>::iterator it = splRobot.begin(); it != splRobot.end(); ++it)
                m_robotBetProportionAry.push_back(atoi(it->c_str()));
        }
        std::ostringstream logStr;
        logStr << "m_robotBetProportion count = " << m_robotBetProportionAry.size();
        m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

        strRobot = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::robotBetChip());
        splRobot.clear();
		NFUtil::SplitStringToVector(strRobot, ',', splRobot);
        if (splRobot.size() > 0)
        {
            for (vector<string>::iterator it = splRobot.begin(); it != splRobot.end(); ++it)
                m_robotBetChipAry.push_back(atoi(it->c_str()));
        }
        std::ostringstream logStr2;
        logStr2 << "m_robotBetChipAry count = " << m_robotBetChipAry.size();
        m_pLogModule->LogInfo(logStr2, __FUNCTION__, __LINE__);
    }

    int n = 5;
    int a[5] = { 0, 1, 2, 3, 4 };
    Perm(0, n, a);

	SetInited();

	InitCfgZjh(gameType);
}

void NFCConfigAreaModule::Perm(int start, int end, int a[])
{
    //得到全排列的一种情况，输出结果
    if (start == end) {
        int* b = (int*)malloc(sizeof(int)* 5);
        for (int i = 0; i < end; i++)
            b[i] = a[i];
        patterns.push_back((int*)b);
        return;
    }
    for (int i = start; i < end; i++) {
        swap(a[start], a[i]);      //交换
        Perm(start + 1, end, a);   //分解为子问题a[start+1,...,end-1]的全排列
        swap(a[i], a[start]);      //回溯
    }
}

int NFCConfigAreaModule::OnMsGidRefreshConfig(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
    //m_pLogModule->LogInfo("刷新用户配置信息", __FUNCTION__, __LINE__);

    //if (p->size() != 16)
    //{
    //    m_pLogModule->LogInfo("p->size() error", __FUNCTION__, __LINE__);
    //    return -1;
    //}

    //int index = 0;
    //uint32_t taskId = (*p)[index++]->vv.u32;
    //CAreaTaskRefreshConfig* task = dynamic_cast<CAreaTaskRefreshConfig*>(m_pNetCommonModule->PopDbTask(taskId));
    //if (!task)
    //{
    //    m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
    //    return -1;
    //}
    //// 自动释放内存
    //auto_new1_ptr<CAreaTaskRefreshConfig> atask(task);

    //int32_t retCode = (*p)[index++]->vv.i32;
    //string& retErrorMsg = VOBJECT_GET_SSTR((*p)[index++]);
    //if (0 == retCode)
    //{
    //    ReadFromVOBJ(*p, index);
    //}

    return 0;
}

void NFCConfigAreaModule::checkBindMachine()
{

}


bool NFCConfigAreaModule::IsInited()
{
    return this->inited;
}


void NFCConfigAreaModule::SetInited()
{
    this->inited = true;
}

void NFCConfigAreaModule::ReadFromVOBJ(const nlohmann::json& j, int& index)
{
	NFGamespace::DBMgrRefreshConfigCallBackStruct refreshObj = j;

	this->bet_Time = refreshObj.betTime;
	this->banker_on_minbean = refreshObj.banker_on_minbean;
    this->banker_out_minbean = refreshObj.banker_out_minbean;
    this->user_banker_model = refreshObj.user_banker_model;
    this->usersystem_banker_model = refreshObj.usersystem_banker_model;
    this->serial_banker_num = refreshObj.serial_banker_num;
    this->banker_lost_maxbean = refreshObj.banker_lost_maxbean;
    this->user_bet_maxnum = refreshObj.user_bet_maxnum;

    this->bet_nums.clear();
    string betNums = refreshObj.betNums;
    vector<string> spl;
    NFUtil::SplitStringToVector(betNums, ',', spl);
    if (spl.size() > 0)
    {
        for (vector<string>::iterator it = spl.begin(); it != spl.end(); ++it)
            this->bet_nums.push_back(atoi(it->c_str()));

        this->min_bean = this->bet_nums[0];
    }

    this->sys_revenue_amount = refreshObj.sys_revenue_amount;
    this->sys_now_stock = refreshObj.sys_now_stock;
    this->sys_revenue_scale = refreshObj.sys_revenue_scale;
    this->sys_max_upcondition = refreshObj.sys_max_upcondition;
    this->inited = true;
}

void NFCConfigAreaModule::writeBetNumToPluto(nlohmann::json& j)
{
	try
	{
		nlohmann::json jsonArray;
		for (auto num : this->bet_nums)
		{
			jsonArray.push_back(num);
		}

		nlohmann::json jsonStructObject = nlohmann::json::object({ {"betNums",jsonArray} });

		j.insert(jsonStructObject.begin(), jsonStructObject.end());
	}
	catch (const nlohmann::detail::exception& ex)
	{
		m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
	}
}


void NFCConfigAreaModule::StartRefreshConfig()
{
	// TODO
    /*CMailBox* mbDbmgr = m_pNetCommonModule->GetServerMailbox(SERVER_DBMGR);
    if (mbDbmgr)
    {
        if (!mbDbmgr->IsConnected())
        {
            m_pLogModule->LogWarning("!mbDbmgr->IsConnected()", __FUNCTION__, __LINE__);
            return;
        }
        else
        {
            CAreaTaskRefreshConfig* task = new CAreaTaskRefreshConfig();
            m_pNetCommonModule->AddDbTask(task);
            uint32_t areaId = m_pWorldGameAreaModule->GetMailboxId();

            CPluto* pu = new CPluto;
            (*pu).Encode(MSGID_DBMGR_REFRESH_CONFIG) << task->GetTaskId() << areaId << EndPluto;
            mbDbmgr->PushPluto(pu);
        }
    }
    else
    {
        m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
    }*/
}

int NFCConfigAreaModule::GetTMinMaxRandom(const TMinMax* t_min_max) const
{
    if (t_min_max)
        return t_min_max->Min + rand() % (t_min_max->Max - t_min_max->Min);
    return 0;
}


////////////////////////////////////////SConfigScoreList start//////////////////////////////////////
void NFCConfigAreaModule::AddSConfigScoreItem(SConfigScoreItem* pItem)
{
    //SConfigScoreMap[pItem->score] = pItem;
}

void NFCConfigAreaModule::ClearSConfigScoreMap()
{
//    CLEAR_POINTER_MAP(SConfigScoreMap);
}

SConfigScoreItem* NFCConfigAreaModule::FindSConfigScoreItem(const int score)
{
    map<int, SConfigScoreItem*>::iterator it = SConfigScoreMap.find(score);
    if (SConfigScoreMap.end() != it)
        return it->second;
    else
        return nullptr;
}

int NFCConfigAreaModule::GetRoundFee(int score)
{
    int ret = 0;
    SConfigScoreItem* pItem = FindSConfigScoreItem(score);
    if (pItem)
        ret = pItem->roundFee;
    else
        ret = 0;

    if (ret < 0)
    {
        m_pLogModule->LogError(" ret < 0 ", __FUNCTION__, __LINE__);
        ret = 0;
    }

    return ret;
}

bool NFCConfigAreaModule::RobotScoreValid(const int64_t robotBean, const int selScore)
{
    SConfigScoreItem* pItem = FindSConfigScoreItem(selScore);
    // 超过最大值，可以不离开座位
    if (pItem)
        return robotBean >= pItem->robotBean.Min;
    else
        return false;
}

uint16_t NFCConfigAreaModule::SConfigScoreCount() const
{
    return (uint16_t)SConfigBeanRangeVector.size();
}

void NFCConfigAreaModule::SConfigScoreWriteToPluto(CPluto* pu)
{
	// TODO
    /*if (pu)
    {
        string str("");
        char buffer[100];
        bool isFirst = true;
        for (vector<SConfigBeanRangeItem*>::iterator it = SConfigBeanRangeVector.begin(); it != SConfigBeanRangeVector.end(); ++it)
        {
            SConfigBeanRangeItem* pItem = *it;
            if (isFirst)
            {
                snprintf(buffer, sizeof(buffer), "%s", pItem->cfgStr.c_str());
                isFirst = false;
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "|%s", pItem->cfgStr.c_str());
            }
            str += buffer;
        }

        (*pu) << str;
    }*/
}
////////////////////////////////////////SConfigScoreList end//////////////////////////////////////

////////////////////////////////////////SConfigBeanRangeList Start /////////////////////////////////

void NFCConfigAreaModule::ClearSConfigBeanRange()
{
    CLEAR_POINTER_CONTAINER(SConfigBeanRangeVector);
}


void NFCConfigAreaModule::AddSConfigBeanRangeItem(SConfigBeanRangeItem* pItem)
{
    SConfigBeanRangeVector.push_back(pItem);
}


bool NFCConfigAreaModule::SConfigBeanRangeScoreValid(const int64_t userBean, int& selScore)
{
    for (vector<SConfigBeanRangeItem*>::iterator it = SConfigBeanRangeVector.begin(); it != SConfigBeanRangeVector.end(); ++it)
    {
        SConfigBeanRangeItem* pItem = *it;
        if (userBean <= pItem->bean)
        {
            if (selScore < 0)
                selScore = pItem->defaultScore;

            return pItem->ScoreExists(selScore);
        }
    }

    return false;
}


int NFCConfigAreaModule::SConfigBeanGetMaxBaseScore(const int64_t bean, const int maxScore)
{
    int retScore = -1;
    for (vector<SConfigBeanRangeItem*>::iterator it = SConfigBeanRangeVector.begin(); it != SConfigBeanRangeVector.end(); ++it)
    {
        SConfigBeanRangeItem* pItem = *it;
        if (bean <= pItem->bean)
        {
            pItem->GetMaxBaseScore(maxScore, retScore);
        }
    }

    return retScore;
}


void NFCConfigAreaModule::SConfigBeanWriteToPlute(CPluto* pu)
{
	// TODO
    /*if (pu)
    {
        string str("");
        char buffer[100];
        bool isFirst = true;
        for (vector<SConfigBeanRangeItem*>::iterator it = SConfigBeanRangeVector.begin(); it != SConfigBeanRangeVector.end(); ++it)
        {
            SConfigBeanRangeItem* pItem = *it;
            if (isFirst)
            {
                snprintf(buffer, sizeof(buffer), "%s", pItem->cfgStr.c_str());
                isFirst = false;
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "|%s", pItem->cfgStr.c_str());
            }
            str += buffer;
        }

        (*pu) << str;
    }
*/
}


uint16_t NFCConfigAreaModule::GetSConfigBeanCount() const
{
    return (uint16_t)SConfigBeanRangeVector.size();
}
////////////////////////////////////////SConfigBeanRangeList end /////////////////////////////////

////////////////////////////////////////SConfigBeanRangeItem start///////////////////////////////////////

void NFCConfigAreaModule::AddScoreSConfigBeanRangeItem(int score)
{
    ScoreItemMap[score] = score;
}

void NFCConfigAreaModule::ClearSConfigBeanRangeItem()
{
    ScoreItemMap.clear();
}

void NFCConfigAreaModule::SConfigBeanItemGetMaxBaseScore(int score, int& retScore)
{
    for (map<int, int>::iterator it = ScoreItemMap.begin(); it != ScoreItemMap.end(); ++it)
    {
        int item = it->second;
        if (item > retScore && item <= score)
            retScore = item;
    }
}

bool NFCConfigAreaModule::SConfigBeanItemScoreExists(int score)
{
    return ScoreItemMap.end() != ScoreItemMap.find(score);
}


int NFCConfigAreaModule::GetChooseOpera(bool AIsWin, int AInt, int ExtraSeeCardRate, int ExtraFollowRate, int ExtraAddBetRate, int ExtraVsCardRate, int ExtraFoldCardRate)
{
	int Result = eotFoldCard;
	if (AInt < 0) return Result;

	if (AIsWin)
	{
		if (AInt < WinCfgOperaRate[eotSeeCard] + ExtraSeeCardRate)
			Result = eotSeeCard;
		else if (AInt < WinCfgOperaRate[eotSeeCard] + WinCfgOperaRate[eotFollow] + ExtraSeeCardRate + ExtraFollowRate)
			Result = eotFollow;
		else if (AInt < WinCfgOperaRate[eotSeeCard] + WinCfgOperaRate[eotFollow] + WinCfgOperaRate[eotAddBet] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate)
			Result = eotAddBet;
		else if (AInt < WinCfgOperaRate[eotSeeCard] + WinCfgOperaRate[eotFollow] + WinCfgOperaRate[eotAddBet] + WinCfgOperaRate[eotVsCard] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate + ExtraVsCardRate)
			Result = eotVsCard;
		else if (AInt < WinCfgOperaRate[eotSeeCard] + WinCfgOperaRate[eotFollow] + WinCfgOperaRate[eotAddBet] + WinCfgOperaRate[eotVsCard] + WinCfgOperaRate[eotFoldCard] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate + ExtraVsCardRate + ExtraFoldCardRate)
			Result = eotFoldCard;
		else if (AInt < WinCfgOperaRate[eotSeeCard] + WinCfgOperaRate[eotFollow] + WinCfgOperaRate[eotAddBet] + WinCfgOperaRate[eotVsCard] + WinCfgOperaRate[eotFoldCard] + WinCfgOperaRate[eotOpenCard] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate + ExtraVsCardRate + ExtraFoldCardRate)
			Result = eotOpenCard;
	}
	else
	{
		if (AInt < LoseCfgOperaRate[eotSeeCard] + ExtraSeeCardRate)
			Result = eotSeeCard;
		else if (AInt < LoseCfgOperaRate[eotSeeCard] + LoseCfgOperaRate[eotFollow] + ExtraSeeCardRate + ExtraFollowRate)
			Result = eotFollow;
		else if (AInt < LoseCfgOperaRate[eotSeeCard] + LoseCfgOperaRate[eotFollow] + LoseCfgOperaRate[eotAddBet] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate)
			Result = eotAddBet;
		else if (AInt < LoseCfgOperaRate[eotSeeCard] + LoseCfgOperaRate[eotFollow] + LoseCfgOperaRate[eotAddBet] + LoseCfgOperaRate[eotVsCard] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate + ExtraVsCardRate)
			Result = eotVsCard;
		else if (AInt < LoseCfgOperaRate[eotSeeCard] + LoseCfgOperaRate[eotFollow] + LoseCfgOperaRate[eotAddBet] + LoseCfgOperaRate[eotVsCard] + LoseCfgOperaRate[eotFoldCard] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate + ExtraVsCardRate + ExtraFoldCardRate)
			Result = eotFoldCard;
		else if (AInt < LoseCfgOperaRate[eotSeeCard] + LoseCfgOperaRate[eotFollow] + LoseCfgOperaRate[eotAddBet] + LoseCfgOperaRate[eotVsCard] + LoseCfgOperaRate[eotFoldCard] + LoseCfgOperaRate[eotOpenCard] + \
			ExtraSeeCardRate + ExtraFollowRate + ExtraAddBetRate + ExtraVsCardRate + ExtraFoldCardRate)
			Result = eotOpenCard;
	}

	return Result;
}

////////////////////////////////////////SConfigBeanRangeItem end///////////////////////////////////////



SConfigScoreList::SConfigScoreList() : m_map()
{

}

SConfigScoreList::~SConfigScoreList()
{
	Clear();
}

void SConfigScoreList::Clear()
{
	//CLEAR_POINTER_MAP(m_map);
}

void SConfigScoreList::AddItem(SConfigScoreItem* pItem)
{
	m_map[pItem->maxTotalBet] = pItem;
}

SConfigScoreItem* SConfigScoreList::FindItem(int maxTotalBet)
{
	map<int, SConfigScoreItem*>::iterator it = m_map.find(maxTotalBet);
	if (m_map.end() != it)
		return it->second;
	else
		return NULL;
}

bool SConfigScoreList::RobotScoreValid(const int64_t robotBean, int selScore)
{
	SConfigScoreItem* pItem = FindItem(selScore);
	// 超过最大值，可以不离开座位
	if (pItem)
		return robotBean >= pItem->robotBean.Min;
	else
		return false;
}

void NFCConfigAreaModule::InitCfgZjh(NFGamespace::QS_GameServer_Type gameType)
{


	NF_SHARE_PTR<NFIPropertyManager> gameServerPropertyManager = m_pClassModule->GetClassPropertyManager(NFrame::GameServerIni::ThisName());

	std::string configId;

	if (!NFGamespace::NFCommonConfig::GetSingletonPtr()->GetGameServerTypeID(gameType, configId))
	{
		NFASSERT(gameType, "not gameType", __FILE__, __FUNCTION__);
	}

	if (!gameServerPropertyManager)
	{
		NFASSERT(0, "not NFrame::GameServerIni", __FILE__, __FUNCTION__);
	}

	//bean_name = cfg->GetValue("params", "bean_name");
	bean_name = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::bean_name());
	bet_sec = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::bet_sec());
	realuser_max_waitsec = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::realuser_max_waitsec());
	vs_bet_round = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::vs_bet_round());
	straightFlushScore = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::straightFlushScore());
	sct3OfAKindScore = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::sct3OfAKindScore());

	string strScore0 = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::score0());
	vector<string> spScore0;
	NFUtil::SplitStringToVector(strScore0, ',', spScore0);
	if (spScore0.size() > 0)
	{
		int len = (int)spScore0.size();
		if (len >= 7)
		{
			SConfigScoreItem* pItem = new SConfigScoreItem();
			pItem->maxTotalBet = atoi(spScore0.at(0).c_str());
			if (pItem->maxTotalBet < 1)
				pItem->maxTotalBet = 1;
			pItem->minBean = atoi(spScore0.at(1).c_str());
			pItem->robotBean.Min = atoi(spScore0.at(2).c_str());
			pItem->robotBean.Max = atoi(spScore0.at(3).c_str());
			if (pItem->robotBean.Max <= pItem->robotBean.Min)
				pItem->robotBean.Max = pItem->robotBean.Min + 1;
			pItem->pinBet = atoi(spScore0.at(4).c_str());
			if (pItem->pinBet < 1)
				pItem->pinBet = 1;
			pItem->baseBet = atoi(spScore0.at(5).c_str());
			if (pItem->baseBet < 1)
				pItem->baseBet = 1;
			for (int i = 6; i < len; ++i)
			{
				int tmp = atoi(spScore0.at(i).c_str());
				if (tmp < 1)
					tmp = 1;
				pItem->raiseBet.push_back(tmp);
			}
			pItem->index = 1;
			score_list->AddItem(pItem);
		}
		//	m_robotBetProportionAry.push_back(atoi(it->c_str()));
	}

	string strScore1 = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::score1());
	vector<string> spScore1;
	NFUtil::SplitStringToVector(strScore1, ',', spScore1);
	if (spScore1.size() > 0)
	{
		int len = (int)spScore1.size();
		if (len >= 7)
		{
			SConfigScoreItem* pItem = new SConfigScoreItem();
			pItem->maxTotalBet = atoi(spScore1.at(0).c_str());
			if (pItem->maxTotalBet < 1)
				pItem->maxTotalBet = 1;
			pItem->minBean = atoi(spScore1.at(1).c_str());
			pItem->robotBean.Min = atoi(spScore1.at(2).c_str());
			pItem->robotBean.Max = atoi(spScore1.at(3).c_str());
			if (pItem->robotBean.Max <= pItem->robotBean.Min)
				pItem->robotBean.Max = pItem->robotBean.Min + 1;
			pItem->pinBet = atoi(spScore1.at(4).c_str());
			if (pItem->pinBet < 1)
				pItem->pinBet = 1;
			pItem->baseBet = atoi(spScore1.at(5).c_str());
			if (pItem->baseBet < 1)
				pItem->baseBet = 1;
			for (int i = 6; i < len; ++i)
			{
				int tmp = atoi(spScore1.at(i).c_str());
				if (tmp < 1)
					tmp = 1;
				pItem->raiseBet.push_back(tmp);
			}
			pItem->index = 2;
			score_list->AddItem(pItem);
		}
		//	m_robotBetProportionAry.push_back(atoi(it->c_str()));
	}

	string strScore2 = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::score2());
	vector<string> spScore2;
	NFUtil::SplitStringToVector(strScore2, ',', spScore2);
	if (spScore2.size() > 0)
	{
		int len = (int)spScore2.size();
		if (len >= 7)
		{
			SConfigScoreItem* pItem = new SConfigScoreItem();
			pItem->maxTotalBet = atoi(spScore2.at(0).c_str());
			if (pItem->maxTotalBet < 1)
				pItem->maxTotalBet = 1;
			pItem->minBean = atoi(spScore2.at(1).c_str());
			pItem->robotBean.Min = atoi(spScore2.at(2).c_str());
			pItem->robotBean.Max = atoi(spScore2.at(3).c_str());
			if (pItem->robotBean.Max <= pItem->robotBean.Min)
				pItem->robotBean.Max = pItem->robotBean.Min + 1;
			pItem->pinBet = atoi(spScore2.at(4).c_str());
			if (pItem->pinBet < 1)
				pItem->pinBet = 1;
			pItem->baseBet = atoi(spScore2.at(5).c_str());
			if (pItem->baseBet < 1)
				pItem->baseBet = 1;
			for (int i = 6; i < len; ++i)
			{
				int tmp = atoi(spScore2.at(i).c_str());
				if (tmp < 1)
					tmp = 1;
				pItem->raiseBet.push_back(tmp);
			}
			pItem->index = 3;
			score_list->AddItem(pItem);
		}
	}

	string strScore3 = m_pElementModule->GetPropertyString(configId, NFrame::GameServerIni::score3());
	vector<string> spScore3;
	NFUtil::SplitStringToVector(strScore3, ',', spScore3);
	if (strScore3.size() > 0)
	{
		int len = (int)spScore3.size();
		if (len >= 7)
		{
			SConfigScoreItem* pItem = new SConfigScoreItem();
			pItem->maxTotalBet = atoi(spScore3.at(0).c_str());
			if (pItem->maxTotalBet < 1)
				pItem->maxTotalBet = 1;
			pItem->minBean = atoi(spScore3.at(1).c_str());
			pItem->robotBean.Min = atoi(spScore3.at(2).c_str());
			pItem->robotBean.Max = atoi(spScore3.at(3).c_str());
			if (pItem->robotBean.Max <= pItem->robotBean.Min)
				pItem->robotBean.Max = pItem->robotBean.Min + 1;
			pItem->pinBet = atoi(spScore3.at(4).c_str());
			if (pItem->pinBet < 1)
				pItem->pinBet = 1;
			pItem->baseBet = atoi(spScore3.at(5).c_str());
			if (pItem->baseBet < 1)
				pItem->baseBet = 1;
			for (int i = 6; i < len; ++i)
			{
				int tmp = atoi(spScore3.at(i).c_str());
				if (tmp < 1)
					tmp = 1;
				pItem->raiseBet.push_back(tmp);
			}
			pItem->index = 4;
			score_list->AddItem(pItem);
		}
		//	m_robotBetProportionAry.push_back(atoi(it->c_str()));
	}

	std::ostringstream logStr;
	logStr << "m_robotBetProportion count = " << m_robotBetProportionAry.size();
	m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

}