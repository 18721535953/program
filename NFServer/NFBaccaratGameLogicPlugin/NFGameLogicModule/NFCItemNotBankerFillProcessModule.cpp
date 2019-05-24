#include "NFCItemNotBankerFillProcessModule.h"
#include "../NFTableModule/NFGameTable.h"


NFCItemNotBankerFillProcessModule::NFCItemNotBankerFillProcessModule(NFIPluginManager* p)
{
    pPluginManager = p;
}

bool NFCItemNotBankerFillProcessModule::Init()
{
    return true;
}

bool NFCItemNotBankerFillProcessModule::AfterInit()
{
    return true;
}

bool NFCItemNotBankerFillProcessModule::FillCardProcess(NFGameTable* table)
{
    BetRegions & regions = table->GetBetRegions();
    int tempCardValue = regions.GetCardsOfCard_ID(CardID::NOT_DEALER_ID)->GetCardValueFirstTwoTimes();

    if ((tempCardValue >= 0 && tempCardValue <= 5) && (regions.GetCardsOfCard_ID(CardID::DEALER_ID)->GetCardValueFirstTwoTimes() < 8))
        return true;

    return false;
}

bool NFCItemNotBankerFillProcessModule::Shut()
{
    return true;
}