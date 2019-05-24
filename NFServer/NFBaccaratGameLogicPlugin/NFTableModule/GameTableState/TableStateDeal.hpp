#ifndef TABLE_STATE_DEAL_HPP
#define TABLE_STATE_DEAL_HPP

#include "../NFGameTable.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginLoader/NFCPluginManager.h"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"

class TableStateDeal :public ITableState
{
public:
    TableStateDeal(NFGameTable* pGameTable, NFIPluginManager * p)
    {
        m_pGameTable = pGameTable;
        pPluginManager = p;
    }

	NF_MACHINE_STATE GetStateType() override
    {
        return TableState::TABLE_DEAL;
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

        //m_pLogModule->LogInfo("Enter State of Deal !");
    }

	NF_MACHINE_STATE Update(float fDeltaTime) override
    {
        // 发牌
        m_pGameTable->DealCard();

        // 通知客户端
        m_pGameTable->SendDealCardDataToClient();

        // 进入等待动画状态
        SetStateTo(TableState::TABLE_WITH_ANIMATION);

        return toState;
    }

    void FixedUpdate(float fixedDeltaTime) override
    {

    }

    void Exit() override
    {
        //m_pLogModule->LogInfo("Exit State of Deal !");
    }
private:
	NF_MACHINE_STATE toState;
};

#endif