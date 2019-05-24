#ifndef NFI_CONFIG_AREA_MODULE_H
#define NFI_CONFIG_AREA_MODULE_H
#include "NFIModule.h"
#include <vector>
#include "../../Dependencies/json/json.hpp"
#include "../../NFMidWare/NFQsCommonPlugin/NFQsCommonConfig.hpp"
class CPluto;
struct TMinMax;
struct SConfigScoreItem;
struct SConfigBeanRangeItem;
struct  SConfigScoreList;
struct SConfigRobot;

class NFIConfigScoreListModule :public NFIModule
{
public:
	virtual	SConfigScoreItem* FindItem(int maxTotalBet) = 0;
	virtual void AddItem(SConfigScoreItem* pItem) = 0;
};
struct SConfigRobot
{
	int ready_min_sec;
	int ready_max_sec;
	int max_waitready_sec;
	int fillrobot_min_sec;
	int fillrobot_max_sec;
	int fill_rate;
	int finish_wait_sec;

	SConfigRobot() : ready_min_sec(0), ready_max_sec(0), max_waitready_sec(0), \
		fillrobot_min_sec(0), fillrobot_max_sec(0), fill_rate(0), finish_wait_sec(0) {}
};
class NFIConfigAreaModule :public NFIModule
{
public:

	virtual void InitCfg(NFGamespace::QS_GameServer_Type gameType) = 0;

    virtual bool IsInited() = 0;

    virtual void SetInited() = 0;

    virtual void ReadFromVOBJ(const nlohmann::json& j, int& index) = 0;

    virtual int GetTMinMaxRandom(const TMinMax * t_min_max) const = 0;

    virtual void writeBetNumToPluto(nlohmann::json& j) = 0;
    /////////////////////////-- SConfigScore Start --////////////////////////
    virtual void ClearSConfigScoreMap() = 0;

    virtual void AddSConfigScoreItem(SConfigScoreItem* pItem) = 0;

    virtual SConfigScoreItem* FindSConfigScoreItem(const int score)  = 0;

    virtual bool RobotScoreValid(const int64_t robotBean, const int selScore) = 0;// 豆变化是否踢出robot

    virtual int GetRoundFee(int score) = 0;

    virtual void SConfigScoreWriteToPluto(CPluto* pu) = 0;

    virtual uint16_t SConfigScoreCount() const = 0;
    ////////////////////////-- SConfigScore end --///////////////////////////

    ////////////////////////-- SConfigBeanRange start --//////////////////////////
    virtual void ClearSConfigBeanRange() = 0;

    virtual void AddSConfigBeanRangeItem(SConfigBeanRangeItem* pItem) = 0;

    virtual bool SConfigBeanRangeScoreValid(const int64_t userBean, int& selScore) = 0;

    virtual int SConfigBeanGetMaxBaseScore(const int64_t bean, const int maxScore) = 0;

    virtual void SConfigBeanWriteToPlute(CPluto* pu) = 0;

    virtual uint16_t GetSConfigBeanCount() const = 0;
    ////////////////////////-- SConfigBeanRange end--/////////////////////////////

    ////////////////////////-- SConfigBeanRange start --//////////////////////////
    virtual void ClearSConfigBeanRangeItem() = 0;

    virtual void AddScoreSConfigBeanRangeItem(int score) = 0;

    virtual void SConfigBeanItemGetMaxBaseScore(int score, int& retScore) = 0;

    virtual bool SConfigBeanItemScoreExists(int score) = 0;
    ////////////////////////-- SConfigBeanRange end--/////////////////////////////

	virtual int32_t GetRobotFillMs() = 0;
	virtual int GetChooseOpera(bool AIsWin, int AInt, int ExtraSeeCardRate, int ExtraFollowRate, int ExtraAddBetRate, int ExtraVsCardRate, int ExtraFoldCardRate) = 0;
public:
    int area_num;
    int masterFsId;
    int min_bean;
    int64_t robot_min_bean;
    int64_t robot_max_bean;
    int max_offline_sec;
    int realuser_max_waitsec;
    std::list<int> near_node;
    std::string area_jingweidu;
    int gameRoomId;
    std::string m_cis_key;
    std::string m_aes_key;
    int revenue;

    std::vector<int> bet_nums;
    int bet_Time;
    int banker_on_minbean;
    int banker_out_minbean;
    int user_banker_model;
    int usersystem_banker_model;
    int serial_banker_num;
    int banker_lost_maxbean;
    int user_bet_maxnum;
    int64_t sys_now_stock;
    int64_t sys_revenue_amount;
    int sys_revenue_scale;
    int64_t sys_max_upcondition;
    //机器人下注
    int m_enableCheat;                      //是否开启 0：不开启 1：开启
    std::vector<int> m_robotBetProportionAry;    //机器人在各区域下注比例
    std::vector<int> m_robotBetChipAry;          //机器人下注使用的筹码
    int m_robotBetMinPercent;               //机器人下注最小百分比
    int m_robotBetMaxPercent;               //机器人下注最大百分比

	//robot
	int startId;
	int endId;

	int64_t	ready_min_sec;
	int64_t ready_max_sec;

	//炸金花数据
public:
	NFIConfigScoreListModule * score_list;

	string bean_name;

	int bet_sec;

	int fillchair_sec;
	int addrobot_sec;
	int addrobot_rate;

	int estimate_ip;

	int pin_bet_round;                  //第几轮可以血拼，轮数不计算底注轮
	int vs_bet_round;                   //第几轮可以vs
	int no_see_round;					//闷牌 最多几收
	string specialGold_name;            //记分局台费的名字
	int allow_other_gaming;

	std::vector<int> WinCfgOperaRate;
	std::vector<int> LoseCfgOperaRate;
	
	int straightFlushScore;
	int sct3OfAKindScore;
	SConfigRobot robot_cfg;
};

#endif
