#ifndef TABLE_STATE_WAIT_FOR_NEXT_GAME_HPP
#define TABLE_STATE_WAIT_FOR_NEXT_GAME_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateWaitForNextGame :public ITableState
{
public:
    TableStateWaitForNextGame(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_WAIT_FOR_NEXT_GAME;
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        this->toState = tsType;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();

        toState = TableState::TABLE_CONTINUE;

        //m_pLogModule->LogInfo("Enter State of Wait Next Game !");
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        m_pGameTable->MinusTimeCount(1);

        if (m_pGameTable->GetTimeCount() <= 0)
            SetStateTo(TableState::TABLE_WAIT_GAME_START);
        
        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {

    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Wait Next Game !");

        m_pGameTable->SetTimeCount(3);
        m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
    }
private:
	NF_MACHINE_STATE toState;
};

#endif