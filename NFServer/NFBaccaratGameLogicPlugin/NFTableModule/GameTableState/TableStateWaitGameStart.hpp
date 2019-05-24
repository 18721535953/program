#ifndef TABLE_STATE_WAIT_GAME_START_HPP
#define TABLE_STATE_WAIT_GAME_START_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateWaitGameStart :public ITableState
{
public:

    TableStateWaitGameStart(NFGameTable* pTable, NFIPluginManager * p)
    {
        m_pGameTable = pTable;
        pPluginManager = p;
        
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_WAIT_GAME_START;
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        toState = tsType;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();

        //m_pLogModule->LogInfo("Enter State of Wait Game Start !");
        m_pGameTable->SetTimeCount(3);
        m_pGameTable->GetGameControlValue();
        toState = TableState::TABLE_CONTINUE;
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        m_pGameTable->MinusTimeCount(1);

        if (m_pGameTable->GetTimeCount() <= 0)
            SetStateTo(TableState::TABLE_WAGER_1);

        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {
        if (m_pGameTable->GetCurUserCount() <= 0)
            SetStateTo(TableState::TABLE_IDLE);
    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Wait Game Start !");

        if (m_pGameTable->BeforeStartGame())
        {
            m_pGameTable->SetTimeCount(8);
            m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
            m_pGameTable->InitMatchLog();
            m_pGameTable->RandomRobotBet();
        }
    }

private:
	NF_MACHINE_STATE toState;
};

#endif