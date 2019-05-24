#ifndef NFI_ITEM_FILL_PROCESS_MODULE_H
#define NFI_ITEM_FILL_PROCESS_MODULE_H
#include "NFIModule.h"

class NFGameTable;

class NFIItemFillProcessModule :public NFIModule
{
public:
    virtual bool FillCardProcess(NFGameTable* table) = 0;
};

#endif
