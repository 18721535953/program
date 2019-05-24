#ifndef C_PLAYER_CARDS_HPP
#define C_PLAYER_CARDS_HPP

#include "GameConfig.hpp"
#include "Dependencies/json/json.hpp"
#include <cstdint>
#include <vector>
#include <string>



#define CHANGE_TO_CATTLEVALUE(value) (min(value,(int)scv10))


struct TGameCard
{
    int Color;
    int Value;

    TGameCard(){ Clear(); }
    bool IsValid(){ return (Color >= sccDiamond && Color <= sccSpade && Value >= CARD_A && Value <= CARD_K); }
    void Clear(){ Color = sccNone;Value = CARD_NONE; }
    void CopyFrom(TGameCard& src){ Color = src.Color; Value = src.Value; }
    int Compare(TGameCard& dec)
    {
        int ret = Value - dec.Value;
        if (0 == ret)
            ret = Color - dec.Color;

        return ret;
    }
};
// 玩家牌
class CPlayerCard
{
public:
    CPlayerCard(int index){ Clear(); this->selfId = index; }
    CPlayerCard(){ Clear(); }
    ~CPlayerCard(){}
    void Clear()
    {
        m_cardCount = 0;
        m_isSort = false;

        for (int i = 0; i < MAX_USER_CARD_COUNT; i++)
        {
            m_cardList[i] = -1;
        }
    }

    void WriteToPluto(nlohmann::json& u)
    {
		 std::string str = "";
		 for (int i = 0; i < this->m_cardCount; ++i)
		 {
			 str += std::to_string(m_cardList[i]);
			 if ((i + 1) < m_cardCount)
				 str += ",";
		 }
		 const char* c_s = str.c_str();
		 u["cards"] = std::to_string(m_cardList[0]);
    }// only for deal card


    void AddCard(int value)
    {
        m_cardList[m_cardCount] = value;
        m_cardCount++;
    }

    void FillCard(int value)
    {
        m_cardList[2] = value;
        m_cardCount = 3;
    }

    int getMaxCard()
    {
        int max = 0;
        for (int i = 0; i < m_cardCount; ++i)
        {
            if (m_cardList[i]>max)
            {
                max = m_cardList[i];
            }
        }
        return max;
    }

    int* getCardList()
    {
        return this->m_cardList;
    }

    int32_t getSelfID()
    {
        return selfId;
    }

    void SetSelfID(int32_t value)
    {
        this->selfId = value;
    }

    int getCardCount()
    {
        return m_cardCount;
    }

    void SetCardCount(int count)
    {
        m_cardCount = count;
    }

    // 是否是完美对子
    bool IsProfectPairOfCard()
    {
        if (m_cardCount < 2)
            return false;

        // 只有首兩張牌是對子才行
        if (m_cardList[0] == m_cardList[1])
        {
            return true;
        }

        return false;
    }


    //是否是对子
    bool IsPairOfCard()
    {
        if (m_cardCount < 2)
            return false;

        // 只有首兩張牌是對子才行
        if ((m_cardList[0] / 10) == (m_cardList[1] / 10))
        {
            return true;
        }

        return false;
    }

    // 获得头两张牌的值
    int GetCardValueFirstTwoTimes()
    {
        if (m_cardCount <= 0)
            return 0;

        int totalValue = 0;
        for (int i = 0; i < 2; i++)
        {
            int tempValue = m_cardList[i] / 10;
            tempValue = tempValue > 9 ? 0 : tempValue;
            totalValue += tempValue;
        }
        totalValue = totalValue > 9 ? totalValue % 10 : totalValue;
        return totalValue;
    }

    // 获取全部牌值
    int GetCardsTotalValue()
    {
        if (m_cardCount <= 0)
            return 0;

        if (m_cardCount <= 2)
            return GetCardValueFirstTwoTimes();

        int totalValue = GetCardValueFirstTwoTimes();

        int tempValue = m_cardList[2] / 10;
        tempValue = tempValue > 9 ? 0 : tempValue;

        totalValue += tempValue;
        totalValue = totalValue > 9 ? totalValue % 10 : totalValue;

        return totalValue;
    }

    std::string GetCardsStr()
    {
        std::string str = "";
        for (int i = 0; i < m_cardCount; i++)
        {
            str += std::to_string(m_cardList[i]);
            if ((i + 1) < m_cardCount)
            {
                str += ",";
            }
        }

        return str;
    }

    void SetCardList(std::vector<int> cardList)
    {
        if (cardList.size() > 3)
            return;

        m_cardCount = 0;
        for (size_t i = 0; i < cardList.size(); i++)
        {
            this->m_cardList[i] = cardList[i];
            m_cardCount++;
        }
    }

    void ReadCardList(int* cards)
    {
        for (int i = 0; i < MAX_USER_CARD_COUNT; i++)
        {
            cards[i] = this->m_cardList[i];
        }
    }

private:
    int m_cardCount;                            // will not be changed after deal
    int32_t m_cardList[MAX_USER_CARD_COUNT];        // will not be changed after deal
    bool m_isSort;
    // 百家乐用  庄家是1 闲家是2
    int32_t selfId;
};

// 游戏用户信息
class CPlayerUserInfo
{
public:
    CPlayerUserInfo()
    {
        Clear();
    }
    ~CPlayerUserInfo(){}
    void Clear(){ cardList.Clear(); }
    void RoundClear(){ cardList.Clear(); }
public:
    CPlayerCard cardList;
};

#endif