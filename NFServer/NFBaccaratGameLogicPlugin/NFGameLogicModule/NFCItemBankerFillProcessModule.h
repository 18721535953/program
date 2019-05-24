#ifndef NFC_ITEM_BANKER_FILL_PROCESS_MODULE_H
#define NFC_ITEM_BANKER_FILL_PROCESS_MODULE_H
#include "../../../NFComm/NFPluginModule/NFIItemFillProcessModule.h"

class CPlayerCard;

class NFCItemBankerFillProcessModule
    :public NFIItemFillProcessModule
{
public:
    NFCItemBankerFillProcessModule(NFIPluginManager* p);

    ~NFCItemBankerFillProcessModule();

    virtual bool Init() override;
    virtual bool AfterInit() override;
    virtual bool Shut() override;

    virtual bool FillCardProcess(NFGameTable* table) override;

    bool CheckFillCard0_1_2(int notDealerCardCount, NF_SHARE_PTR<CPlayerCard>& notDealearCards);

    bool CheckFillCard3(int notDealerCardCount, NF_SHARE_PTR<CPlayerCard>& notDealearCards);

    bool CheckFillCard4(int notDealerCardCount, NF_SHARE_PTR<CPlayerCard>& notDealearCards);

    bool CheckFillCard5(int notDealerCardCount, NF_SHARE_PTR<CPlayerCard>& notDealearCards);

    bool CheckFillCard6(int notDealerCardCount, NF_SHARE_PTR<CPlayerCard>& notDealearCards);

};

#endif