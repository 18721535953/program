#ifndef TABLE_STATE_Wager_One_HPP
#define TABLE_STATE_Wager_One_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateWagerOne :public ITableState
{
public:
    TableStateWagerOne(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_WAGER_1;
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        toState = tsType;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();

        toState = TableState::TABLE_CONTINUE;
        //m_pLogModule->LogInfo("Enter State of Wager One !");
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        m_pGameTable->MinusTimeCount(1);

        if (m_pGameTable->GetTimeCount() <= 0)
            SetStateTo(TableState::TABLE_DEAL);

        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {
        if (m_pGameTable->GetCurUserCount() <= 0)
            SetStateTo(TableState::TABLE_IDLE);

        m_pGameTable->RandomRobotBetTime();
    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Wager One !");

        m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
    }
private:
	NF_MACHINE_STATE toState;

};

#endif