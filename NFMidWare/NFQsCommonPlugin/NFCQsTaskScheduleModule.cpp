#include "NFCQsTaskScheduleModule.h"
#include "NFQsTaskStruct.hpp"
#include "NFComm/NFCore/NFDateTime.hpp"

NFCQsTaskScheduleModule::NFCQsTaskScheduleModule(NFIPluginManager* p)
{
	pPluginManager = p;
	m_AllocaTaskID = 0;
}

NFCQsTaskScheduleModule::~NFCQsTaskScheduleModule()
{
	
}

bool NFCQsTaskScheduleModule::Awake()
{
	return true;
}

bool NFCQsTaskScheduleModule::Init()
{
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	return true;
}

bool NFCQsTaskScheduleModule::AfterInit()
{
	return true;
}

bool NFCQsTaskScheduleModule::Execute()
{

	if (taskTimer.GetTimePassMillionSecond() > 1000)
	{
		// 删除超时任务
		for (list<int>::iterator iter = timeOutList.begin(); iter != timeOutList.end(); ++iter)
		{
			int taskId = *iter;
			if (ExistElement(taskId))
			{
				RemoveElement(taskId);
			}
		}
		timeOutList.clear();
		CheckTaskListTimeOut();
		taskTimer.SetNowTime();
	}
	return true;
}

void NFCQsTaskScheduleModule::AddTask(NF_SHARE_PTR<CAreaTaskItemBase> & task, std::string& token, const uint32_t taskType, int32_t userId)
{
	if (!AddTask(task)) return;

	// TODO
}

bool NFCQsTaskScheduleModule::AddTask(NF_SHARE_PTR<CAreaTaskItemBase> & task)
{
	int taskId = m_AllocaTaskID++;
	if (ExistElement(taskId))
	{
		std::ostringstream logStr;
		logStr << "重复的taskId:" << taskId << " ; msgId:" << task->GetMsgId();
		m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
		return false;
	}
	task->SetTaskId(taskId);

	std::ostringstream logStr;
	logStr << "添加任务 : " << taskId << "任务类型：" << task->GetMsgId() << task->GetPassTime();
	m_pLogModule->LogInfo(logStr, __FUNCTION__, __LINE__);

	return AddElement(taskId, task);
}

NF_SHARE_PTR<CAreaTaskItemBase> NFCQsTaskScheduleModule::PopDbTask(const uint32_t taskId)
{
	if (!ExistElement(taskId))
	{
		std::ostringstream logStr;
		logStr << "不存在的taskId:" << taskId;
		m_pLogModule->LogError(logStr, __FUNCTION__, __LINE__);
		return nullptr;
	}
	timeOutList.push_back(taskId);
	NF_SHARE_PTR<CAreaTaskItemBase> temp = GetElement(taskId);

	return temp;
}

void NFCQsTaskScheduleModule::AddTaskTimeOutCallBack(NF_QS_TASK_TIMEOUT_FUN_PTR& cb)
{
	m_TimeOutTaskList.push_back(cb);
}

void NFCQsTaskScheduleModule::CheckTaskListTimeOut()
{

	CAreaTaskItemBase* forItem = FirstNude();
	for (; forItem != nullptr; forItem = NextNude())
	{
		//std::ostringstream logStr;
		//logStr << "任务用时 " << forItem->GetPassTime() << "   任务ID:" << forItem->GetTaskId();
		//m_pLogModule->LogWarning(logStr, __FUNCTION__, __LINE__);

		if (forItem->IsTimeout())
		{
			timeOutList.push_back(forItem->GetTaskId());

			std::ostringstream logStr;
			logStr << "任务用时 " << forItem->GetPassTime() << "   超时:" << forItem->GetTimeOutMs();
			m_pLogModule->LogWarning(logStr, __FUNCTION__, __LINE__);

			if (m_TimeOutTaskList.size() > 0)
			{
				for (auto timeOutItem : m_TimeOutTaskList)
				{
					timeOutItem->operator()(forItem->GetTaskId(), forItem->GetMsgId());
				}
			}
		}
	}
}
