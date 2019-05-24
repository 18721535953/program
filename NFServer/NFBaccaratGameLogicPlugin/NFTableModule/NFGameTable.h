#pragma once
#ifndef NF_GAME_TABLE_H
#define NF_GAME_TABLE_H
#include <map>
#include <vector>
#include <memory>
#include <deque>

#include "NFComm/NFCore/NFUtil.hpp"
#include "NFComm/NFPluginModule/NFIControlCardModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "NFComm/NFPluginModule/NFPlatform.h"
#include "NFComm/NFPluginModule/NFITableUserModule.h"
#include "StructHPP/BetArea.hpp"
#include "StructHPP/NFMatchLog.hpp"
#include "StructHPP/GameConfig.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"


struct SUserInfo;
class NFITableManagerModule;
class NFTableUser;


class TableConfig
{

public:
    void Clear()
    {
        bet_nums.clear();
        bet_Time = 0;
        banker_on_minbean = 0;
        banker_out_minbean = 0;
        user_banker_model = 0;
        usersystem_banker_model = 0;
        serial_banker_num = 0;
        banker_lost_maxbean = 0;
        user_bet_maxnum = 0;
    }

    void readFromConfig(){}

    inline void writeBetNumsToPluto(CPluto& pu)
    {

    }
public:
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
    int     sys_revenue_scale;
    int64_t sys_max_upcondition;
};

struct RankingUser
{
    std::string userName;
    int64_t bean;
};

class NFGameTable:public NFMachineState
{
public:
	NFGameTable();
    NFGameTable(int32_t handle, NFITableManagerModule * p);

    ~NFGameTable();

    void Init();

    void AfterInit();

    /*
    * 获取游戏用户列表
    */
    std::map<int32_t, NF_SHARE_PTR<NFITableUserModule>>& GetUserList();

    /*
    * 获取桌子的handle
    */
    const int32_t GetHandle() const;

    /*
    * 获取桌子的Tick
    */
    uint32_t GetTablePassTick();

    /*
    * 设置计时器时间
    */
    void SetTimeCount(int32_t value);

    /*
    * 获取计时器剩余的时间
    */
    int GetTimeCount();

    /*
    * 计时器减去指定的时间
    * value : 减去的数值
    */
    void MinusTimeCount(int value);

    /*
    * 添加桌子Tick
    * ms : 添加的tick
    */
    void AddTableMsTick(uint32_t ms);

    /*
    * 获取桌子指定区域的下注总额
    * betId : 下注区域ID
    */
    int64_t GetBeanByRegionId(int betId);

    /*
    * 获取桌子所有区域的下注总额
    */
    int64_t GetAllBetBean();

    /**
       减去下注金额
    */
    void SubBetBeanById(int id, int sum);

    /**
        清除一个桌子用户的下注信息,并从下注总额中减去
    */
    void SubBetBeanOfTableUser(NF_SHARE_PTR<NFTableUser> pTableUser);

    /**
        检查是否有断线的用户
    */
    void CheckOfflineOfTableUser();

    /*
    * 根据id 获取指定下标的牌值
    * id : 庄/闲 的 id
    * index : 牌组的下标
    */
    int GetCardValueByCardID(CardID id, int index);

    /*
    * 获取桌子下注区域
    */
    BetRegions& GetBetRegions();

    /*
    * 桌子聊天处理
    * p : 数据包
    * chairIndex : 用户所在桌子的位置下标
    */
    int ProcClientChat(const nlohmann::json& p, int chairIndex);

    /*
    * 桌子下注处理
    * p : 数据包
    * chairIndex : 用户所在桌子的位置下标
    */
    int ProcClientBet(const nlohmann::json& p, int chairIndex);

    /*
    * 桌子用户下注
    * pTUser : 桌子用户
    * betId  : 下注区域
    * betNum : 下注数值
    */
    void DoUserBet(NF_SHARE_PTR<NFTableUser>& pTUser, int betId, int betNum);

    /*
    * 控牌
    */
    bool ControlCard();
    /*
    * 用户离开桌子
    * userId : 用户ID
    */
    bool LeaveTable(const int userId);

    /*
    * 用户正常离开桌子 ,只可以正常离开，不会引起断线
    * userId : 用户ID
    * mayOffline : 是否下线
    */
    NF_SHARE_PTR<NFTableUser> OnlyNomalLeaveTable(int userId, bool& mayOffline);

    /*
    * 根据id查找用户，入座的用户
    * userId : 用户id
    */
    NF_SHARE_PTR<NFTableUser> FindUserById(int userId);

    /*
    * 桌子是否正在游戏中
    */
    bool IsGaming();

    /*
    * 是否在游戏中途离开
    * pTUser : 桌子用户
    */
    bool CanLeaveWhenGaming(NF_SHARE_PTR<NFITableUserModule> pTUser);

    /*
    * 清理桌子数据
    */
    void NoUserClearData();

    /*
    * 开始游戏前的处理
    */
    bool BeforeStartGame();

    /*
    * 初始化对局日志
    */
    void InitMatchLog();

    /*
    * 随机加入机器人
    */
    void RandomRobotBet();

    /*
    * 每回合清除数据
    */
    void RoundStopClearData();

    /*
    * 桌子位置是否有效
    * chairIndex : 桌子位置下标
    */
    bool IsChairIndexValid(int chairIndex);

    /*
    * 获取空的桌子位置下标
    */
    int GetEmptyChairIndex();

    /*
    * 桌子用户是否有fd
    * pTUser : 桌子用户
    */
    bool HasFd(NF_SHARE_PTR<NFTableUser> pTUser);

    /*
    * 设置控牌数值
    * controlValue : 控制数值
    */
    void SetControlValue(int controlValue);

    /*
    * 获取控制数值
    */
    int GetControlValue() const;

    /*
    * 设置放水值
    */
    void SetScore(uint32_t score);

    /*
    * 获取放水值
    */
    uint32_t GetScore();

    /*
    * 获取桌子指定位置的用户
    * chairIndex : 桌子位置下标
    */
    NF_SHARE_PTR<NFTableUser> FindUserByChairIndex(int chairIndex);

	/*
	* 添加用户到桌子中
	* pUser : 用户
	* chairInex : 桌子位置下标
	* isRobot : 是否是机器人
	*/
	void AddUser(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex, int isRobot);

	/*
	* 发送所有信息给用户
	* toPUser : 接收信息的用户
	* ignoreUser : 忽略的用户
	*/
	void SendAllInfoToUser(std::shared_ptr<SUserInfo> toPUser, std::shared_ptr<SUserInfo> ignoreUser);

    /*
    * 激活桌子状态
    */
    void ActiveTableToGameState();

	/**
	 * \brief 计算区域结果
	 * \param result 保存胜负区域
	 */
	void CalculateResultOfBetArea(map<int, float>& result);

	/**
	 * \brief 从牌组中获取一张牌
	 * \param index 牌的下标
	 * \return 返回index位置的牌
	 */
	int GetCardOfGroup(size_t index);

    /*
    * 用户进入桌子
    * pUser : 进入桌子的用户
    * chairIndex : 进入的桌子位置
    * isRobot : 是否是机器人
    */
    bool EnterTable(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex, bool isRobot);

    /*
    * 清除用户
    * pTUser : 需要清除的桌子用户
    */
    void ClearUser(NFTableUser& pTUser);

    /*
    * 上报积分
    */
    void StartReportScore();

    /*
    * 上报积分 (新函数)
    */
    void ReportSocreLog(uint32_t taskId, uint32_t areaId, int32_t gameRoomId, int32_t isLockGame, string openSeriesNum, string gameStartMsStamp);

    /*
    * 上报对局日志 (新函数)
    */
    void ReportMatchLog(uint32_t taksId, uint32_t areaId, int32_t gameRoomId, int32_t isLockGame);

    /*
    * 上报对局日志 (旧函数)
    */
    void StartReportMatchLog();

    /*
    * 上报积分异常
    * code : 错误码
    * errorMsg : 错误信息
    */
    void OnReportScoreError(int code, const char* errorMsg);

    /*
    * 上报积分成功
    * p : 数据包
    * index : 数据包下标
    */
    void OnReportScoreSuccess(const nlohmann::json& p, int index);

    /*
    * 上报对局日志异常
    * code : 错误码
    * errorMsg : 错误信息
    */
    void OnMatchReportError(int code, const char* errorMsg);

    /*
    * 刷新用户信息
    * baseInfo : 用户基本信息
    * chairIndex : 桌子位置下标
    */
    void ClientUpdateUserInfo(NFGamespace::DB_UserBaseInfo& baseInfo, int chairIndex);

    /*
    * 如果没有真实用户，则清除机器人
    */
    void ClearRobotUserIfNoRealUser();

    /*
    * 获取桌子空位置的下标，如果有机器人，则把机器人挤掉
    */
    int GetEmptyChairAry(int* chairAry);

    // 获取真实玩家数量
    int GetRealUserCount();

    // 获取当前玩家数量
    int GetCurUserCount() const;

    // 更新排行榜
    void UpdateAndSendRankListDate();

    /*
    * 开始游戏通知
    */
    void SendStartGameNotify(int chairIndex);

    /*
    * 发牌
    */
    void DealCard();

    /*
    * 补牌
    */
    void FillCard();

    /*
    * 发送发牌数据给客户端
    */
    void SendDealCardDataToClient();

    /*
    * 发牌
    */
    void DealCardOfCardID(CardID card_id, int value);

    /*
    * 补牌
    */
    void FillCardOfGardID(CardID card_id, int value);

    /*
    * 计算最后结果
    */
    void CalcualteResult();

    /*
    * 计算区域并记录
    */
    void ResultAreaAndRecord();

    /*
    * 发送当前这一局的游戏数据到客户端
    */
    void SendResultDataToClient();

    /*
    * 机器人下注
    */
    void RobotBet();

    /*
    * 随机机器人下注时间
    */
    void RandomRobotBetTime();

    /*
    * 随机机器人下注时间
    */
    int GetRandomRobotBetAreaByProportion();

    /*
    * 随机机器人下注筹码
    */
    int GetRandomRobotBetBean();

    /*
    * 向后台获取游戏控制
    */
    void GetGameControlValue();

    /**
      写入机器人押注信息
    */
    void WriteRobotBetInfoToPluto(nlohmann::json & j);
    //////////////////////////////////////////////////////////////////////

    /*
    * 发送用户状态
    * userId : 用户的id
    * tuserState : 用户的状态
    */
    void SendUserStateNotify(const int userId, const int tuserState);

    /*
    * 发送桌子记录
    * fd : 与用户绑定的socket描述符
    */
    void SendBroadTableRecord(const NFGUID& fd);

    /**
      发送桌子的玩家金币、牌型信息、下注信息
    */
    void SendUserBeanAndCardTypeAndBetBeanOfTable(const int fd);

    /*
    * 发送其他玩家离开房间通知
    * username : 离开的玩家的名字
    */
    void SendBroadToUserOtherLeaveRoom(std::string username);

    /*
    * 发送桌子区域输赢信息通知
    */
    void SendResultAreaToUserAtTheSameTable();

    /*
    * 发送其他玩家进入房间通知
    *  pUser：进入房间的玩家
    */
    void SendBroadToUserOtherEnterRoom(const SUserInfo& pUser);

    /*
    * 指定一个桌子的状态，发送给一个指定的用户
    * toUser：要发送的指定用户
    * tableSatate：桌子的状态
    * ingoreUserState：用户的状态，处在这一状态的用户会跳过，不发包
    */
    void SendTableStateToOneUser(/*const*/ NFTableUser& toUser, TableState tableSatate, int ingoreUserState);

    /*
    * 指定一个桌子的状态，发送给一个指定的用户
    * toUser：要发送的指定用户
    * tableSatate：桌子的状态
    * ingoreUserState：用户的状态，处在这一状态的用户会跳过，不发包
    */
    void SendTableStateToOneUser(const NF_SHARE_PTR<SUserInfo>& toUser, TableState tableSatate, int ingoreUserState);

    /*
    * 指定一个桌子的状态， 发送给同一桌子的所有用户
    * tableState :桌子的状态
    * ingoreUserState : 用户的状态，处在这一状态的用户会跳过,不发包
    */
    void SendTableStateToUserAtTheSameTable(TableState tableSatate, int ingoreUserState);

    /*
    * 向客户端发送错误码和错误信息
    * pTUser: 桌子用户
    * code : 错误码
    * errorMsg : 错误信息
    */
    void SendForceLeaveNotify(NF_SHARE_PTR<NFTableUser>& pTUser, int code, const char* errorMsg);

    //////////////////////////////// // //////////////////////////////////////

    /*
    * 清除桌子记录
    */
    void ClearTableRecord();

    /*
    * 桌子记录数量 +1
    */
    void AddTableRecordCount();

    /*
    * 桌子记录数量 -1
    */
    void SubTableRecordCount();

    /*
    * 获取桌子记录数量
    */
    int GetTableRecordCount();

    /*
    * 写入桌子记录到CPluto
    * pu : 
    */
    void WriteTableRecordToPlute(nlohmann::json & j);
   
    /*
    * 追加桌子记录
    * betId : 押注区域ID
    * count : 追加数量
    */
    void PushBackTableRecord(int betId, int count);
  
    /*
    * 将当前这一句的结果追加到记录中
    */
    void RecordTableRecord();

    /*
    * 随机初始化桌子记录
    */
    void RandomInitTableRecord();

    /*
    * 当前局赢的区域中是否有指定的区域
    * * areaType : 查询的区域
    */
    bool IsHaveResultArea(WagerAreaType areaType);

private:
	void Riffle();
private:
    int                     m_gameCount;                        // 开局次数  用于控制机器人下注概率
    int                     m_handle;                           // 桌子唯一标识
    int                     m_controlValue;                     // 控制变量 1为控制，0为不控制
    uint32_t                m_score;                            // 放水值
    int                     m_getControlTimes;
    int                     m_tstate;                           // 桌子状态
    int                     m_decTimeCount;                     // 倒计时
    int                     m_tableRecordCount = 0;             // 已经记录的数量
    bool                    m_isActive;
    char                    m_gameStartMsStamp[100];
    int64_t                 m_addedStock;                       // 每局增加的库存
    int64_t                 m_revenueAmount;                    // 每局的抽水
    int64_t                 m_RobotBetMaxBean;                  // 机器人下注最大金额
    uint32_t                m_endGameTick;

    // 库存相关
    int                     profitGold;
    int                     lossGold;
    int                     totalBetGold;
	int						m_deckOfCards[DECK_CARDS_COUNT];     // 牌组
    TableConfig             m_tabelConfig;                      // 配置
    BetRegions              m_regions;                          // 下注区域
    NFTimer					m_lastRunTick;
	NFTimer					m_SystemTick;
    SMatchLog               m_matchLog;                         // 每局比赛日志

    map<int, int64_t>       m_RankList_bet;                     // 下注排行榜
    map<int, int64_t>       m_RankList_win;                     // 输赢排行榜  
    deque<int>              tableRecordQueue;                   // 每局的历史记录
    deque<int>              m_applybanker_userIds;              // 申请上庄的玩家
    vector<NF_SHARE_PTR<NFITableUserModule>>     m_RobotList;                        // 机器人玩家列表
    vector<NF_SHARE_PTR<NFITableUserModule>>     m_userList;                         // 所有玩家
    map<int, NF_SHARE_PTR<NFITableUserModule>>   m_sitUserList;                      // userId和用户的映射，用id查找用户方便，用m_userList的内存
    map<int, float>         resultForArea;                      // 计算输赢区域

private:
    NFITableManagerModule * m_pTableManagerModule;
    //NFIWorldGameAreaModule* m_pWorldGameAreaModule;
    NFIControlCardModule  * m_pControlCardModule;
    NFIConfigAreaModule   * m_pConfigAreaModule;
    NFIGameLogicModule    * m_pGameLogicModule;
    NFIRobotModule        * m_pRobotMoudle;
    NFILogModule          * m_pLogModule;
	NFIQsTaskScheduleModule	* m_pQsTaskScheduleModule;
	NFINetClientModule	  * m_pNetClientModule;
	NFIGameServerToDBModule * m_pNFIGameServerToDBModule;
};



#endif
