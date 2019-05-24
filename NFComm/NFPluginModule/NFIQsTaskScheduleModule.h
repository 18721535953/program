#ifndef NFI_QS_TASK_SCHEDULE_MODULE
#define NFI_QS_TASK_SCHEDULE_MODULE
#include "NFIModule.h"

class CAreaTaskItemBase;

class NFIQsTaskScheduleModule :public NFIModule
{
protected:
	typedef std::function<void(const int, const int)> NF_QS_TASK_TIMEOUT_FUN;
	typedef NF_SHARE_PTR<NF_QS_TASK_TIMEOUT_FUN> NF_QS_TASK_TIMEOUT_FUN_PTR;

public:
	virtual void AddTask(NF_SHARE_PTR<CAreaTaskItemBase> & task, std::string& token, const uint32_t taskType, int32_t userId) = 0;

	virtual bool AddTask(NF_SHARE_PTR<CAreaTaskItemBase> & task) = 0;

	virtual NF_SHARE_PTR<CAreaTaskItemBase> PopDbTask(const uint32_t taskId) = 0;

	template<typename BaseType>
	inline void AddTaskTimeOutCallBack(BaseType * baseType, void(BaseType::*handleRecieve)(const int, const int))
	{
		NF_QS_TASK_TIMEOUT_FUN func = std::bind(handleRecieve, baseType, std::placeholders::_1, std::placeholders::_2);
		NF_QS_TASK_TIMEOUT_FUN_PTR funcPtr(new NF_QS_TASK_TIMEOUT_FUN(func));

		AddTaskTimeOutCallBack(funcPtr);
	}

	virtual void AddTaskTimeOutCallBack(NF_QS_TASK_TIMEOUT_FUN_PTR& cb) = 0;
};

#endif // !NFI_QS_TASK_SCHEDULE_MODULE

