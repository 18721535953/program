#include "NFCItemBankerFillProcessModule.h"
#include "../NFTableModule/NFGameTable.h"



NFCItemBankerFillProcessModule::NFCItemBankerFillProcessModule(NFIPluginManager* p)
{
    pPluginManager = p;
}

NFCItemBankerFillProcessModule::~NFCItemBankerFillProcessModule()
{

}

bool NFCItemBankerFillProcessModule::Init()
{
    return true;
}

bool NFCItemBankerFillProcessModule::AfterInit()
{
    return true;
}

bool NFCItemBankerFillProcessModule::Shut()
{
    return true;
}

bool NFCItemBankerFillProcessModule::FillCardProcess(NFGameTable* table)
{
    if (!table) assert(0);

    BetRegions & m_regions = table->GetBetRegions();
    NF_SHARE_PTR<CPlayerCard> bankerCards = m_regions.GetCardsOfCard_ID(CardID::DEALER_ID);
    NF_SHARE_PTR<CPlayerCard> notBankerCards = m_regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID);
    // 获取庄家头两张牌的总和
    int tempCardValue = bankerCards->GetCardValueFirstTwoTimes();

    // 獲取閑家的牌值總和
    int tempNotDealValue = notBankerCards->GetCardValueFirstTwoTimes();
    /*LogInfo("庄家牌的总值：", "%d", tempCardValue);*/
    if (tempCardValue >= 7 || tempNotDealValue > 7) //不需要补牌
        return false;

    // 闲家的牌数量
    int notDealerCardCount = notBankerCards->getCardCount();

    // 头牌总值是 [0-1-2] 的补牌
    if (tempCardValue >= 0 && tempCardValue <= 2)
        return CheckFillCard0_1_2(notDealerCardCount, notBankerCards);

    // 头牌总值是 [3]的补牌
    if (tempCardValue == 3)
        return CheckFillCard3(notDealerCardCount, notBankerCards);

    // 头牌总值是 [4]的补牌
    if (tempCardValue == 4)
        return CheckFillCard4(notDealerCardCount, notBankerCards);

    // 头牌总值是 [5]的补牌
    if (tempCardValue == 5)
        return CheckFillCard5(notDealerCardCount, notBankerCards);

    // 头牌总值是 [6]的补牌
    if (tempCardValue == 6)
        return CheckFillCard6(notDealerCardCount, notBankerCards);

    return false;
}

bool NFCItemBankerFillProcessModule::CheckFillCard0_1_2(int notDealerCardCount, std::shared_ptr<CPlayerCard>& notDealearCards)
{
    return true;
}

bool NFCItemBankerFillProcessModule::CheckFillCard3(int notDealerCardCount, std::shared_ptr<CPlayerCard>& notDealearCards)
{
    if (notDealerCardCount == 2)
        return true;

    if (notDealerCardCount > 2)
    {
        int tempCardValueNotDealer = notDealearCards->getCardList()[notDealerCardCount - 1] / 10;
        tempCardValueNotDealer = tempCardValueNotDealer > 9 ? 0 : tempCardValueNotDealer;
        if (tempCardValueNotDealer == 8)
            return false;

    }
    return true;
}

bool NFCItemBankerFillProcessModule::CheckFillCard4(int notDealerCardCount, std::shared_ptr<CPlayerCard>& notDealearCards)
{
    if (notDealerCardCount == 2)
        return true;

    if (notDealerCardCount > 2)
    {
        int tempCardValueNotDealer = notDealearCards->getCardList()[notDealerCardCount - 1] / 10;
        tempCardValueNotDealer = tempCardValueNotDealer > 9 ? 0 : tempCardValueNotDealer;
        if (tempCardValueNotDealer == 0 || tempCardValueNotDealer == 1
            || tempCardValueNotDealer == 8 || tempCardValueNotDealer == 9)
            return false;

    }
    return true;
}

bool NFCItemBankerFillProcessModule::CheckFillCard5(int notDealerCardCount, std::shared_ptr<CPlayerCard>& notDealearCards)
{
    if (notDealerCardCount == 2)
        return true;

    if (notDealerCardCount > 2)
    {
        int tempCardValueNotDealer = notDealearCards->getCardList()[notDealerCardCount - 1] / 10;
        tempCardValueNotDealer = tempCardValueNotDealer > 9 ? 0 : tempCardValueNotDealer;
        if ((tempCardValueNotDealer >= 0 && tempCardValueNotDealer <= 3)
            || tempCardValueNotDealer == 8 || tempCardValueNotDealer == 9)
            return false;

    }
    return true;
}

bool NFCItemBankerFillProcessModule::CheckFillCard6(int notDealerCardCount, std::shared_ptr<CPlayerCard>& notDealearCards)
{
    if (notDealerCardCount == 2)
        return false;

    if (notDealerCardCount > 2)
    {
        int tempCardValueNotDealer = notDealearCards->getCardList()[notDealerCardCount - 1] / 10;
        tempCardValueNotDealer = tempCardValueNotDealer > 9 ? 0 : tempCardValueNotDealer;
        if ((tempCardValueNotDealer >= 0 && tempCardValueNotDealer <= 5)
            || tempCardValueNotDealer == 8 || tempCardValueNotDealer == 9)
            return false;

    }
    return true;
}
