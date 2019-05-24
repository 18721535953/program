#ifndef __TYPE__MOGO__HEAD__
#define __TYPE__MOGO__HEAD__

#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/type_mogo_def.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
enum EUserState
{
    EUS_NONE = 0,
    EUS_AUTHED = 1,
    EUS_YAOYAO = 2,
    EUS_INTABLE = 3,
};
//     
// // 用户基本信息
struct SUserBaseInfo
{
	int userId;
	int userType;
	int channelId;
	int64_t score;
	int64_t bean;
	string userName;
	string nickName;
	int sex;
	int level;
	int faceId;
	string faceUrl;
	int64_t specialGold;
	int gameRoomLockStatus;         // 用户加锁状态: 为1表示用户加锁状态
	int lastLockGameRoomId;         //最后被锁场编号
	int64_t totalBetCount;
	int64_t winCount;

	SUserBaseInfo();
	void Clear();
	void CopyFrom(NFGamespace::DB_UserBaseInfo& src);
	void WriteToPluto(nlohmann::json& p);
	//void ReadFromJson(cJSON* pJsObj);
	void ReadFromVObj(T_VECTOR_OBJECT& o, int& index);
};

// 变化信息
struct SUserActiveInfo
{
    int fd;
    int userState;
    int selScore;
    int tableHandle;
    int chairIndex;
    int whereFrom;
    string mac;
    int lastSitTableHandle;                     // for yaoyao change table, yaoyao leave write
    float64_t jingDu;
    float64_t weiDu;
    uint32_t yaoyaoTick;
    bool isAddRobot;
    uint32_t addRobotMs;
    uint32_t robotReadyMs;
    uint32_t enterTableTick;                    // 进入桌子Ms， 结束一局就是结束时刻了
	int32_t score;                             // 玩家這一句的輸贏
	int32_t allBet;                            // 所有投注
	int32_t validBet;                          // 有效投注

	string ip;
    SUserActiveInfo();
    void CopyFrom(SUserActiveInfo& src);
    void Clear();
};

// card用户信息
struct SUserInfo
{
	NFGUID self;
    SUserBaseInfo baseInfo;
    SUserActiveInfo activeInfo;
    uint32_t commitTaskId;                              // 提交给db的taskId，用来验证返回
    int tmpInt;                                         // 临时int变量

	SUserInfo() {}
    void CopyFrom(SUserInfo& src)
	{
		baseInfo.userId = src.baseInfo.userId;
		baseInfo.bean = src.baseInfo.bean;
		baseInfo.channelId = src.baseInfo.channelId;
		baseInfo.faceId = src.baseInfo.faceId;
		baseInfo.level = src.baseInfo.level;
	}
    void Clear(){}
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
    string expands;

    SCisScoreReportRetItem();
    void WriteToPluto(CPluto& p);
   // void ReadFromJson(cJSON* pJsObj);
    void ReadFromVObj(T_VECTOR_OBJECT& o, int& index);
};

struct SCisSpecialGoldComsumeRetItem
{
    int userId;
    int64_t specialGold;

    SCisSpecialGoldComsumeRetItem();
    void WriteToPluto(CPluto& p);
   // void ReadFromJson(cJSON* pJsObj);
    void ReadFromVObj(T_VECTOR_OBJECT& o, int& index);
};

extern uint32_t g_taskIdAlloctor;


#endif
