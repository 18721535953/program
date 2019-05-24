#ifndef NF_TABLE_UESR_HPP
#define NF_TABLE_UESR_HPP

#include "Dependencies/json/json.hpp"
#include "Dependencies/common/lexical_cast.hpp"
#include "NFComm/NFPluginModule/NFGUID.h"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFComm/NFPluginModule/NFITableUserModule.h"
#include "CPlayerCards.hpp"
#include "BetArea.hpp"
#include <cstdint>
#include <vector>


enum EUserState
{
	EUS_NONE = 0,
	EUS_AUTHED = 1,
	EUS_YAOYAO = 2,
	EUS_INTABLE = 3,
};

// 用户基本信息
struct SUserBaseInfo
{
	int userId;
	int userType;
	int channelId;
	int64_t score;
	int64_t bean;
	std::string userName;
	std::string nickName;
	int sex;
	int level;
	int faceId;
	std::string faceUrl;
	int64_t specialGold;
	int gameRoomLockStatus;         // 用户加锁状态: 为1表示用户加锁状态
	int lastLockGameRoomId;         //最后被锁场编号
	int64_t totalBetCount;
	int64_t winCount;

	SUserBaseInfo() {}
	inline void Clear() {}
	inline void CopyFrom(NFGamespace::DB_UserBaseInfo& src)
	{
		userId = src.userId;
		userType = src.userType;
		score = lexical_cast<int64_t>(src.score);
		bean = lexical_cast<int64_t>(src.bean);
		userName = src.userName;
		nickName = src.nickName;
		sex = src.sex;
		level = src.level;
		faceId = src.faceId;
		faceUrl = src.faceUrl;
		gameRoomLockStatus = src.gameRoomLockStatus;
		lastLockGameRoomId = src.lastLockGameRoomId;
		totalBetCount = src.totalBetCount;
		winCount = src.winCount;
		channelId = src.channelId;
	}
	inline void WriteToPluto(nlohmann::json& p) 
	{
		try
		{
			nlohmann::json jsonStruct;

			jsonStruct["userId"] = userId;
			jsonStruct["userType"] = userType;
			jsonStruct["score"] = score;
			jsonStruct["bean"] = bean;
			jsonStruct["userName"] = userName;
			jsonStruct["nickName"] = nickName;
			jsonStruct["sex"] = sex;
			jsonStruct["level"] = level;
			jsonStruct["faceId"] = faceId;
			jsonStruct["faceUrl"] = faceUrl;
			jsonStruct["gameRoomLockStatus"] = gameRoomLockStatus;
			jsonStruct["lastLockGameRoomId"] = lastLockGameRoomId;
			jsonStruct["totalBetCount"] = totalBetCount;
			jsonStruct["winCount"] = winCount;

			nlohmann::json jsonStructObject = nlohmann::json::object({ {"baseInfo",jsonStruct} });

			p.insert(jsonStructObject.begin(), jsonStructObject.end());
		}
		catch (const nlohmann::detail::exception& ex)
		{
			NFASSERT(0, ex.what(), __FILE__, __FUNCTION__);
		}
		
	}
	inline void ReadFromJson(nlohmann::json& o)
	{

	}
	inline void ReadFromVObj(nlohmann::json& o, int& index) 
	{
		userId = o[index++].get<int32_t>();
		userType = o[index++].get<int32_t>();
		std::string buffscore;
		buffscore = o[index++].get<std::string>();
		score = stoll(buffscore);
		std::string buffBean;
		buffBean = o[index++].get<std::string>();
		bean = stoll(buffBean);
		userName = o[index++].get<std::string>();
		nickName = o[index++].get<std::string>();
		sex = o[index++].get<int32_t>();
		level = o[index++].get<int32_t>();
		faceId = o[index++].get<int32_t>();
		faceUrl = o[index++].get<std::string>();
		std::string gold = o[index++].get<std::string>();
		specialGold = stoll(gold);
		gameRoomLockStatus = o[index++].get<int32_t>();
		lastLockGameRoomId = o[index++].get<int32_t>();
		channelId = o[index++].get<int32_t>();
	}
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
	std::string mac;
	int lastSitTableHandle;                     // for yaoyao change table, yaoyao leave write
	float jingDu;
	float weiDu;
	uint32_t yaoyaoTick;
	bool isAddRobot;
	uint32_t addRobotMs;
	uint32_t robotReadyMs;
	uint32_t enterTableTick;                    // 进入桌子Ms， 结束一局就是结束时刻了
	int32_t score;                             // 玩家這一句的輸贏
	int32_t allBet;                            // 所有投注
	int32_t validBet;                          // 有效投注

	std::string ip;
	SUserActiveInfo() {}
	void CopyFrom(SUserActiveInfo& src) {}
	void Clear() {}
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
	void CopyFrom(SUserInfo& src) {}
	void Clear() {}
};

class NFTableUser :public NFITableUserModule
{
public:
    NFTableUser(){ Clear(); }
    ~NFTableUser(){}

    void Clear()
    {
        ustate = tusNone;
        isReady = 0;
        isTrust = false;
        isRobot = false;
        isAbnormal = false;
        readyOrLeaveTick = NFUtil::GetTimeStampInt64Ms();
        userInfo.Clear();
        player.Clear();
        isDeclare = -1;
        discardCount = 0;
        lastDiscardTick = 0;

        betSum = 0;
        noOperationNum = 0;
        ClearBetItems();
        roundIncBean = 0;
        roundRevenue = 0;
        betItems.clear();
    }

    void ClearBetItems()
    {
        //CLEAR_POINTER_CONTAINER(betItems);
        vector<NF_SHARE_PTR<BetItem>>().swap(betItems);
        betSum = 0;
        betItems.clear();
    }

    void AddBet(int betNum, int betCategoryCode)
    {
        for (auto& item : this->betItems)
        {
            if (item->betId == betCategoryCode)
            {
                item->betNum += betNum;
                this->betSum += betNum;
                return;
            }
        }

        NF_SHARE_PTR<BetItem> item = make_shared<BetItem>();
        item->betNum = betNum;
        item->betId = betCategoryCode;
        this->betSum += betNum;
        this->betItems.push_back(item);
    }

    void WriteBetItemsToPluto(nlohmann::json& j)
    {
		try
		{
			nlohmann::json jsonArray;

			uint16_t len = (uint16_t)this->betItems.size();
			for (int i = 0; i < len; ++i)
			{
				nlohmann::json item;
				item["id"] = this->betItems[i]->betId;
				item["betSum"] = this->betItems[i]->betNum;
				jsonArray.push_back(item);
			}

			nlohmann::json jsonObject = nlohmann::json::object({ {"selfBets",jsonArray} });

			j.insert(jsonObject.begin(), jsonObject.end());
		}
		catch (const nlohmann::detail::exception & ex)
		{
			NFASSERT(0, ex.what(), __FILE__, __FUNCTION__);
		}
		
    }

    int GetBetAllSum()
    {
        int tempSum = 0;
        for (size_t i = 0; i < this->betItems.size(); ++i)
        {
            tempSum += this->betItems[i]->betNum;
        }
        return tempSum;
    }

    int32_t GetBetSumById(size_t id)
    {
        if (this->betItems.size() <= 0) return 0;

        auto iter = this->betItems.begin();
        for (; iter != this->betItems.end();iter++)
        {
            BetItem * item = iter->get();
            if (item->betId == (int)id)
                return item->betNum;
        }
        return 0;
    }

    //初始化玩家不操作记录数
    void initNoOperationNum() { this->noOperationNum = 0; }

    void addNoOperationNum(){ this->noOperationNum++; }

    void addNoOperationNum(int count) { this->noOperationNum = count; }

    void AddUserInfoBean(int64_t value){ userInfo.baseInfo.bean += value; }

    void SetUserInfoBean(int64_t value){ userInfo.baseInfo.bean = value; }

    void SetUserInfoName(std::string value){ userInfo.baseInfo.userName = value; }

    void CopyUserInfoFrom(SUserInfo& value){ userInfo.CopyFrom(value); }

    void SetUserInfoTotalBetCount(int64_t value){ userInfo.baseInfo.totalBetCount = value; }

    void SetUserInfoWinCount(int64_t value){ userInfo.baseInfo.winCount = value; }

    void SetEnterTableTick(uint32_t value){ userInfo.activeInfo.enterTableTick = value; }

    void SetActiveScore(int32_t value){ userInfo.activeInfo.score = value; }

    void ClearPlayerUserinfo(){ player.RoundClear(); }

    /////////////////////// Get Set ///////////////////
    
    void SetActiveAllBet(int32_t value){ userInfo.activeInfo.allBet = value; }

    int32_t GetActiveAllBet(){ return userInfo.activeInfo.allBet; }

    void SetActiveValidBet(int32_t value){ userInfo.activeInfo.validBet = value; }

    int32_t GetActiveValidBet(){ return userInfo.activeInfo.validBet; }

    int GetUsTate() const override { return ustate; }

    void SetUsState(int value){ this->ustate = value; }

    int GetIsReady() const { return isReady; }

    void SetIsReady(int value){ this->isReady = value; }

    int GetIsTrust() const { return isTrust; }

    void SetIsTrust(int value){ this->isTrust = value; }

    bool GetIsRobot() const override { return isRobot; }

    void SetIsRobot(bool value){ this->isRobot = value; }

    uint32_t GetReadyOrLeaveTick() const { return readyOrLeaveTick; }

    void SetReadyOrLeaveTick(uint32_t value){ this->readyOrLeaveTick = value; }

    uint32_t GetFillOrLeaveRobotMs() const { return fillOrLeaveRobotMs; }

    void SetFillOrLeaveRobotMs(uint32_t value){ this->fillOrLeaveRobotMs = value; }

    SUserInfo& GetSUserInfo() override { return userInfo; }

    void SetUserInfo(SUserInfo value){ this->userInfo = value; }

    const CPlayerUserInfo& GetCPlayerUserInfo() const { return player; }

    void SetPlayerUserInfo(CPlayerUserInfo value){ this->player = value; }

    int GetIsDeclare() const { return isDeclare; }

    void SetIsDealare(int value){ this->isDeclare = value; }

    uint32_t GetOfflineTick() const { return offlineTick; }

    void SetOfflineTick(uint32_t value){ this->offlineTick = value; }

    int GetDiscardCount() const { return discardCount; }

    void SetDisCardCount(int value){ this->discardCount = value; }

    uint32_t GetLastDisCardTick() const { return lastDiscardTick; }

    void SetLastDisCardTick(uint32_t value){ this->lastDiscardTick = value; }

    int64_t GetEnterBean() const { return enterBean; }

    void SetEnterBean(int64_t value){ this->enterBean = value; }

    int64_t GetLeaveBean() const { return this->leaveBean; }

    void SetLeaveBean(int64_t value){ this->leaveBean = value; }

    uint64_t GetEnterTime() const { return enterTime; }

    void SetEnterTime(uint64_t value){ this->enterTime = value; }

    uint64_t  GetLeaveTime() const { return leaveBean; }

    void SetLeaveTime(uint64_t value){ this->leaveTime = value; }

    std::vector<NF_SHARE_PTR<BetItem>>& GetBetVector(){ return betItems; }

    int GetBetSum() const { return betSum; }

    int64_t GetRoundIncBean() const { return roundIncBean; }

    void SetRoundIncBean(int64_t value){ this->roundIncBean = value; }

    int64_t GetRoundRevenue() const { return roundRevenue; }

    void SetRoundRevenun(int64_t value){ this->roundRevenue = value; }

    int GetNoOperationNum() const { return noOperationNum; }

    bool GetIsAbnormal() const { return isAbnormal; }

    void SetIsAbnormal(bool value){ this->isAbnormal = value; }
private:
    int                         ustate;
    int                         isReady;                    // 定义成int为了方便发包
    int                         isTrust;
    bool                        isRobot;
    uint32_t                    readyOrLeaveTick;
    uint32_t                    fillOrLeaveRobotMs;
    SUserInfo                   userInfo;
    CPlayerUserInfo             player;
    int                         isDeclare;                  // server only 是否叫了，-1表示没叫过，0表示不叫，1表示叫
    uint32_t                    offlineTick;           // server only
    int                         discardCount;               // server only 过牌几次牌
    uint32_t                    lastDiscardTick;       // server only 上次出牌的时间
    int64_t                     enterBean;              // 进入桌子时的金币
    int64_t                     leaveBean;              // 退出桌子时的金币
    uint64_t                    enterTime;             // 进入桌子时的时间戳
    uint64_t                    leaveTime;             // 退出桌子时的时间戳
    std::vector<NF_SHARE_PTR<BetItem>>       betItems;
    int                         betSum;                     //押注总额
    int64_t                     roundIncBean;
    int64_t                     roundRevenue;
    int                         noOperationNum;             //没有操作的次数（当到达一定次数后踢出玩家)
    bool                        isAbnormal;                // 账号异常
};

class SArrangeTableItem
{
public:
    SArrangeTableItem() : tableHandle(-1), selScore(-1), curUser(0), canSitMaxUser(0), arrangeLen(0)
    {
        memset(arrangeUserAry, 0, sizeof(arrangeUserAry));
    }

    int tableHandle;                                        //桌子的标识
    int selScore;                                           //底分？
    int curUser;                                            //当前人数
    int canSitMaxUser;                                      // 最多能进入多少人
    int arrangeLen;                                         // 等待加入桌子的人数 也可以看成是arrangeUserAry的下标
    NF_SHARE_PTR<SUserInfo> arrangeUserAry[MAX_TABLE_USER_COUNT];        //当前桌子的用户信息
};

class SArrangeUserList
{
public:
    SUserInfo* UserAry[MAX_TABLE_HANDLE_COUNT];
    int len;
    int selScore;

    SArrangeUserList(){ Clear(); }
    void Clear()
    {
        len = 0;
        selScore = -1;
    }
    int SortIfTimeout(uint32_t msTime)         // 返回超时人数数量，0表示没人超时
    {
        int Result = 0;
		uint32_t curMsTick = NFUtil::GetTimeStampInt64Ms();
        for (int i = 0; i < len; ++i)
        {
            SUserInfo* pUser = UserAry[i];
            if (pUser->activeInfo.yaoyaoTick - curMsTick >= msTime)
            {
                pUser->tmpInt = 1;      // 超时为1
                ++Result;
            }
            else
            {
                pUser->tmpInt = 0;
            }
        }
        // 只有1和0的数组排序, 1排到前面
        if (Result > 0)
        {
            for (int i = 0; i < len - 1; ++i)
            {
                if (UserAry[i]->tmpInt != 1)
                {
                    int maxIndex = i;
                    for (int j = i + 1; j < len; ++j)
                    {
                        if (1 == UserAry[j]->tmpInt)
                        {
                            maxIndex = j;
                            break;
                        }
                    }
                    if (maxIndex != i)
                    {
                        UserAry[i]->tmpInt = 1;
                        UserAry[maxIndex]->tmpInt = 0;
                    }
                    else
                    {
                        // 没有为1的元素了
                        break;
                    }
                }
            }
        }

        return Result;
    }
};

#endif