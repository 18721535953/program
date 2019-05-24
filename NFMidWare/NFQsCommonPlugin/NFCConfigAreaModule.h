
#ifndef NFC_CONFIG_AREA_MODULE_H
#define NFC_CONFIG_AREA_MODULE_H

#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFIClassModule.h"
#include "NFComm/NFPluginModule/NFIElementModule.h"
#include "NFComm/NFPluginModule/NFIKernelModule.h"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "../../NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "NFComm/NFMessageDefine/NFProtocolDefine.hpp"
#include "NFComm/NFCore/NFUtil.hpp"


struct TMinMax
{
	int Min;
	int Max;

	inline int GetRandom()
	{
		return Min + rand() % (Max - Min);
	}
};
// 游戏豆区间可选底分
struct SConfigBeanRangeItem
{
public:
	int64_t bean;                           // 游戏豆分割点
	int defaultScore;                       // 客户端默认选择多少底分
	string cfgStr;                          // 原始配置字符串

	SConfigBeanRangeItem();
	~SConfigBeanRangeItem()
	{
		Clear();
	}

	inline void Clear()
	{
		m_scoreMap.clear();
	}

	inline void GetMaxBaseScore(int maxScore, int& retScore) {
		for (map<int, int>::iterator it = m_scoreMap.begin(); it != m_scoreMap.end(); ++it)
		{
			int item = it->second;
			if (item > retScore && item <= maxScore)
				retScore = item;
		}
	}

	inline void AddScore(int score)
	{
		m_scoreMap[score] = score;
	}

	inline bool ScoreExists(int score)
	{
		return m_scoreMap.end() != m_scoreMap.find(score);
	}

private:
	map<int, int> m_scoreMap;               // 该区间可选哪些底分【小于等于该豆时可以选哪些底分】
};

struct SConfigBeanRangeList
{
public:
	SConfigBeanRangeList();
	~SConfigBeanRangeList()
	{
		Clear();
	}
	void Clear()
	{
		CLEAR_POINTER_CONTAINER(m_vector);
	}

	void AddItem(SConfigBeanRangeItem* pItem)
	{
		m_vector.push_back(pItem);
	}
	bool ScoreValid(const int64_t userBean, int& selScore) // score=-1 返回默认值
	{
		for (vector<SConfigBeanRangeItem*>::iterator it = m_vector.begin(); it != m_vector.end(); ++it)
		{
			SConfigBeanRangeItem* pItem = *it;
			if (userBean <= pItem->bean)
			{
				if (selScore < 0)
					selScore = pItem->defaultScore;

				return pItem->ScoreExists(selScore);
			}
		}

		return false;
	}
	int GetMaxBaseScore(const int64_t bean, int maxScore)
	{
		int retScore = -1;
		for (vector<SConfigBeanRangeItem*>::iterator it = m_vector.begin(); it != m_vector.end(); ++it)
		{
			SConfigBeanRangeItem* pItem = *it;
			if (bean <= pItem->bean)
			{
				pItem->GetMaxBaseScore(maxScore, retScore);
			}
		}

		return retScore;
	}
	void WriteToPluto(CPluto& pu)
	{
		// TODO
		//string str("");
		//char buffer[100];
		//bool isFirst = true;
		//for (vector<SConfigBeanRangeItem*>::iterator it = m_vector.begin(); it != m_vector.end(); ++it)
		//{
		//    SConfigBeanRangeItem* pItem = *it;
		//    if (isFirst)
		//    {
		//        snprintf(buffer, sizeof(buffer), "%s", pItem->cfgStr.c_str());
		//        isFirst = false;
		//    }
		//    else
		//    {
		//        snprintf(buffer, sizeof(buffer), "|%s", pItem->cfgStr.c_str());
		//    }
		//    str += buffer;
		//}

		//pu << str;
	}
	inline uint16_t Count()
	{
		return (uint16_t)m_vector.size();
	}
public:
	vector<SConfigBeanRangeItem*> m_vector;
};

// 底分、收费、机器人游戏豆随机范围配置
struct SConfigScoreItem
{
	int maxTotalBet;                    // 封顶
	int minBean;                        // 进入条件
	int pinBet;                         // 血拼数值
	int baseBet;                        // 底
	int index;                          // 房间类型编号
	vector<int> raiseBet;               // 加注列表
	TMinMax robotBean;                  // 机器人游戏豆范围
	int roundFee;
	void Clear(){}
	void CopyFrom(SConfigScoreItem& src){}
	//todo
	void inline WriteToPluto(nlohmann::json& p)
	{
		//p << maxTotalBet << pinBet << baseBet;

		uint16_t len = (uint16_t)raiseBet.size();
		//p << len;
		//for (uint16_t i = 0; i < len; ++i)
		//	p << raiseBet[i];

		nlohmann::json jsonStruct;

		jsonStruct["maxTotalBet"] = maxTotalBet;
		jsonStruct["pinBet"] = pinBet;
		jsonStruct["baseBet"] = baseBet;
	
		std::string str = "";
		for (int i = 0; i < len; ++i)
		{
			str += std::to_string(raiseBet[i]);
			//if ((i + 1) < m_cardCount)
			str += ",";
		}
		const char* c_s = str.c_str();
		jsonStruct["raiseBet"] = str;

	}
};

class SConfigScoreList : public NFIConfigScoreListModule
{
public:
	SConfigScoreList();
	~SConfigScoreList();
	virtual	SConfigScoreItem* FindItem(int maxTotalBet);
	void Clear();

	void AddItem(SConfigScoreItem* pItem);

	bool RobotScoreValid(const int64_t robotBean, int selScore); // 豆变化是否踢出robot

	inline uint16_t Count()
	{
		return (uint16_t)m_map.size();
	}
private:
	map<int, SConfigScoreItem*> m_map;
};



enum TOperaType {
	eotSeeCard = 0,
	eotFollow,
	eotAddBet,
	eotVsCard,
	eotFoldCard,
	eotOpenCard,
	eotLen,
};


typedef vector<int> TRateItem;
//游戏区域配置
class NFCConfigAreaModule
	:public NFIConfigAreaModule
{
public:
	NFCConfigAreaModule(NFIPluginManager* p);

	virtual ~NFCConfigAreaModule();

	virtual bool Awake() override;

	virtual bool Init() override;

	virtual bool AfterInit() override;

	virtual bool Execute() override;
	///////////////////////////////////////////////////

	virtual void InitCfg(NFGamespace::QS_GameServer_Type gameType) override;

	virtual bool IsInited() override;

	virtual void SetInited() override;

	virtual void ReadFromVOBJ(const nlohmann::json& j, int& index) override;

	virtual int GetTMinMaxRandom(const TMinMax* t_min_max) const override;

	virtual void writeBetNumToPluto(nlohmann::json& j) override;
	////////////////////// SConfigSocre start //////////////////////
	virtual void ClearSConfigScoreMap();

	virtual void AddSConfigScoreItem(SConfigScoreItem* pItem);

	virtual SConfigScoreItem* FindSConfigScoreItem(const int score);

	virtual bool RobotScoreValid(const int64_t robotBean, const int selScore);// 豆变化是否踢出robot

	virtual int GetRoundFee(int score);

	virtual void SConfigScoreWriteToPluto(CPluto* pu);

	virtual uint16_t SConfigScoreCount() const;
	///////////////////// SConfigSocre end //////////////////////////

	///////////////////// SConfigBean start //////////////////////////
	virtual void ClearSConfigBeanRange();

	virtual void AddSConfigBeanRangeItem(SConfigBeanRangeItem* pItem);

	virtual bool SConfigBeanRangeScoreValid(const int64_t userBean, int& selScore);

	virtual int SConfigBeanGetMaxBaseScore(const int64_t bean, const int maxScore);

	virtual void SConfigBeanWriteToPlute(CPluto* pu);

	virtual uint16_t GetSConfigBeanCount() const;
	///////////////////// SConfigBean end ////////////////////////////

	///////////////////// SConfigBeanItem Start/ //////////////////////
	virtual void ClearSConfigBeanRangeItem();

	virtual void AddScoreSConfigBeanRangeItem(int score);

	virtual void SConfigBeanItemGetMaxBaseScore(int score, int& retScore);

	virtual bool SConfigBeanItemScoreExists(int score);
	//////////////////// SConfigBeanItem end ///////////////////////////
public:

	void StartRefreshConfig();

	int OnMsGidRefreshConfig(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject);

private:
	void checkBindMachine();
	void Perm(int start, int end, int a[]);


private:
	bool inited;
	//CCalcTimeTick m_refreshConfigTime;                      //定时刷新后台配置
	map<int, SConfigScoreItem*> SConfigScoreMap;
	vector<SConfigBeanRangeItem*> SConfigBeanRangeVector;
	map<int, int> ScoreItemMap;               // 该区间可选哪些底分【小于等于该豆时可以选哪些底分】

	NFTimer m_LastTickTimer;

private:
	//NFIWorldGameAreaModule * m_pWorldGameAreaModule;
	NFIGameServerNet_ServerModule * m_pGameServerNet_ServerModule;
	NFIKernelModule * m_pKernelModule;
	NFIElementModule * m_pElementModule;
	NFIClassModule* m_pClassModule;
	NFILogModule * m_pLogModule;
public:
	virtual int GetChooseOpera(bool AIsWin, int AInt, int ExtraSeeCardRate, int ExtraFollowRate, int ExtraAddBetRate, int ExtraVsCardRate, int ExtraFoldCardRate);
	virtual int32_t GetRobotFillMs()
	{
		return  NFUtil::GetRandomRange(robot_cfg.fillrobot_min_sec*1000, robot_cfg.fillrobot_max_sec*1000);
	}
	//SConfigScoreList * score_list;
	//SConfigRobot robot_cfg;
	void InitCfgZjh(NFGamespace::QS_GameServer_Type gameType);
};

#endif
