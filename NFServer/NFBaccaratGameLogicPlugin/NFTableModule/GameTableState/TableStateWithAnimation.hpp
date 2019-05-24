#ifndef TABLE_STATE_WITH_ANIMATION_HPP
#define TABLE_STATE_WITH_ANIMATION_HPP

#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateWithAnimation :public ITableState
{
public:
    TableStateWithAnimation(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_WITH_ANIMATION;
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        toState = tsType;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();

        //m_pLogModule->LogInfo("Enter State of With Animation !");
        m_pGameTable->SetTimeCount(6);
        toState = TableState::TABLE_CONTINUE;
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        m_pGameTable->MinusTimeCount(1);

        if (m_pGameTable->GetTimeCount() <= 0)
            SetStateTo(TableState::TABLE_WAGER_2);

        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {

    }

    void Exit() override
    {
        m_pGameTable->SetTimeCount(8);
        m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
        //m_pLogModule->LogInfo("Exit State of With Animation !");
    }
private:
	NF_MACHINE_STATE toState;
};

#endif