#ifndef NF_BACCARAT_EXPEND_LOGIC_H
#define NF_BACCARAT_EXPEND_LOGIC_H
#include "NFComm/NFCore/NFMapEx.hpp"
#include "NFComm/NFPluginModule/NFIItemFillProcessModule.h"

class NFBaccaratExpendLogic
	:NFMapEx<int, NFIItemFillProcessModule>
{
public:
	NFBaccaratExpendLogic(NFIPluginManager * p);
	~NFBaccaratExpendLogic();

	void Init();

	bool ResgisterFillProcessModule(const int nModuleType, NF_SHARE_PTR<NFIItemFillProcessModule> pModule);

	NF_SHARE_PTR<NFIItemFillProcessModule> GetFillProcessModule(const int nModuleType);

	float GetLossPerCentByBetId(int betId);

private:
	NFIPluginManager * pPluginManager;

	map<int, float> lossPerCentMap;          // ≈‚¬ 
};

#endif
