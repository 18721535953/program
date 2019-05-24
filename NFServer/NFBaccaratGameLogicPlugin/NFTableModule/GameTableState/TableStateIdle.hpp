#ifndef TABLE_STATE_IDLE_HPP
#define TABLE_STATE_IDLE_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateIdle :public ITableState
{
public:
    TableStateIdle(){}

    TableStateIdle(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_IDLE;
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        toState = tsType;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();

        //m_pLogModule->LogInfo("Enter State of Idle !");
        tableIdleTimes = 20;
        toState = TableState::TABLE_CONTINUE;
        m_pGameTable->RoundStopClearData();
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        if (tableIdleTimes <= 0)
        {
            //m_pLogModule->LogInfo("run record start!", __FUNCTION__, __LINE__);
            m_pGameTable->DealCard();
            m_pGameTable->FillCard();
            m_pGameTable->ResultAreaAndRecord();
            m_pGameTable->RoundStopClearData();
            tableIdleTimes = 20;
            if (m_pGameTable->GetRealUserCount() > 0)
                SetStateTo(TableState::TABLE_WAIT_GAME_START); 
            //m_pLogModule->LogInfo("run record end!", __FUNCTION__, __LINE__);
        }
        tableIdleTimes--;
        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {

    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Idle !");
        m_pGameTable->SetTimeCount(3);
        m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
    }
private:
	NF_MACHINE_STATE toState;
    int tableIdleTimes;

};

#endif