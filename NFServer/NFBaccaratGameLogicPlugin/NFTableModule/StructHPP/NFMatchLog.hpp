#ifndef MATCH_LOG_HPP
#define MATCH_LOG_HPP

#include <string>
#include <cstdint>
#include <map>
#include <vector>

struct SInfoLogItem
{
    int             userId;
    int             channelId;
    int             chairId;
    int64_t         startBean;
    int64_t         endBean;
    std::string          startCards;
    std::string          endCards;
    std::string          username;
    std::string          nickname;
    int32_t                         score;
    int32_t                         allBet;
    int32_t                         validBet;
    SInfoLogItem() : userId(-1), chairId(-1), startBean(0), endBean(0), username(""), nickname(""){}
};

struct SActionLogItem
{
    int             userId;
    int64_t         betBean;
    int             betArea;
    int             timestamp;
    std::string          cardType;
    std::string          cards;


    SActionLogItem() : userId(-1), betBean(0), betArea(-1), cardType("[]"), cards("[]"){}
};
struct SMatchLog
{
    uint64_t                        matchId;
    uint32_t                        userCount;
    uint64_t                        startTime;
    uint64_t                        endTime;
    uint32_t                        handle;
    uint32_t                        selScore;
    uint32_t                        tax;
    std::string                          channelId;
    std::string                          bankerCards;
    std::string                          notBankerCards;
    std::string                          resultForArea;
    std::string                          resultForAll;
    //int32_t                         score;
    //int32_t                         allBet;
    //int32_t                         validBet;

    std::map<int, NF_SHARE_PTR<SInfoLogItem>>         infoLog;
    std::vector<NF_SHARE_PTR<SActionLogItem>>     actionLogVec;
    std::map<int, std::vector<NF_SHARE_PTR<SActionLogItem>>>        actionLog;
    //vector<SActionLogItem*>         actionLog;

    inline void RemoveLogByUserId(int userId)
    {
        RemoveSActionLogOfByUserId(userId);
        RemoveInfoLogByUserId(userId);
    }

    inline void RemoveSActionLogOfByUserId(int userId)
    {
        auto iter = actionLog.find(userId);
        if (iter != actionLog.end())
            actionLog.erase(iter);
    }

    inline void RemoveInfoLogByUserId(int userId)
    {
        auto iter = infoLog.find(userId);
        if (iter != infoLog.end())
            infoLog.erase(iter);
    }

    SMatchLog() : matchId(0), userCount(0), startTime(0), endTime(0), handle(0), selScore(0), tax(0), channelId(""), infoLog(), actionLog(), actionLogVec(){}
    inline void Clear()
    {
        matchId = 0;
        userCount = 0;
        startTime = 0;
        endTime = 0;
        handle = 0;
        selScore = 0;
        tax = 0;
        channelId = "";
        bankerCards = "";
        notBankerCards = "";
        infoLog.clear();
        actionLogVec.clear();
        actionLog.clear();
    }
    inline void WriteToPluto(nlohmann::json& p)
    {
        //p << matchId << userCount << startTime << endTime << handle << selScore << tax << channelId << bankerCards << notBankerCards << resultForArea << resultForAll;

        ////infoLog
        //uint16_t len = (uint16_t)infoLog.size();
        //p << len;
        //for (map<int, NF_SHARE_PTR<SInfoLogItem>>::iterator it = infoLog.begin(); it != infoLog.end(); it++)
        //{
        //    NF_SHARE_PTR<SInfoLogItem> item = it->second;
        //    p << item->userId << item->channelId << item->chairId << item->startBean << item->endBean << item->startCards << item->endCards << item->username << item->nickname << item->score << item->allBet << item->validBet;
        //}

        ////actionLog
        //len = (uint16_t)actionLog.size();

        //p << len;

        //for (auto itemMap : actionLog)
        //{
        //    std::vector<NF_SHARE_PTR<SActionLogItem>> actionLogVec = itemMap.second;
        //    std::string tempStr = "";
        //    for (size_t i = 0; i < actionLogVec.size(); i++)
        //    {
        //        NF_SHARE_PTR<SActionLogItem> item = actionLogVec[i];
        //        tempStr += std::to_string(item->timestamp) + ":" + std::to_string(item->betArea) + ":" + std::to_string(item->betBean);
        //        if (i + 1 < actionLogVec.size())
        //            tempStr += ",";


        //    }
        //    p << itemMap.first << tempStr;
        //}
    }
};

struct SCisScoreReportRetItem
{
	int userId;
	int64_t score;
	int64_t bean;
	int64_t specialGold;
	int64_t incScore;
	int64_t incBean;
	int experience;
	int level;
	std::string expands;

	SCisScoreReportRetItem(){}
	inline void WriteToPluto(nlohmann::json& p)
	{
		p.push_back(userId);
		p.push_back(bean);
		p.push_back(incBean);
		p.push_back(score);
		p.push_back(experience);
		p.push_back(expands);
	}

	inline void ReadFromJson(nlohmann::json& p)
	{

	}

	inline void ReadFromVObj(nlohmann::json& o, int& index)
	{
		userId = o[index++].get<int32_t>();
		string strscore = o[index++].get<std::string>();
		score = stoll(strscore);

		string strBean = o[index++].get<std::string>();
		bean = stoll(strBean);

		string strGold = o[index++].get<std::string>();
		specialGold = stoll(strGold);

		string strincScore = o[index++].get<std::string>();
		incScore = stoll(strincScore);
		string strincBean = o[index++].get<std::string>();
		incBean = stoll(strincBean);

		experience = o[index++].get<int32_t>();
		level = o[index++].get<int32_t>();
		expands = o[index++].get<std::string>();
	}
};

#endif