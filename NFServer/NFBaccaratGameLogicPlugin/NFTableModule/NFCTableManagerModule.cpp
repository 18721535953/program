#ifndef NFC_TABLE_MANAGER_MODULE_CPP
#define NFC_TABLE_MANAGER_MODULE_CPP

#include "NFCTableManagerModule.h"
#include "NFGameTable.h"
#include "StructHPP/NFTableUser.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFQsTaskStruct.hpp"

NFCTableManagerModule::NFCTableManagerModule(NFIPluginManager* p) : m_arrangeTableTime(), m_arrangeTableAry{}
{
	pPluginManager = p;
}

NFCTableManagerModule::~NFCTableManagerModule()
{
	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		delete m_arrangeTableAry[i];
	}
}


bool NFCTableManagerModule::Awake()
{

	return true;
}

bool NFCTableManagerModule::Init()
{
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();
	m_pGameLogicModule = pPluginManager->FindModule<NFIGameLogicModule>();
	m_pRobotModule = pPluginManager->FindModule<NFIRobotModule>();
	m_pConfigAreaModule = pPluginManager->FindModule<NFIConfigAreaModule>();
	m_pNetClientModule = pPluginManager->FindModule<NFINetClientModule>();
	m_pQsTaskScheduleModule = pPluginManager->FindModule<NFIQsTaskScheduleModule>();

	m_pTableList.reserve(MAX_TABLE_HANDLE_COUNT);
	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		m_pTableList.push_back(make_shared<NFGameTable>(i, this)); //分配1024张桌子
	}

	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		m_pTableList[i]->Init();
	}

	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		m_arrangeTableAry[i] = new SArrangeTableItem();
	}

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
	//m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, NFMsg::EGMI_ACK_ROLE_LIST, this, &NFCTableManagerModule::OnMsGidMatchReportCallback);
	for (int i = 0; i < MAX_TABLE_HANDLE_COUNT; i++)
	{
		m_pTableList[i]->AfterInit();
	}

	return true;
}

bool NFCTableManagerModule::Execute()
{
	for (auto tableItem : m_pTableList)
	{
		if (tableItem->GetTablePassTick() > 1000)
		{
			tableItem->AddTableMsTick(1000);
			tableItem->Update(1);
		}
		tableItem->FixedUpdate(0);
	}

	return true;
}

void NFCTableManagerModule::OnMsGidMatchReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	
	int index = 0;
	int32_t retCode = 0  ;
	string retErrorMsg = "" ;
	int32_t tableHandle = 0;

	NF_SHARE_PTR<NFGameTable> pTable = GetTableByHandle(tableHandle);
	if (!pTable)
	{
		m_pLogModule->LogError("find table failed!", __FUNCTION__, __LINE__);
		return  ;
	}

	if (retCode != 0)
	{
		pTable->OnMatchReportError(retCode, retErrorMsg.c_str());
	}

	return ;
}

void NFCTableManagerModule::OnMsGidScoreReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	NFGamespace::DB_ReportScore p;
	uint32_t jsonLen = p.CopyFrom(msg);
	int index = 0;
	uint32_t taskId = p.taskid;

	NF_SHARE_PTR<CAreaTaskReportScore> task = NFUtil::Cast<CAreaTaskReportScore, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));

	if (!task)
	{
		m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
		return  ;
	}
	// 自动释放内存
	//auto_new1_ptr<CAreaTaskReportScore> atask(task);

	NF_SHARE_PTR<NFGameTable> pTable = GetTableByHandle(task->GetTableHandle());
	if (!pTable)
	{
		m_pLogModule->LogError("find table failed!", __FUNCTION__, __LINE__);
		return ;
	}

	int32_t retCode = 0 ;
	std::string retErrorMsg = "";
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

	return ;
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
	NF_SHARE_PTR<NFGameTable> pTable = GetTableByHandle(task->GetTableHandle());
	if (!pTable)
	{
		m_pLogModule->LogError("find table failed!", __FUNCTION__, __LINE__);
		return;
	}

	int32_t retCode = p.retCode;
	string& retErrorMsg =p.retErrorMsg;
	uint32_t contorlValue = p.controlValue;

	string strcontrolLimit = p.controlLimit;

	int64_t contorlLimit = stoll(strcontrolLimit);

	pTable->SetControlValue(contorlValue);
	pTable->SetScore(contorlLimit);

	std::ostringstream logStr;
	logStr << "controlValue = " << contorlValue << ", score = " << contorlLimit;
	m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

	return ;
}


bool NFCTableManagerModule::AddSitUser(int userId, int tableHandle, int chairIndex)
{
	map<int, uint32_t>::iterator iter = m_pSitAllUserList.find(userId);
	if (m_pSitAllUserList.end() != iter)
	{
		m_pLogModule->LogError("user exists", __FUNCTION__, __LINE__);
		return false;
	}

	uint32_t value = (tableHandle & 0xFFFF) | (chairIndex << 16);
	m_pSitAllUserList.insert(make_pair(userId, value));

	return true;
}

bool NFCTableManagerModule::EnterRoomBySelectSocre(NF_SHARE_PTR<SUserInfo> pUser, int ignoreTableHandle, function<void()> successCallBack)
{
	if (!this->UpdateCanArrangeTableList())              //没有可用的桌子
	    return false;

	//查找可以加入的、已经有人的桌子
	SArrangeTableItem* pItem = this->FindEmptyTable(pUser->activeInfo.selScore, 1, MAX_TABLE_USER_COUNT - 1, ignoreTableHandle);
	if (pItem)
	{
	    if (!this->AddUserToArrange(*pItem,pUser))
	        return false;

	    std::ostringstream logStr;
	    logStr << "find a table of some people handle = " << pItem->tableHandle << " score = " << pItem->selScore;
	    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
	}
	else
	{
	    //查找空桌子
	    pItem = this->FindEmptyTable(pUser->activeInfo.selScore, 0, MAX_TABLE_USER_COUNT, ignoreTableHandle);
	    if (pItem)
	    {
	        if (!this->AddUserToArrange(*pItem, pUser))
	            return false;
	    }
	    else
	    {
	        return false;
	    }
	}

	NF_SHARE_PTR<NFGameTable> pTable = m_pTableList[pItem->tableHandle];
	int emptyCount = pTable->GetEmptyChairAry(m_emptyChairAry);
	if (pItem->arrangeLen > emptyCount)
	{
	    // 座位不够
	    m_pLogModule->LogError("pItem->arrangeLen > emptyCount dont't have enough chairs.", __FUNCTION__, __LINE__);
	    return false;
	}

	for (int j = 0; j < pItem->arrangeLen; ++j)
	{
	    NF_SHARE_PTR<SUserInfo> pUserToAdded = pItem->arrangeUserAry[j];
	    bool isRobot = (NULL == pUserToAdded);
	    if (isRobot)
	    {
	        pUserToAdded = m_pRobotModule->AllocRobotUser(pItem->selScore);
	        if (!pUserToAdded)
	            continue;
	    }
	    if (!pTable->EnterTable(pUserToAdded, m_emptyChairAry[j], isRobot))
	    {
	        // 加入失败需要释放robot
	        if (isRobot)
	            m_pRobotModule->FreeRobotUser(pUserToAdded->baseInfo.userId);
	    }
	}

	//先处理进入房间成功回调
	successCallBack();

	//发送桌子上的所有信息
	pTable->SendAllInfoToUser(pUser, nullptr);
	return true;
}

bool NFCTableManagerModule::AddUserToArrange(class SArrangeTableItem& pItem, NF_SHARE_PTR<SUserInfo> pUser)
{
	 //对于机器人 pUser为nil
	if (pItem.arrangeLen >= (MAX_TABLE_USER_COUNT - pItem.curUser))
	{
	    m_pLogModule->LogError("no chair", __FUNCTION__, __LINE__);
	    return false;
	}

	if (pItem.selScore < 0 && &pUser != NULL)
	{
	    pItem.selScore = pUser->activeInfo.selScore;
	    std::ostringstream logStr;
	    logStr << "AddUserToArrange pItem->selScore = " << pItem.selScore;
	    m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
	}
	pItem.arrangeUserAry[pItem.arrangeLen] = pUser;

	--pItem.canSitMaxUser;             //还可以坐下多少人的数量减一
	++pItem.arrangeLen;                //待加入桌子的人的数量加一

	return true;
}

void NFCTableManagerModule::AddUserToArrange(SArrangeUserList& pUserList, class SArrangeTableItem& pItem, int delUserIndex, NF_SHARE_PTR<SUserInfo> pUser)
{
	// 对于机器人 XUser为nil   delUserIndex=-1表示不删除FArrangeUserAry中的user
	if (pItem.arrangeLen >= (MAX_TABLE_USER_COUNT - pItem.curUser))
	{
		m_pLogModule->LogError("no chair", __FUNCTION__, __LINE__);
		return;
	}

	// 以第一个入座的人selScore为准
	if (pItem.selScore < 0)
		pItem.selScore = pUserList.selScore;
	pItem.arrangeUserAry[pItem.arrangeLen] = pUser;
	--pItem.canSitMaxUser;
	if (delUserIndex >= 0)
	{
		for (int i = delUserIndex; i < pUserList.len - 1; i++)
			pUserList.UserAry[i] = pUserList.UserAry[i + 1];

		--pUserList.len;
	}

	pItem.arrangeLen++;
}


SArrangeTableItem* NFCTableManagerModule::FindEmptyTable(int selScore, int minEmptyChair, int maxEmptyChair, int ignoreHandle)
{
	SArrangeTableItem* item = m_arrangeTableAry[selScore];

	if (!item) return nullptr;

	if (item->canSitMaxUser >= minEmptyChair && item->canSitMaxUser <= maxEmptyChair)
	{
		std::ostringstream logStr;
		logStr << "target socre ：" << selScore << " minEmptyChair = " << minEmptyChair << " item->canSitMaxUser = " << item->canSitMaxUser << " selScore = " << item->selScore;
		m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

		if (item->tableHandle != ignoreHandle)
		{
			if (item->selScore < 0 || selScore == item->selScore)
			{
				std::string log = "find a table selscore = ";
				log.append(lexical_cast<string>(item->selScore));
				m_pLogModule->LogInfo(log, __FUNCTION__, __LINE__);
				return item;
			}

		}
	}
	return nullptr;
}


bool NFCTableManagerModule::UpdateCanArrangeTableList()
{
	m_arrangeTableLen = 0;
	for (vector<NF_SHARE_PTR<NFGameTable>>::iterator iter = m_pTableList.begin(); iter != m_pTableList.end(); ++iter)
	{
		NFGameTable* item = iter->get();
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
				// 获取还能进的最大人数
				curTb->canSitMaxUser = MAX_TABLE_USER_COUNT - curTb->curUser;
				curTb->arrangeLen = 0;
				curTb->tableHandle = item->GetHandle();

				if (item->GetCurUserCount() <= 0)
				{
					curTb->selScore = -1;
				}
				m_arrangeTableLen++;
			}
		}
	}

	if (m_arrangeTableLen < 1)
		return false;
	else
		return true;
}


NF_SHARE_PTR<NFGameTable> NFCTableManagerModule::GetPUserTable(const SUserInfo& pUser, int& chairIndex)
{
	int tableHandle = pUser.activeInfo.tableHandle;
	chairIndex = pUser.activeInfo.chairIndex;

	if (tableHandle >= 0)
		return m_pTableList[tableHandle];
	else
		return nullptr;
	return nullptr;
}

int NFCTableManagerModule::GetSelScoreByHandle(int handle)
{
	if (handle >= 0 && handle < (int)m_pTableList.size())
		return m_arrangeTableAry[handle]->selScore;
	return -1;
}

NF_SHARE_PTR<NFGameTable> NFCTableManagerModule::GetTableByHandle(int handle)
{
	if (handle >= 0 && handle < (int)m_pTableList.size())
		return m_pTableList[handle];
	else
		return nullptr;
}

void NFCTableManagerModule::AddTable(NF_SHARE_PTR<NFGameTable> newTable)
{
	m_pTableList.push_back(newTable);
}

int NFCTableManagerModule::GetUserCount()
{
	return (int)m_pSitAllUserList.size();
}

NF_SHARE_PTR<NFGameTable> NFCTableManagerModule::GetUserTable(int usreId, int& chairIndex)
{
	int tableHandle = 0;
	if (FindSitUser(usreId, tableHandle, chairIndex))
		return m_pTableList[tableHandle];
	else
		return nullptr;
}

bool NFCTableManagerModule::FindSitUser(int userId, int& retTableHandle, int& retChairIndex)
{
	map<int, uint32_t>::iterator iter = m_pSitAllUserList.find(userId);
	if (m_pSitAllUserList.end() == iter)
		return false;

	uint32_t value = iter->second;
	retTableHandle = value & 0xFFFF;
	retChairIndex = (value >> 16) & 0xFFFF;

	return true;
}


void NFCTableManagerModule::OnUserOffline(int userId)
{
	int chairIndex = 0;
	NF_SHARE_PTR<NFGameTable> pTable = GetUserTable(userId, chairIndex);
	if (!pTable)
		return;

	pTable->LeaveTable(userId);
}

bool NFCTableManagerModule::RemoveSitUser(int userId)
{
	map<int, uint32_t>::iterator iter = m_pSitAllUserList.find(userId);
	if (m_pSitAllUserList.end() == iter)
	{
		std::string logStr = "user not exists userId = ";
		logStr.append(lexical_cast<string>(userId));
		m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
		return false;
	}

	m_pSitAllUserList.erase(iter);
	return true;
}

bool NFCTableManagerModule::userChangeTable(SUserInfo* pUser, NFGameTable* oldTable, function<void()> successCallBack)
{
	/*bool leave = oldTable->LeaveTable(pUser->baseInfo.userId);
	if (!leave)
	{
		return false;
	}

	if (!this->EnterRoomBySelectSocre(pUser, oldTable->GetHandle(), successCallBack))
	{
		return false;
	}*/
	return true;
}

// 给单个用户发邮件
void NFCTableManagerModule::SendMailToTableUser(const int nMsgId,const nlohmann::json& jsonSend, const NFGUID& fd)
{
	if (!m_pGameLogicModule->FindUserByGuid(fd))
		m_pLogModule->LogError("share_pu == nullptr", __FUNCTION__, __LINE__);

	m_pGameLogicModule->SendMsgToClient(nMsgId, jsonSend, fd);
}

// 给一个桌子上的所有用户发送邮件
void NFCTableManagerModule::SendMailToUserAtTheSameTable(const int nMsgId, const nlohmann::json& jsonSend, const int ignoreUserId, int isIgnoreState, map<int, NF_SHARE_PTR<NFITableUserModule>>& tableUserMap)
{
	if (jsonSend.size() == 0)
		m_pLogModule->LogError("json is null", __FUNCTION__, __LINE__);

	for (map<int, NF_SHARE_PTR<NFITableUserModule>>::iterator iter = tableUserMap.begin(); iter != tableUserMap.end(); ++iter)
	{
		NF_SHARE_PTR<NFITableUserModule> pTUser = iter->second;
		if (!pTUser) continue;
		if (pTUser->GetIsRobot())continue;


		if (isIgnoreState == pTUser->GetUsTate() && pTUser->GetSUserInfo().baseInfo.userId != -1)
		{
			std::string logStr = "等待一下局，不发送数据包 userId = ";
			logStr.append(lexical_cast<string>(pTUser->GetSUserInfo().baseInfo.userId));
			m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);
			continue;
		}


		if (pTUser && pTUser->GetSUserInfo().baseInfo.userId != ignoreUserId)
		{
			m_pGameLogicModule->SendMsgToClient(nMsgId, jsonSend, pTUser->GetSUserInfo().self);
		}
	}
}

// 给所有桌子上的所有用户发邮件
void NFCTableManagerModule::SendMailToAllTableUser(const int nMsgId, const nlohmann::json& jsonSend, int ignoreUserId, vector<NF_SHARE_PTR<NFGameTable>>& tableVector)
{
	if (jsonSend.size() == 0)
		m_pLogModule->LogError("json is null", __FUNCTION__, __LINE__);

	for (auto item : tableVector)
	{
		
		SendMailToUserAtTheSameTable(nMsgId, jsonSend, ignoreUserId, true, item->GetUserList());
	}
}


bool NFCTableManagerModule::Shut()
{
	return true;
}


void NFCTableManagerModule::OnNodeDbGidReadUserInfoCallback(const NFSOCK nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen)
{
	NFGamespace::DB_UserBaseInfo baseInfo;
	baseInfo.CopyFrom(msg);
	int index = 0;
	uint32_t taskId = baseInfo.taskid;
	NF_SHARE_PTR<CAreaTaskReadUserInfo> task = NFUtil::Cast<CAreaTaskReadUserInfo, CAreaTaskItemBase>(m_pQsTaskScheduleModule->PopDbTask(taskId));
	if (!task)
	{
		m_pLogModule->LogWarning("m_taskList.end() == iter", __FUNCTION__, __LINE__);
		return;
	}

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
			jsonSend["code"] = 18;
			m_pGameLogicModule->SendMsgToClient(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, jsonSend, clientFd);
			
// 			//m_pWorldGameAreaModule->AddCloseFd(clientFd);
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
				return m_pGameLogicModule->ClientLoginResponse(pUser->self, 1003, "帐号不存在");
#endif
			}

			if (EUS_NONE != pUser->activeInfo.userState)
			{
				m_pLogModule->LogInfo("EUS_NONE != pUser->ativeInfo.userState", __FUNCTION__, __LINE__);
				return  ;
			}

			auto pOldUser = m_pGameLogicModule->GetUserInfoByUserId(baseInfo.userId);

			if (pOldUser)
			{
				NFGUID fdOld = pOldUser->self;
				nlohmann::json jsonSend;
				jsonSend["action"] = NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY;
				jsonSend["code"] = 18;
				m_pGameLogicModule->SendMsgToClient(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, jsonSend, fdOld);

				//m_pNetCommonModule->RemoveUserByFd(clientFd);
			}

			//账号校验通过
  //			mb->SetAuthz(MAILBOX_CLIENT_AUTHZ);

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
				pTable = GetUserTable(userId, chairIndex);
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

				return  ;
			}
		}
	}

	if (task->GetMsgId() == NFMsg::MSGID_CLIENT_LOGIN)
	{
		// 刷新用户信息返回
		if (0 != retCode)
		{
			m_pGameLogicModule->ClientUpdateUserInfoResponse(clientFd, retCode, retErrorMsg.c_str());
			return  ;
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

			return  ;
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
			NFMsg::RoleDelayOffline xMsgs;
			*xMsgs.mutable_self() = NFINetModule::NFToPB(pUser->self);
			xMsgs.set_code(18);
			xMsgs.set_delayoff(1);

			std::string strMsg;
			if (!xMsgs.SerializeToString(&strMsg))
			{
				return;
			}

			m_pGameServerNet_ServerModule->SendMsgToGate(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, strMsg, pUser->self);

			//nlohmann::json jsonSend;
			//jsonSend["action"] = NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY;
			//jsonSend["code"] = 18;
			//m_pGameLogicModule->SendMsgToClient(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, jsonSend, pUser->self);

			std::ostringstream logStr;
			logStr << "上锁或解锁失败  recode = " << retCode << "  errorMsg = " << retErrorMsg;
			m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
		}
		else
		{
			m_pLogModule->LogError("rocLockOrUnlockReportCallBack user is null", __FUNCTION__, __LINE__);
			//LogError("3333 ProcLockOrUnlockReportCallBack user is null", "fd:%d", userid);
		}
		std::ostringstream stream;
		stream << "CWorldGameArea::ProcLockOrUnlockReportCallBack" << nSockIndex;
		m_pLogModule->LogError(stream, __FUNCTION__, __LINE__);
		return  ;
	}


	return;
}

#endif