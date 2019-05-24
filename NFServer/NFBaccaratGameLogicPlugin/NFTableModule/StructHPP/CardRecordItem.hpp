#ifndef CARD_RECORD_ITEM_HPP
#define CARD_RECORD_ITEM_HPP
#include <string>
#include <vector>
#include "NFComm/NFCore/NFUtil.hpp"

class CardRecordItem
{
public:
    std::vector<int> dealerCardVec;
    std::vector<int> notDealCardVec;

    CardRecordItem()
    {
        /* cardCount = 0;
        notDealCardCount = 0;*/
    }

    inline void SetDealValue(std::string cardStr)
    {
        dealerCardVec.clear();
        std::vector<std::string> cardVec;
        NFUtil::SplitStringToVector(cardStr, cardVec, ",");

        for (size_t i = 0; i < cardVec.size(); i++)
        {

            dealerCardVec.push_back(std::atoi(cardVec[i].c_str()));
            //LogInfo("SetDealValue  ", " %d ", dealerCardVec[i]);
        }
    }

    inline void SetNotDealValue(std::string cardStr)
    {
        notDealCardVec.clear();
        std::vector<std::string> cardVec;
		NFUtil::SplitStringToVector(cardStr, cardVec, ",");

        for (size_t i = 0; i < cardVec.size(); i++)
        {
            notDealCardVec.push_back(std::atoi(cardVec[i].c_str()));
        }
    }


    inline int GetDealCardTotalValue()
    {
        return GetCardsTotalValue(dealerCardVec);
    }

    inline int GetNotDealCardTotalValue()
    {
        return GetCardsTotalValue(notDealCardVec);
    }

    inline bool isEqualToCardVec()
    {
        if (GetDealCardTotalValue() == GetNotDealCardTotalValue())
        {
            return true;
        }
        else
        {
            return  false;
        }
    }

    // 获得头两张牌的值
    inline int GetCardValueFirstTwoTimes(std::vector<int> CardVec)
    {
        if (CardVec.size() <= 0)
            return 0;

        int totalValue = 0;
        for (int i = 0; i < 2; i++)
        {
            int tempValue = CardVec[i] / 10;
            tempValue = tempValue > 9 ? 0 : tempValue;
            totalValue += tempValue;
        }
        totalValue = totalValue > 9 ? totalValue % 10 : totalValue;
        return totalValue;
    }

    // 获取全部牌值
    inline int GetCardsTotalValue(std::vector<int> CardVec)
    {
        if (CardVec.size() <= 0)
            return 0;

        if (CardVec.size() <= 2)
            return GetCardValueFirstTwoTimes(CardVec);

        int totalValue = GetCardValueFirstTwoTimes(CardVec);

        int tempValue = CardVec[2] / 10;
        tempValue = tempValue > 9 ? 0 : tempValue;

        totalValue += tempValue;
        totalValue = totalValue > 9 ? totalValue % 10 : totalValue;

        return totalValue;
    }

    inline int GetDealerCount()
    {
        return (int)dealerCardVec.size();
    }

    inline int GetNotDealerCount()
    {
        return (int)notDealCardVec.size();
    }

    inline std::vector<int> GetDealCardList()
    {
        return dealerCardVec;
    }

    inline std::vector<int> GetNotDealCardList()
    {
        return notDealCardVec;
    }

    inline bool IsEqualOfTwoCards()
    {
        return (GetCardsTotalValue(dealerCardVec) == GetCardsTotalValue(notDealCardVec));
    }

    inline std::string GetCardTypeStr()
    {
        std::string tempStr = "";
        for (size_t i = 0; i < notDealCardVec.size(); i++)
        {
            tempStr += std::to_string(notDealCardVec[i]);
            if ((i + 1) < notDealCardVec.size())
                tempStr += ",";
        }

        tempStr += "|";

        for (size_t i = 0; i < dealerCardVec.size(); i++)
        {
            tempStr += std::to_string(dealerCardVec[i]);
            if ((i + 1) < dealerCardVec.size())
                tempStr += ",";
        }

        return tempStr;
    }
};

#endif
