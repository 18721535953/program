#ifndef NFC_ITEM_NOT_BANKER_FILL_PROCESS_MODULE_H
#define NFC_ITEM_NOT_BANKER_FILL_PROCESS_MODULE_H
#include "../../../NFComm/NFPluginModule/NFIItemFillProcessModule.h"

class NFCItemNotBankerFillProcessModule
    :public NFIItemFillProcessModule
{
public:
    NFCItemNotBankerFillProcessModule(NFIPluginManager* p);

    ~NFCItemNotBankerFillProcessModule(){}

    virtual bool Init() override;
    virtual bool AfterInit() override;
    virtual bool Shut() override;

    virtual bool FillCardProcess(NFGameTable* table) override;
};

#endif
