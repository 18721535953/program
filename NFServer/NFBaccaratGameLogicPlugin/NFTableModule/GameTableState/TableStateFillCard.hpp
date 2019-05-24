#ifndef TABLE_STATE_FILL_CARD_HPP
#define TABLE_STATE_FILL_CARD_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateFillCard :public ITableState
{
public:
    TableStateFillCard(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_FILL_CARD;
    }

    void Enter() override
    {
        if (!pPluginManager) assert(0);

        if (!m_pLogModule) m_pLogModule = pPluginManager->FindModule<NFILogModule>();

        toState = TableState::TABLE_CONTINUE;

        //m_pLogModule->LogInfo("Enter State of Fill Card !");
    }

    void SetStateTo(NF_MACHINE_STATE tsType) override
    {
        toState = tsType;
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        // TODO  暂时把控制写死
        bool isControl = true;
        bool isSuccess = false;

        if (isControl)
            isSuccess = m_pGameTable->ControlCard();

        if (!isSuccess)
            m_pGameTable->FillCard();

        SetStateTo(TableState::TABLE_CALCULATE_RESULT);

        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {

    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Fill Card !");

        m_pGameTable->SetTimeCount(7);
        m_pGameTable->SendTableStateToUserAtTheSameTable((TableState)toState, tusWaitNextRound);
    }
private:
	NF_MACHINE_STATE toState;
};

#endif