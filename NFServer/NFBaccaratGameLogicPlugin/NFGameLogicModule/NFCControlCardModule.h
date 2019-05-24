#ifndef NFC_CONTROL_CARD_MODULE_H
#define NFC_CONTROL_CARD_MODULE_H
#include <vector>
#include "NFComm/NFPluginModule/NFIControlCardModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "../NFTableModule/NFGameTable.h"
#include "../NFTableModule/StructHPP/CardRecordItem.hpp"

struct TheSelectedAreaItem
{
    std::string areaString; //区域
    int64_t gold;           //区域对应的金币（赢）
};

class NFCControlCardModule
    :public NFIControlCardModule
{
public:
    NFCControlCardModule(NFIPluginManager * p);

    ~NFCControlCardModule();

    virtual bool Awake() override;
    virtual bool Init() override;
    virtual bool AfterInit() override;

    virtual CardRecordItem ControlCardOfTable(int resultCode, NFGameTable* table) override;

    bool ReadRecord();

private:
    // 让玩家赢
    CardRecordItem ControlWin();
    // 让玩家输
    CardRecordItem ControlLoss();

    /************************************************************************/
    /*根据已选择的区域 控制牌型                                             */
    /*@pTable : 桌子                                                        */
    /*@areaStr : 已选中的区域                                               */
    /************************************************************************/
    CardRecordItem ControlCardsByArea(NFGameTable& pTable, const string& areaStr);

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    bool PushToVectorOfTheAreaWithTheType(const std::vector<std::string> & originVec ,std::vector<CardRecordItem> & cardRecordVector);

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    void PushToVectorOfTheAreaWithTheType(const std::map<string, int64_t> & areaMap, std::vector<CardRecordItem> & cardRecordVector);

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    int GetDogallCountOfOriginVec(const std::vector<std::string> & originVec);

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    bool RandomAResultFromAreaVector(const std::vector<CardRecordItem> & cardRecordVector, CardRecordItem & tempRecrdItem);

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    void SplitAreaString(const string areaString,vector<int> & areaVector);



private:

    NFILogModule        * m_pLogModule;
    NFIGameLogicModule  * m_pGameLogicModule;
    NFGameTable         * table; //当前要控制的桌子
	std::vector<CardRecordItem>         recordVec;
	//typedef std::vector<CardRecordItem> RECORD_VEC;
	std::map<std::string, std::vector<CardRecordItem>>   recordMap;
};

#endif
