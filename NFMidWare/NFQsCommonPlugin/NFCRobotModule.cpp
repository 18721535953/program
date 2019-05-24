#include "NFCRobotModule.h"
//#include "StructHPP/GameConfig.hpp"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFComm/NFMessageDefine/NFProtocolDefine.hpp"
#include "NFComm/NFPluginModule/NFPlatform.h"
#include "NFComm/NFCore/NFDateTime.hpp"
#include "../../NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "NFQsTaskStruct.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "../../NFServer/NFBaccaratGameLogicPlugin/NFTableModule/StructHPP/NFTableUser.hpp"

NFCRobotModule::NFCRobotModule(NFIPluginManager* p) :m_isInit(false), m_isReading(false), m_readSuccessCount(0), m_allUserId(), m_allRobot(), m_idleRobot(), m_gameRobot()
{
    pPluginManager = p;
}

NFCRobotModule::~NFCRobotModule()
{
    m_allUserId.clear();

//    CLEAR_POINTER_MAP(m_allRobot);
    // 3者共用内存
    m_idleRobot.clear();
    m_gameRobot.clear();
}

bool NFCRobotModule::Awake()
{
    return true;
}

bool NFCRobotModule::Init()
{
    m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pClassModule = pPluginManager->FindModule<NFIClassModule>();
	m_pElementModule = pPluginManager->FindModule<NFIElementModule>();
    //m_pNetCommonModule = pPluginManager->FindModule<NFINetCommonModule>();
    m_pConfigAreaModule = pPluginManager->FindModule<NFIConfigAreaModule>();
    m_pGameLogicModule = pPluginManager->FindModule<NFIGameLogicModule>();
	m_pNFIGameServerToDBModule = pPluginManager->FindModule<NFIGameServerToDBModule>();
	m_pQsTaskScheduleModule = pPluginManager->FindModule<NFIQsTaskScheduleModule>();
    return true;
}

bool NFCRobotModule::AfterInit()
{
	m_checkReadRobotUserInfoTime.Init();
    return true;
}

bool NFCRobotModule::Execute()
{
    if (m_checkReadRobotUserInfoTime.GetTimePassMillionSecond() > 1000)
    {
		m_checkReadRobotUserInfoTime.SetNowTime();
        CheckStartReadUserInfo();
    }
    return true;
}

void NFCRobotModule::InitCfg(NFGamespace::QS_GameServer_Type gameType)
{
	NF_SHARE_PTR<NFIClass> xClass = m_pClassModule->GetElement(NFrame::GameServerIni::ThisName());
	
	std::string configId;

	if (!NFGamespace::NFCommonConfig::GetSingletonPtr()->GetGameServerTypeID(gameType, configId))
	{
		NFASSERT(gameType, "not gameType", __FILE__, __FUNCTION__);
	}

	if (!xClass)
	{
		NFASSERT(0, "not NFrame::GameServerIni", __FILE__, __FUNCTION__);
	}

	
    char buffer[100];
    int startId = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::startId());
    int endId = m_pElementModule->GetPropertyInt32(configId, NFrame::GameServerIni::endId());

    std::ostringstream strRobotLog;
    strRobotLog << "机器人ID startId = " << startId << " endId = " << endId;
    m_pLogModule->LogInfo(strRobotLog, __FUNCTION__, __LINE__);

    for (int i = startId; i < endId; i++)  // TODO
    {
        m_allRobot.insert(make_pair(i, new SUserInfo()));
        m_allUserId.push_back(i);
    }

    //if (m_allRobot.size() < MAX_TABLE_HANDLE_COUNT * (MAX_TABLE_USER_COUNT - 1))
    //{
    //    m_pLogModule->LogError("robot too little!", __FUNCTION__, __LINE__);
    //}

    std::ostringstream strLog;
    strLog << "m_allRobot.size() = " << m_allRobot.size();
    m_pLogModule->LogInfo(strLog, __FUNCTION__, __LINE__);
}

void NFCRobotModule::CheckStartReadUserInfo()
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

void NFCRobotModule::ReadUserInfoCallback(int retCode, const char* retMsg, int userIndex, NFGamespace::DB_UserBaseInfo& baseInfo)
{
	int recCount = 0;
	m_pLogModule->LogInfo(std::to_string(++recCount), __FUNCTION__, __LINE__);
	if (!m_isReading)
	{
		m_pLogModule->LogWarning("!m_isReading", __FUNCTION__, __LINE__);
		return;
	}

	if (0 != retCode)
	{
		std::ostringstream strLog;
		strLog << "retCode = " << retCode << ";retMsg = " << retMsg;
		m_pLogModule->LogWarning(strLog, __FUNCTION__, __LINE__);
		Read1RobotUserInfo(userIndex, true);
		return;
	}

	map<int, NF_SHARE_PTR<SUserInfo>>::iterator iterFind = m_allRobot.find(baseInfo.userId);
	if (m_allRobot.end() == iterFind)
	{
		std::ostringstream strLog;
		strLog << "find User failed: userId = " << baseInfo.userId;
		m_pLogModule->LogWarning(strLog, __FUNCTION__, __LINE__);
		return;
	}
	if (iterFind->first == iterFind->second->baseInfo.userId)
	{
		std::ostringstream strLog;
		strLog << "user allready read: userId = " << baseInfo.userId;
		m_pLogModule->LogWarning(strLog, __FUNCTION__, __LINE__);
		return;
	}
// 	if (m_allUserId.at(userIndex) != iterFind->first)
// 	{
// 		m_pLogModule->LogWarning("m_allUserId[userIndex] != userId", __FUNCTION__, __LINE__);
// 		return;
// 	}

	NF_SHARE_PTR<SUserInfo> pUserInfo = iterFind->second;
	pUserInfo->baseInfo.CopyFrom(baseInfo);
	m_readSuccessCount++;

	if (m_readSuccessCount >= (int)m_allRobot.size())
	{
		// init idleRobot
		for (map<int, NF_SHARE_PTR<SUserInfo>>::iterator iter = m_allRobot.begin(); m_allRobot.end() != iter; ++iter)
		{
			m_idleRobot.push_back(iter->second);
		}
		m_isReading = false;
		m_isInit = true;
	}
	else
	{
		Read1RobotUserInfo(userIndex, false);
	}
}

bool NFCRobotModule::IsRobot(int userId)
{
    return m_allRobot.find(userId) != m_allRobot.end();
}

NF_SHARE_PTR<SUserInfo> NFCRobotModule::AllocRobotUser(int selScore)
{
    int len = (int)m_idleRobot.size();
    //if (len < 1)
    //{
    //    m_pLogModule->LogError("robot user all using", __FUNCTION__, __LINE__);
    //    return nullptr;
    //}

    int index = rand() % len;
    //// 为了用erase
	vector<NF_SHARE_PTR<SUserInfo>>::iterator iter = m_idleRobot.begin()  + index;
	NF_SHARE_PTR<SUserInfo> ret = *iter;
    m_gameRobot.insert(make_pair(ret->baseInfo.userId, ret));
    m_idleRobot.erase(iter);

    //// 随机bean
    ret->baseInfo.bean = NFUtil::GetRandomRange(m_pConfigAreaModule->robot_min_bean, m_pConfigAreaModule->robot_max_bean);
    ret->activeInfo.robotReadyMs = NFUtil::GetRandomRange(m_pConfigAreaModule->ready_min_sec, m_pConfigAreaModule->ready_max_sec);
    ret->activeInfo.selScore = selScore;

    return ret;
	return 0;
}

bool NFCRobotModule::FreeRobotUser(int userId)
{
    map<int, NF_SHARE_PTR<SUserInfo>>::iterator iter = m_gameRobot.find(userId);
    if (m_gameRobot.end() == iter)
    {
        std::ostringstream logStr;
        logStr << "find user failed userId = " << userId;
        m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
        return false;
    }

    m_idleRobot.push_back(iter->second);
    m_gameRobot.erase(iter);

    std::ostringstream logStr;
    logStr << "释放机器人 RobotId = " << userId;
    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
    return true;
}

void NFCRobotModule::Read1RobotUserInfo(int lastIndex, bool reRead)
{
    if (m_isInit)
        return;
    
    //CMailBox* mbDbmgr = m_pNetCommonModule->GetServerMailbox(NFMsg::SERVER_DBMGR);
    //if (mbDbmgr)
    {
        // 找到一个未读用户信息的机器人
        int curIndex = lastIndex;
        if (!reRead)
            curIndex = lastIndex + READ_ROBOT_USER_COUNT;
    
        if (curIndex >= (int)m_allUserId.size())
        {
            std::ostringstream logStr;
            logStr << "end read: user index = " << lastIndex << " m_allUserSize = " << m_allUserId.size();
            m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
            // LogInfo("CRobotMgr::Read1RobotUserInfo", "end read: UserIndex=%d", lastIndex);
            return;
        }
        int userId = m_allUserId[curIndex];
        //创建任务
		NF_SHARE_PTR<CAreaTaskItemBase> task = make_shared<CAreaTaskReadUserInfo>(NFMsg::MSGID_ROBOT_READ_USERINFO, curIndex);
    
        std::string token = "";
        m_pQsTaskScheduleModule->AddTask(task);
        m_pGameLogicModule->ProcUserInfoToDb(task->GetTaskId(), userId, token, m_pConfigAreaModule->gameRoomId, int32_t(0), curIndex);
        //m_pNetCommonModule->AddDbTask(new CAreaTaskReadUserInfo(MSGID_ROBOT_READ_USERINFO, curIndex), token, MSGID_DBMGR_READ_USERINFO, userId);
    
        std::ostringstream logStr;
        logStr << "read user info user id = " << userId;
        m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
    }
    //else
    //{
    //    m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
    //}
}

void NFCRobotModule::StartReadRobotUserInfo()
{
    //CMailBox* mbDbmgr = m_pNetCommonModule->GetServerMailbox(NFMsg::SERVER_DBMGR);
    
// 
// 	NF_SHARE_PTR<ConnectData> dbData = m_pNFIGameServerToDBModule->GetServerNetInfo(NF_ST_DB);
// 	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
// 	{
// 		// 登录失败
// 		//  ClientLoginResponse(srcFd, 101, "服务器维护中");
// 		return -1;
// 	}
	
	if (m_pNFIGameServerToDBModule)
    {
        //if (!mbDbmgr->IsConnected())
        //{
        //    m_pLogModule->LogWarning("!mbDbmgr->IsConnected()", __FUNCTION__, __LINE__);
            // LogWarning("CRobotMgr::StartReadRobotUserInfo", "!mbDbmgr->IsConnected()");
        //    return;
        //}
        //else
        {
            for (int i = 0; i < 5; ++i)
            {
                if (i >= (int)m_allUserId.size())
                    break;
    
                int userId = m_allUserId[i];
                //创建任务
                std::string token = "";
                //CAreaTaskItemBase * task = new CAreaTaskReadUserInfo(MSGID_ROBOT_READ_USERINFO, i);
                //m_pNetCommonModule->AddDbTask(task);

				NF_SHARE_PTR<CAreaTaskItemBase> readUserItask = make_shared<CAreaTaskReadUserInfo >(NFMsg::MSGID_ROBOT_READ_USERINFO, i);
				m_pQsTaskScheduleModule->AddTask(readUserItask);


// 				if (m_pNFIGameServerToDBModule)
// 				{
// 					//short dataLen = tmpStr.size();
// 					m_pNFIGameServerToDBModule->SendReadUserInfo(task->GetTaskId(), gameRoomId, dataLen, tmpStr.c_str());
// 
// 				}
// 				else
// 				{
// 					m_pLogModule->LogError("!mbDbmgr", __FUNCTION__, __LINE__);
// 				}

                m_pGameLogicModule->ProcUserInfoToDb(readUserItask->GetTaskId(), userId, token, m_pConfigAreaModule->gameRoomId, int32_t(0), i);
    
                std::ostringstream strLog;
                strLog << "read robot user info ,User id = " << userId << ",taskID == " << readUserItask->GetTaskId();
                m_pLogModule->LogInfo(strLog, __FUNCTION__, __LINE__);
            }
            m_isReading = true;
        }
    }
    else
    {
        m_pLogModule->LogInfo("!mbDbMgr", __FUNCTION__, __LINE__);
    }
}

bool NFCRobotModule::GetIsInit() const
{
    return m_isInit;
}
