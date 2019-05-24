#include "NFBaccaratExpendLogic.h"
#include "NFServer/NFBaccaratGameLogicPlugin/NFTableModule/StructHPP/BetArea.hpp"
#include "NFCItemNotBankerFillProcessModule.h"
#include "NFCItemBankerFillProcessModule.h"

NFBaccaratExpendLogic::NFBaccaratExpendLogic(NFIPluginManager * p)
{
	pPluginManager = p;
}

NFBaccaratExpendLogic::~NFBaccaratExpendLogic()
{

}

void NFBaccaratExpendLogic::Init()
{
	lossPerCentMap.insert(make_pair(WagerAreaType::DEALER, 0.95f));
	lossPerCentMap.insert(make_pair(WagerAreaType::NOT_DEALEAR, 1));
	lossPerCentMap.insert(make_pair(WagerAreaType::DOGFALL, 8));
	lossPerCentMap.insert(make_pair(WagerAreaType::DEALER_DOUBLE, 11));
	lossPerCentMap.insert(make_pair(WagerAreaType::NOT_DEALEAR_DOUBLE, 11));
	lossPerCentMap.insert(make_pair(WagerAreaType::ARBITRARILY_DOUBLE, 5));
	lossPerCentMap.insert(make_pair(WagerAreaType::PERFECT_DOUBLE, 20));
	lossPerCentMap.insert(make_pair(WagerAreaType::SMALL, 1.5f));
	lossPerCentMap.insert(make_pair(WagerAreaType::BIG, 0.5f));

	ResgisterFillProcessModule(CardID::DEALER_ID, NF_SHARE_PTR<NFIItemFillProcessModule>(NF_NEW NFCItemBankerFillProcessModule(pPluginManager)));
	ResgisterFillProcessModule(CardID::NOT_DEALER_ID, NF_SHARE_PTR<NFIItemFillProcessModule>(NF_NEW NFCItemNotBankerFillProcessModule(pPluginManager)));

	for (NF_SHARE_PTR<NFIItemFillProcessModule> xModule = First(); xModule != nullptr; xModule = Next())
	{
		xModule->Init();
	}
}


bool NFBaccaratExpendLogic::ResgisterFillProcessModule(const int nModuleType, NF_SHARE_PTR<NFIItemFillProcessModule> pModule)
{
	if (ExistElement(nModuleType))
	{
		assert(0);
		return false;
	}

	return AddElement(nModuleType, pModule);
}

NF_SHARE_PTR<NFIItemFillProcessModule> NFBaccaratExpendLogic::GetFillProcessModule(const int nModuleType)
{
	return GetElement(nModuleType);
}

float NFBaccaratExpendLogic::GetLossPerCentByBetId(int betId)
{
	if (lossPerCentMap.find(betId) == lossPerCentMap.end())
		return -1;

	return lossPerCentMap[betId];
}
