#ifndef NF_QS_TASK_STRUCT_HPP
#define NF_QS_TASK_STRUCT_HPP

#include "NFComm/NFCore/NFUtil.hpp"
#include "NFComm/NFMessageDefine/NFDefine.pb.h"

class CAreaTaskItemBase
{
public:
	CAreaTaskItemBase(uint32_t msgId, uint32_t timeoutMs) :m_msgid(msgId), m_addTime(), m_timeoutMs(timeoutMs) 
	{
		m_addTime.SetNowTime();
	}
	virtual ~CAreaTaskItemBase() {}
protected:
	uint32_t m_taskId;
	uint32_t m_msgid;
	uint32_t m_timeoutMs;
	NFTimer m_addTime;
public:
	inline uint32_t GetTaskId() const
	{
		return m_taskId;
	}

	inline void SetTaskId(const int taskId) 
	{
		this->m_taskId = taskId;
	}

	inline uint32_t GetMsgId() const
	{
		return m_msgid;
	}

	inline bool IsTimeout()
	{
		return (m_addTime.GetTimePassMillionSecond() >= m_timeoutMs);
	}
	inline int GetTimeOutMs()
	{
		return m_timeoutMs;
	}

	inline void SetMsgId(uint32_t msgId)
	{
		m_msgid = msgId;
	}

	inline int GetPassTime()
	{
		return m_addTime.GetTimePassMillionSecond();
	}
};

class CAreaTaskReadUserInfo : public CAreaTaskItemBase
{
public:
	CAreaTaskReadUserInfo(uint32_t msgId, int clientId) :CAreaTaskItemBase(msgId, 10 * 1500), userid(clientId)
	{
		m_addTime.Init();
	}
	CAreaTaskReadUserInfo(uint32_t msgId, NFGUID clientId) :CAreaTaskItemBase(msgId, 10 * 1500), m_self(clientId)
	{
		m_addTime.Init();
	}
	~CAreaTaskReadUserInfo() {}
private:
	NFGUID m_self;  // client socket fd
	int userid; //robot curIndex
	int m_UserId;                         // client userid
public:
	inline NFGUID GetGUId() const
	{
		return m_self;
	}
	inline int GetClientUserId() const
	{
		return userid;
	}
};

class CAreaTaskReportScore : public CAreaTaskItemBase
{
public:
	CAreaTaskReportScore(int tableHandle) :CAreaTaskItemBase(NFMsg::MSGID_DBMGR_REPORT_SCORE, 10 * 1000), m_tableHandle(tableHandle) 
	{
		m_addTime.Init();
	}
	CAreaTaskReportScore(int tableHandle, uint32_t msgId) :CAreaTaskItemBase(msgId, 10 * 1000), m_tableHandle(tableHandle) {}
	~CAreaTaskReportScore() { m_tableHandle = -1; }
private:
	int m_tableHandle = -1;
public:
	inline int GetTableHandle() const
	{
		return m_tableHandle;
	}
};

class CAreaTaskRefreshConfig : public CAreaTaskItemBase
{
public:
	CAreaTaskRefreshConfig() :CAreaTaskItemBase(NFMsg::MSGID_DBMGR_REFRESH_CONFIG, 10 * 1000) 
	{
		m_addTime.Init();
	}
	~CAreaTaskRefreshConfig() {}
};

class CAreaTaskGameControl : public CAreaTaskItemBase
{
public:
	CAreaTaskGameControl(int tableHandle) : CAreaTaskItemBase(NFMsg::MSGID_DBMGR_GAME_CONTROL, 15 * 1000), m_tableHandle(tableHandle)
	{
		m_addTime.Init();
	}
	~CAreaTaskGameControl() {}
private:
	int m_tableHandle;
public:
	inline int GetTableHandle() const
	{
		return m_tableHandle;
	}
};

class CAreaTaskMatchLog : public CAreaTaskItemBase
{
public:
	CAreaTaskMatchLog() :CAreaTaskItemBase(NFMsg::MSGID_DBMGR_MATCH_LOG_TODB, 15 * 1000) 
	{
		m_addTime.Init();
	}
	~CAreaTaskMatchLog() {}
};

class CAreaTaskUserLog : public CAreaTaskItemBase
{
public:
	CAreaTaskUserLog() :CAreaTaskItemBase(NFMsg::MSGID_DBMGR_USER_LOG, 15 * 1000) 
	{
		m_addTime.Init();
	}
	~CAreaTaskUserLog() {}
};

class CAreaTaskLockOrUnlockUser :public CAreaTaskItemBase
{
public:
	CAreaTaskLockOrUnlockUser(uint32_t locktype, int32_t userid) :CAreaTaskItemBase(NFMsg::MSGID_DBMGR_LOCK_GAMEROOM_TODB, 15 * 1000)
	{
		lockType = locktype;
		userId = userid;
		m_addTime.Init();
	}
	~CAreaTaskLockOrUnlockUser()
	{

	}
public:
	inline	int GetClientUserId()const { return userId; }
private:
	int lockType;
	int userId;
};

//class CTaskRoleOffline:public CAreaTaskItemBase
//{
//public:
//	CTaskRoleOffline(NFGUID code):CAreaTaskItemBase(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY,5*100),m_Guid(code)
//	{
//		m_addTime.Init();
//	}
//
//	CTaskRoleOffline(NFGUID code,int time_out) :CAreaTaskItemBase(NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, 5 * 100), m_Guid(code)
//	{
//		m_addTime.Init();
//		m_timeoutMs = time_out;
//	}
//
//	NFGUID GetCode() const { return m_Guid; }
//private:
//	NFGUID m_Guid;
//};
#endif