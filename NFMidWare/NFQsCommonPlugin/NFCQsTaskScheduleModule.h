#ifndef NFC_QS_TASK_SCHEDULE_MODULE_H
#define NFC_QS_TASK_SCHEDULE_MODULE_H

#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFCore/NFMapEx.hpp"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFCore/NFUtil.hpp"
#include <queue>

class NFCQsTaskScheduleModule :public NFIQsTaskScheduleModule
	,NFMapEx<int, CAreaTaskItemBase>
{
public:
	NFCQsTaskScheduleModule(NFIPluginManager* p);

	virtual ~NFCQsTaskScheduleModule();

	virtual bool Awake();

	virtual bool Init();

	virtual bool AfterInit();

	virtual bool Execute();

	virtual void AddTask(NF_SHARE_PTR<CAreaTaskItemBase> & task, std::string& token, const uint32_t taskType, int32_t userId);

	virtual bool AddTask(NF_SHARE_PTR<CAreaTaskItemBase> & task);

	virtual NF_SHARE_PTR<CAreaTaskItemBase> PopDbTask(const uint32_t taskId);

	virtual void AddTaskTimeOutCallBack(NF_QS_TASK_TIMEOUT_FUN_PTR& cb) override;

	////////////////////////////////////////////////////////////////////////
private:
	void CheckTaskListTimeOut();

	inline int AllocaTaskID()
	{
		if(m_AllocaTaskQueue.size() > 0)
		{
			int taskId = m_AllocaTaskQueue.back();
			m_AllocaTaskQueue.pop();
			return taskId;
		}
		return m_AllocaTaskID++;
	}

private:
	NFILogModule * m_pLogModule;

private:
	int m_AllocaTaskID;

	int m_CheckTaskTime;

	NFTimer taskTimer;

	std::list<NF_QS_TASK_TIMEOUT_FUN_PTR> m_TimeOutTaskList;

	// 获得超时任务列表
	list<int> timeOutList;

	queue<int> m_AllocaTaskQueue;
};

#endif