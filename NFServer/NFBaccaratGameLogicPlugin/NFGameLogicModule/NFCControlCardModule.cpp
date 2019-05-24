#include "NFCControlCardModule.h"
#include "../NFTableModule/NFGameTable.h"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFBaccaratExpendLogic.h"

NFCControlCardModule::NFCControlCardModule(NFIPluginManager* p)
{
    pPluginManager = p;
}

NFCControlCardModule::~NFCControlCardModule()
{
    
}

bool NFCControlCardModule::Awake()
{
    return true;
}

bool NFCControlCardModule::Init()
{
    m_pLogModule = pPluginManager->FindModule<NFILogModule>();
    m_pGameLogicModule = pPluginManager->FindModule<NFIGameLogicModule>();
    //ReadRecord();
    return true; 
}

bool NFCControlCardModule::AfterInit()
{
    return true;
}

bool NFCControlCardModule::ReadRecord()
{
    /* 读取配置文件 */

    std::ifstream in("BaccaratCardTypeRecord.txt");

    if (in.is_open())
    {
        m_pLogModule->LogInfo("Open Config File!!!!!!", __FUNCTION__, __LINE__);
    }
    else
    {
        m_pLogModule->LogInfo("Can't open a config File!", __FUNCTION__, __LINE__);
    }
    std::string s;

    int recordCount = 0;
    while (std::getline(in, s))
    {
        recordCount++;
        std::vector<std::string> readLineVec;
		NFUtil::SplitStringToVector(s, readLineVec, ":");
        std::vector<std::string> readLineCardVec;
		NFUtil::SplitStringToVector(readLineVec[1], readLineCardVec, "|");

        m_pLogModule->LogInfo(lexical_cast<string>(recordCount), __FUNCTION__, __LINE__);

        if (recordMap.find(readLineVec[0]) == recordMap.end())
        {
            std::vector<CardRecordItem> temp;
            CardRecordItem tempRecordItem;
            tempRecordItem.SetDealValue(readLineCardVec[1]);
            tempRecordItem.SetNotDealValue(readLineCardVec[0]);
            temp.push_back(tempRecordItem);
            recordMap.insert(std::map<std::string, std::vector<CardRecordItem>>::value_type(readLineVec[0], temp));
        }
        else
        {
            std::map<std::string, std::vector<CardRecordItem>>::iterator it = recordMap.find(readLineVec[0]);
            CardRecordItem tempRecordItem;
            tempRecordItem.SetDealValue(readLineCardVec[1]);
            tempRecordItem.SetNotDealValue(readLineCardVec[0]);
            it->second.push_back(tempRecordItem);
        }
    }

    in.close();
    return true;
}

CardRecordItem NFCControlCardModule::ControlCardOfTable(int resultCode, NFGameTable* table)
{
    if (!table) assert(0);

    this->table = table;

    if (resultCode == 1)
        return ControlLoss();
    else
        return ControlWin();
}

CardRecordItem NFCControlCardModule::ControlWin()
{
    if (!table) assert(0);

    CardRecordItem tempRecrdItem;
    std::map<string, int64_t> areaMap;
    std::vector<std::string> perVector;
    int64_t maxAllBean = 0;
    string area = "";


    for (auto temp : recordMap)
    {
        vector<int> areaVec;

        const string tempArea = temp.first;
        SplitAreaString(tempArea, areaVec);

        int64_t tempSumBean = 0; // 投注区域赢得钱
        int64_t tempBetAllSum = 0; // 投注区域的本金
        for (size_t i = 0; i < areaVec.size(); i++)
        {
            int64_t tempBean = table->GetBeanByRegionId(areaVec[i]);
            if (tempBean <= 0)
                continue;

            tempBetAllSum += tempBean;
            float lossPerCent = m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(areaVec[i]);
            tempBean *= lossPerCent;
            tempSumBean += tempBean;

        }

        // 如果这些区域投注的钱大于所有玩家下注的钱
        if (tempSumBean > (table->GetAllBetBean() - tempBetAllSum))
        {
            areaMap.insert(make_pair(temp.first, tempSumBean));

            if (tempSumBean <= table->GetScore())
                perVector.push_back(temp.first);
        }

    }

    if (areaMap.size() == 0) // 如果 == 0 ，说明这一句中玩家必输 随机选取一个区域即可
    {
        m_pLogModule->LogInfo("There is not CARDS of that condition,so random an area!!!", __FUNCTION__, __LINE__);
        return tempRecrdItem;
    }

    // 如果没有小于等于limitOfGold的区域,则让玩家输
    if (perVector.size() == 0)
    {
        m_pLogModule->LogInfo("There is no region less than or equal to limitOfGold,so let the player lose!!", __FUNCTION__, __LINE__);
        return ControlLoss();
    }

    // 判断区域是否符合要求
    std::vector<CardRecordItem> cardRecordItemVec;

    if (!PushToVectorOfTheAreaWithTheType(perVector, cardRecordItemVec))
    {
        m_pLogModule->LogWarning("There is no area that meets the requirements!!! So let the player lose!!! ", __FUNCTION__, __LINE__);
        return ControlLoss();
    }

    if (!RandomAResultFromAreaVector(cardRecordItemVec, tempRecrdItem))
        return ControlLoss();

    if (tempRecrdItem.isEqualToCardVec() )
    {
        if (NFUtil::GetRandomRange(0, 10) < 6)
            return ControlLoss();
    }

    if (tempRecrdItem.dealerCardVec.size() == 0)
        return ControlLoss();

    return tempRecrdItem;
}

CardRecordItem NFCControlCardModule::ControlLoss()
{
    if (!table) assert(0);

    CardRecordItem tempRecrdItem;
    map<string, int64_t> areaMap;
    int64_t minAllBean = 0;
    string area = "";
    std::vector<CardRecordItem> cardRecordItemVec;
    vector<std::string> perVector;

    for (auto temp : recordMap)
    {
        vector<int> areaVec;
        const string tempArea = temp.first;
        SplitAreaString(tempArea, areaVec);
       
        int64_t tempSumBean = 0; // 投注区域赢的钱
        int64_t tempBetAllSum = 0; // 投注区域的本金
        int64_t betAllSumOnTable = table->GetAllBetBean();
        for (size_t i = 0; i < areaVec.size(); i++)
        {
            // 如果 最终区域中有和 ， 则 减去庄、闲中的本金
            if (areaVec[i] == WagerAreaType::DOGFALL)
            {
                betAllSumOnTable -= table->GetBeanByRegionId(WagerAreaType::DEALER);
                betAllSumOnTable -= table->GetBeanByRegionId(WagerAreaType::NOT_DEALEAR);
            }

            int64_t tempBean = table->GetBeanByRegionId(areaVec[i]);
            if (tempBean <= 0)
                continue;

            tempBetAllSum += tempBean;
            float lossPerCent = m_pGameLogicModule->m_pNFBaccaratExpendLogic->GetLossPerCentByBetId(areaVec[i]);
            tempBean *= lossPerCent;
            tempSumBean += tempBean;
        }

        // 如果这些区域投注的钱((扣除赢的区域的本金))小于所有玩家下注的钱
        if (tempSumBean <= (betAllSumOnTable - tempBetAllSum))
        {
            areaMap.insert(make_pair(temp.first, tempSumBean));
            // 找出 在玩家赢钱小于等于 百分之30 的区域

           // float per = tempSumBean / (betAllSumOnTable - tempBetAllSum);
            if ((betAllSumOnTable - tempBetAllSum) > 0)
            {
                float tempSum = (betAllSumOnTable - tempBetAllSum) * 0.3f;
                if (tempSumBean <= tempSum)
                    perVector.push_back(temp.first);
            }

        }
            
    }

    if (areaMap.size() == 0) // 如果 == 0 ， 随机选取一个区域
    {
        m_pLogModule->LogWarning("There are not CARDS of that condition,so random an area.", __FUNCTION__, __LINE__);
        return tempRecrdItem;
    }

    // 如果 没有小于等于百分之30的区域 则 选择最大的(输)
    if (perVector.size() == 0)
    {
        auto iter = areaMap.begin();
        int64_t minArea = iter->second;
        m_pLogModule->LogInfo("There is no region less than or equal to 30 percent!!", __FUNCTION__, __LINE__);
        
        for (iter++; iter != areaMap.end(); iter++)
        {
            int64_t perAraeGold = iter->second;
            if (perAraeGold < minArea)
            {
                minArea = perAraeGold;
                area = iter->first;
            }
        }
    }

    // 如果 area 不为空 说明 没有小于等于半分之30的区域
    if (area != "")
    {
        tempRecrdItem = ControlCardsByArea(*table, area);
        // 如果没有找到符合的牌型 在现有找到的所有 输的区域中，选择一个输的最多的
        // 先找到 符合 条件的记录
        if (tempRecrdItem.dealerCardVec.size() == 0)
        {
            m_pLogModule->LogInfo("No much type was found.", __FUNCTION__, __LINE__);
            PushToVectorOfTheAreaWithTheType(areaMap, cardRecordItemVec);
            RandomAResultFromAreaVector(cardRecordItemVec, tempRecrdItem);
        }
    }
    else
    {
        if (!PushToVectorOfTheAreaWithTheType(perVector, cardRecordItemVec))
            PushToVectorOfTheAreaWithTheType(areaMap, cardRecordItemVec);

        RandomAResultFromAreaVector(cardRecordItemVec, tempRecrdItem);
    }
    return tempRecrdItem;
}


CardRecordItem NFCControlCardModule::ControlCardsByArea(NFGameTable& pTable, const string& areaStr)
{
    CardRecordItem  temp;
    // 该区域没有记录的牌型
    if (recordMap.find(areaStr) == recordMap.end())
    {
        string str = "Can not find the record of == ";
        str.append(areaStr);
        m_pLogModule->LogInfo(str);
        return temp;
    }

    map<string, std::vector<CardRecordItem>> ::iterator it = recordMap.find(areaStr);
    int cardDealOne = pTable.GetCardValueByCardID(CardID::DEALER_ID, 0);
    int cardNotDealOne = pTable.GetCardValueByCardID(CardID::NOT_DEALER_ID, 0);

    vector<CardRecordItem> recordVec;

    if (it == recordMap.end())
    {
        m_pLogModule->LogInfo("Can not find the record of area = " + areaStr, __FUNCTION__, __LINE__);
        return temp;
    }

    for (auto temp : it->second)
    {
        if (cardDealOne == temp.dealerCardVec[0] && cardNotDealOne == temp.notDealCardVec[0])
            recordVec.push_back(temp);
    }

    if (recordVec.size() == 0)
    {
        m_pLogModule->LogWarning("There are not CARDS of this type .(The first card is different from the one show on the table !! )", __FUNCTION__, __LINE__);
        return temp;
    }

    int index = 0;
    do
    {
        index = NFUtil::GetRandomRange(0, (int)recordVec.size() - 1);
        temp = recordVec[index];

    } while ((temp.dealerCardVec.size() == 0) || (temp.isEqualToCardVec() && recordVec.size() >= 2 && NFUtil::GetRandomRange(0, 10) < 7));

    return temp;
}

bool NFCControlCardModule::PushToVectorOfTheAreaWithTheType(const std::vector<std::string> & originVec, std::vector<CardRecordItem> & cardRecordVector)
{
    cardRecordVector.clear();

    if ((originVec.size() == 1 && ControlCardsByArea(*table, originVec[0]).isEqualToCardVec())
        || (originVec.size() <= 3 && GetDogallCountOfOriginVec(originVec) >= 1))
    {
        if (NFUtil::GetRandomRange(0, 10) < 8) return false;
    }

    for (auto it : originVec)
    {
        CardRecordItem tempItem = ControlCardsByArea(*table, it);
        if (tempItem.dealerCardVec.size() != 0)
            cardRecordVector.push_back(tempItem);
    }
    return true;
}

void NFCControlCardModule::PushToVectorOfTheAreaWithTheType(const std::map<string, int64_t> & areaMap, std::vector<CardRecordItem> & cardRecordVector)
{
    auto iter = areaMap.begin();
    for (iter; iter != areaMap.end(); iter++)
    {
        CardRecordItem tempItem = ControlCardsByArea(*table, iter->first);
        if (tempItem.dealerCardVec.size() != 0)
            cardRecordVector.push_back(tempItem);
    }
}

bool NFCControlCardModule::RandomAResultFromAreaVector(const std::vector<CardRecordItem> & cardRecordVector, CardRecordItem & tempRecrdItem)
{
    int recordCount = (int)cardRecordVector.size();

    // 如果只有一组，不管是不是和  直接返回
    //if (recordCount == 1)
    //{
    //    tempRecrdItem = cardRecordVector[0];
    //    return true;
    //}

    if (recordCount > 0)
    {
        // 如果 结果有 “和” 则有一定的几率重新找牌  
        do
        {
            tempRecrdItem = cardRecordVector[NFUtil::GetRandomRange(0, recordCount - 1)];
        } while ((tempRecrdItem.isEqualToCardVec() || tempRecrdItem.dealerCardVec.size() == 0) && recordCount >= 2 && NFUtil::GetRandomRange(0, 10) <= 8);

        return true;
    }
    m_pLogModule->LogInfo("CardRecordVector is empty!!!", __FUNCTION__, __LINE__);
    return false;

}

int NFCControlCardModule::GetDogallCountOfOriginVec(const std::vector<std::string> & originVec)
{
    int count = 0;
    for (auto it : originVec)
    {
        if (ControlCardsByArea(*table, it).isEqualToCardVec()) count++;
    }
    return count;
}

void NFCControlCardModule::SplitAreaString(const string areaString, vector<int> & areaVector)
{
    char* tempAreaChar = new char[strlen(areaString.c_str()) + 1];
    strcpy(tempAreaChar, areaString.c_str());
    char* tempStr = strtok(tempAreaChar, ",");
    while (tempStr != nullptr)
    {
        areaVector.push_back(std::atoi(tempStr));
        tempStr = strtok(nullptr, ",");
    }
    delete[]tempAreaChar;
}
