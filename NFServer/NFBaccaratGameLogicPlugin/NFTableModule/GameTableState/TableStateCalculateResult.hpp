#ifndef TABLE_STATE_CALCULATE_RESULT_HPP
#define TABLE_STATE_CALCULATE_RESULT_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateCalculateResult :public ITableState
{
public:
    TableStateCalculateResult(){}

    TableStateCalculateResult(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_CALCULATE_RESULT;
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        toState = tsType;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();
        
        //m_pLogModule->LogInfo("Enter State of Calculate Result !");
        CalculateDate();

        m_pGameTable->SetTimeCount(10);

        toState = TableState::TABLE_CONTINUE;
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        m_pGameTable->MinusTimeCount(1);

        if (m_pGameTable->GetTimeCount() <= 0)
            SetStateTo(TableState::TABLE_WAIT_FOR_NEXT_GAME);

        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {

    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Calculate Result !");
        m_pGameTable->SetTimeCount(5);
        m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
    }
private:
    void CalculateDate()
    {
        m_pGameTable->CheckOfflineOfTableUser();

        m_pGameTable->CalcualteResult();
        if (m_pGameTable->GetAllBetBean() > 0)
            m_pGameTable->StartReportScore();
        else
            m_pGameTable->RoundStopClearData();
    }
private:
	NF_MACHINE_STATE toState;
};

#endif