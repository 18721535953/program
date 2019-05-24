#ifndef NF_GAME_TABLE_STATE_MANAGER_H
#define NF_GAME_TABLE_STATE_MANAGER_H

#include "NFComm/NFCore/NFMapEx.hpp"

typedef int32_t NF_MACHINE_STATE;

class NFGameTable;
class NFILogModule;
class NFIPluginManager;

class ITableState
{
public:

	virtual NF_MACHINE_STATE GetStateType() = 0;

	virtual void SetStateTo(NF_MACHINE_STATE tsType) = 0;

	virtual void Enter() = 0;

	virtual NF_MACHINE_STATE Update(float fDeltaTime) = 0;

	virtual void FixedUpdate(float fixedDeltaTime) = 0;

	virtual void Exit() = 0;

public:
	NFGameTable * m_pGameTable;
	NFIPluginManager * pPluginManager;
	NFILogModule * m_pLogModule;
};


class NFMachineState
    :NFMapEx<NF_MACHINE_STATE, ITableState>
{
public:

	NFMachineState()
    {
        currentState = nullptr;
		m_MachineDefaultState = -1;
		m_MachineContinueState = -1;
		m_MachineError = -1;
    }

    inline bool AddElement(const NF_MACHINE_STATE& name, const std::shared_ptr<ITableState> data) override
    {
        return NFMapEx::AddElement(name, data);
    }

	inline bool CheckMachineConfig()
	{
		if (m_MachineContinueState == -1 || m_MachineDefaultState == -1)
			return false;
		return true;
	}

    inline ITableState* GetCurState()
    {
        return currentState;
    }

	template<typename T>
	inline T GetCurStateEnum()
    {
		if (currentState)
			return static_cast<T>(currentState->GetStateType());
		return static_cast<T>(m_MachineError);
    }

	inline NF_MACHINE_STATE GetCurStateEnum()
	{
		if (currentState)
			return currentState->GetStateType();
		return m_MachineError;
	}


    inline void ChangeTabelStateTo(NF_MACHINE_STATE newStateType)
    {
        if (currentState != nullptr &&currentState->GetStateType() == newStateType)
            return;

        if (GetElementNude(newStateType))
        {
            if (currentState != nullptr)
                currentState->Exit();
        }

        currentState = GetElementNude(newStateType);
        currentState->Enter();
    }

    inline void EnterDefaultState()
    {
        ChangeTabelStateTo(m_MachineDefaultState);
    }

    inline void FixedUpdate(float fixedDeltaTime)
    {
        if (currentState != nullptr)
        {
            currentState->FixedUpdate(fixedDeltaTime);
        }
    }

    inline void Update(float fDeltaTime)
    {
		NF_MACHINE_STATE nextStateType = m_MachineContinueState;
        if (currentState != nullptr)
        {
            nextStateType = currentState->Update(fDeltaTime);
        }

        if (nextStateType < m_MachineContinueState)
        {
            ChangeTabelStateTo(nextStateType);
        }
    }

    inline ITableState* GetState(NF_MACHINE_STATE type)
    {
        return GetElementNude(type);
    }

private:
    ITableState * currentState;

protected:
	NF_MACHINE_STATE m_MachineDefaultState;			// 默认状态
	NF_MACHINE_STATE m_MachineContinueState;		// 一般设置为状态机最大值
	NF_MACHINE_STATE m_MachineError;

};

#endif