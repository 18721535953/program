#ifndef BET_AREA_HPP
#define BET_AREA_HPP

#include <deque>
#include "CPlayerCards.hpp"
#include "NFComm/NFPluginModule/NFPlatform.h"
struct BetItem
{
    int betNum;
    int betId;
};

class CompareRegion
{
public:
    CompareRegion(){ clearCards(); }

    ~CompareRegion(){}

    void calculateValue(){}

    void clearCards(){ this->cards.Clear(); }

	void writeCardsToPluto(nlohmann::json &pu) {}

    /*bool operator > (CompareRegion* region);

    bool operator < (CompareRegion* region);
    */
public:
    CPlayerCard cards;
};

//玩家单个下注区域
class BetRegion :public CompareRegion
{
public:
    BetRegion() :CompareRegion(){ this->id = 0; }

    void initWithId(int id)
    {
        this->id = id;
        this->betSum = 0;
        this->robotBetSum = 0;
        this->historyVec.clear();
    }

    void AddBetNum(int num)
    {
        if (num > 0)
            this->betSum += num;
    }

    void SubBetNum(int num)
    {
        if (num > 0)
            this->betSum -= num;
    }

    void AddRobotBetNum(int num)
    {
        if (num > 0)
            this->robotBetSum += num;
    }

    void clearBets()
    {
        this->betSum = 0;
        this->robotBetSum = 0;
    }

    int64_t GetBetSum(){ return this->betSum; }

    int64_t GetRobotBetSum(){ return this->robotBetSum; }
public:
    int id;
    deque<int> historyVec;
private:
    int64_t betSum;
    int64_t robotBetSum;
};

class BetRegions
{
public:
    BetRegions()
    {
        for (int i = 1; i <= 2; i++)
        {
            cardCroup.push_back(make_shared<CPlayerCard>(i));
        }
    }

    ~BetRegions()
    {
        //CLEAR_POINTER_MAP(this->regions);
    }

    void init()
    {
        for (int i = 0; i <= 8; i++)
        {
            BetRegion * region = new BetRegion();
            region->initWithId(i);
            this->regions.insert(make_pair(i, region));
        }
    }

    void writeBetSumToPluto(nlohmann::json &pu)
    {
		try
		{
			nlohmann::json jsonArray = nlohmann::json::array();
			for (auto kv : this->regions)
			{
				nlohmann::json jsonItem;
				jsonItem["id"] = kv.second->id;
				jsonItem["betSum"] = kv.second->GetBetSum() + kv.second->GetRobotBetSum();

				jsonArray.push_back(jsonItem);
			}

			nlohmann::json jsonTempObj = nlohmann::json::object({ {"betRegions",jsonArray} });

			pu.insert(jsonTempObj.begin(), jsonTempObj.end());
		}
		catch (const nlohmann::detail::exception & ex)
		{
			NFASSERT(0, ex.what(), __FILE__, __FUNCTION__);
		}
    }

    void WritePlayerCardToPluto(nlohmann::json& j)
    {
		nlohmann::json jsonArray;

		for (auto playerCard : cardCroup)
		{
			nlohmann::json tempObj;

			tempObj["card_id"] = playerCard->getSelfID();
			if (playerCard->getCardCount() == 3)
			{
				string str = std::to_string(playerCard->getCardList()[1])
					+ ","
					+ std::to_string(playerCard->getCardList()[2]);
				tempObj["cards"] = str;
			}
			else
			{
				string str = std::to_string(playerCard->getCardList()[1])
					+ ","
					+ "-1";
				tempObj["cards"] = str;
			}
			int temp = playerCard->GetCardsTotalValue();
			tempObj["card_points"] = temp;

			jsonArray.push_back(tempObj);
		}

		nlohmann::json arrayObject = nlohmann::json::object({ "FillCardsInfo",jsonArray });

		j.insert(arrayObject.begin(), arrayObject.end());
    }

    void WriteGameingPlayerCardToPluto(nlohmann::json& share_pu)
    {
        for (auto playerCard : cardCroup)
        {
			nlohmann::json item;
			item["cardID"] = int32_t(playerCard->getSelfID());
			item["cardType"] = playerCard->GetCardsStr();
			share_pu.push_back(item);
        }
    }

    int64_t getBetAllSum()
    {
        int64_t betAllSum = 0;
        for (auto kv : this->regions)
        {
            BetRegion* region = kv.second;
            betAllSum += region->GetBetSum();
        }
        return betAllSum;
    }

    void SubBetItem(int id, int sum)
    {
        if (regions.find(id) != regions.end())
        {
            BetRegion* region = regions[id];
            region->SubBetNum(sum);
        }
    }

    int64_t getRobotBetAllSum()
    {
        int64_t betAllSum = 0;
        for (auto kv : this->regions)
        {
            BetRegion* region = kv.second;
            betAllSum += region->GetRobotBetSum();
        }
        return betAllSum;
    }

    BetRegion* getRegionById(int id)
    {
        map<int, BetRegion*>::iterator iter = this->regions.find(id);
        if (this->regions.end() == iter)
            return NULL;
        return iter->second;
    }

    void calculateValues()
    {
        for (auto kv : this->regions)
        {
            CompareRegion* region = kv.second;
            region->calculateValue();
        }
    }

    void clearCards()
    {
        for (auto kv : this->regions)
        {
            BetRegion* region = kv.second;
            region->clearCards();
        }
    }

    int getRegionSize()
    {
        return (int)this->regions.size();
    }

    bool isCardTypeEmpty()
    {
        return  (cardCroup[0]->getCardCount() == 0 && cardCroup[1]->getCardCount() == 0);
    }

    map<int, BetRegion*>& GetRegions()
    {
        return regions;
    }

    int64_t GetRegionAllBetById(int betId)
    {
        if (betId < 0)
            return 0;
        return regions[betId]->GetBetSum();
    }

    NF_SHARE_PTR<CPlayerCard> GetCardsOfCard_ID(CardID card_id)
    {
        for (auto temp : cardCroup)
        {
            if (card_id == temp->getSelfID())
                return temp;
        }
        return nullptr;
    }

    void clear()
    {
        for (auto kv : this->regions)
        {
            BetRegion* region = kv.second;
            region->clearCards();
            region->clearBets();
        }

        for (auto temp : cardCroup)
        {
            temp->Clear();
        }
    }
public:
    int tmp_bankercardvalues;
    vector<vector<int>>tmp_m_id_num;
    vector<NF_SHARE_PTR<CPlayerCard>> cardCroup;
    //CPlayerCard dealerCards;            //庄区的牌
    //CPlayerCard notDealerCards;         //闲区的牌
    int dealCardIndex;                  //发牌位置
private:
    map<int, BetRegion*> regions;

};

#endif