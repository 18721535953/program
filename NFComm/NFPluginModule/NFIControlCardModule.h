#ifndef NFI_CONTROL_CARD_MODULE_H
#define NFI_CONTROL_CARD_MODULE_H
#include "NFIModule.h"
#include "../../NFServer/NFBaccaratGameLogicPlugin/NFTableModule/StructHPP/CardRecordItem.hpp"
class NFGameTable;

class NFIControlCardModule :public NFIModule
{
public:
    virtual CardRecordItem ControlCardOfTable(int resultCode, NFGameTable* table) = 0;
};

#endif