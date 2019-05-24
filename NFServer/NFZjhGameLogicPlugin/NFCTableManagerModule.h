#ifndef __NFC_TABLE_MANAGER_MODULE_H__
#define __NFC_TABLE_MANAGER_MODULE_H__

#include <stdlib.h>
#include <list>
#include <map>
#include <vector>
#include <memory>
#include <deque>
using std::list;
#include <inttypes.h>
#include "type_mogo.h"
#include "type_area.h"

#include "NFComm/NFPluginModule/NFITableManagerModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFPluginModule/NFIControlCardModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFComm/NFPluginModule/NFITableUserModule.h"

#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "Dependencies/json/json.hpp"

#include "NFComm/NFCore/NFUtil.hpp"

#include "NFComm/NFPluginModule/NFINetClientModule.h"

#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFMidWare/NFQsCommonPlugin/NFQsTaskStruct.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFMachineState.hpp"
#include "NFMidWare/NFQsCommonPlugin/NFCConfigAreaModule.h"
//#include "win32def.h"
//#include "epoll_server.h"
// 入座用户状态
enum
{
    tusNone = 0, 
    tusWaitNextRound = 1,           // 等待下一局游戏【可变人数游戏用到】【百家乐等百人游戏除外，游戏中进入也可以下注】
    tusNomal = 2, 
    tusOffline = 3, 
    tusFlee = 4,
};

// 桌子状态
enum
{
    tbsNone = 0,                    // 等待准备 或显示上一局结果，给客户端一定时间去显示结果, 这时可以正常离开游戏
    tbsDealCard = 1,                // 发牌，属于游戏中了
    tbsBet = 2,                     // 下注
    tbsDiscard = 3,                 // 出牌
    tbsCalcResult = 4,              // 计算结果，上报积分的过程，游戏的最后一个阶段
};

//解散桌子类型
enum
{
	clearInit = -2,						// 初始化
	clearDisagree = -1,					// 拒绝
	clearReq = 0,						// 请求散桌
	clearAgree = 1,						// 同意散桌
};


// 下注类型   弃牌、比牌、 跟牌、加注、火拼 看牌
enum
{
    sbtNone = 0,
    sbtFold = 1,			// 弃牌
    sbtVsCard = 2,			// 比牌
    sbtCall = 3,			// 跟注
    sbtRaise = 4,			// 加注
    sbtPin = 5,				// 火拼
    sbtSeeCard = 6,			// 看牌
};

// user card state： 正常, fold, VsFailed
enum
{
    ucsNormal = 0,
    ucsFold = 1,		//弃牌
    ucsVsFailed = 2,	//比牌
};

// game result type
enum
{
    grtNone = 0,
    grtUserVs = 1,          // user 主动 vs
    grtBluff = 2,           // 偷鸡
    grtSysForceVs = 3,      // 到封顶强制开
};

//桌子类型
enum
{
    vrtNone = 0,                        //桌子没有类型
    vrtBean = 1,                        //金币场
    vrtScoreAllPay = 2,                 //桌子的类型是记分局，桌费由每个人平分
    vrtScoreOnePay = 3,                 //桌子的类型是记分局，桌费由房主承担
};

// 机器人信息，用于机器人逻辑
struct TRobotInfo
{
    int CurTimeCount;                   // 当前计时到了多少
    int ActionTime;                     // 计时到多少去动作
    int betType;
    int hideBetValue;
    int vsUserId;
    int lastAction;                     // 上一次操作

    TRobotInfo();
    void OperationClear();
    void Clear();
};

// 日志相关

struct SInfoLogItem
{
    int             userId;
    int             channelId;
    int             chairId;
    int64_t         startBean;
    int64_t         endBean;
    uint64_t        totalBet;
    uint64_t        SysRect;
    string          startCards;
    string          endCards;
    string          username;
    string          nickname;

    SInfoLogItem();
};

struct SActionLogItem
{
    int             userId;
    int64_t         betBean;
    string          cardType;
    string          cards;
	int				currTimes;
	int				tstate;
    SActionLogItem();
};

struct SMatchLog
{
    uint64_t                        matchId;
    uint32_t                        typeId;
    uint32_t                        userCount;
    uint64_t                        startTime;
    uint64_t                        endTime;
    uint32_t                        handle;
    uint32_t                        selScore;
    uint32_t                        tax;

    string                          channelId;

    map<int, SInfoLogItem*>         infoLog;
    vector<SActionLogItem*>         actionLog;

    SMatchLog();
    void Clear();
    //void WriteToPluto(nlohmann::json& p);
};

class CTableUser : public NFITableUserModule
{
public:
	CTableUser();
    ~CTableUser();

    void Clear();
    bool CanBet();
public:
    int ustate;
    int isReady;                            // 定义成int为了方便发包
    int totalBet;                           // user total bet
    int cardState;
    int bSeeCard;							// 是否看牌
    int agreeEnd;                           // 是否同意了解散房间
    int isTrust;
    bool isRobot;
	int manualTrust;				        // 玩家主动点击的托管
	bool IsMenPai;							// 第一轮是否闷牌了，只有判断喜分的时候用
    int64_t enterBean;                      // 进入桌子时的金币
    int64_t leaveBean;                      // 退出桌子时的金币
    uint64_t enterTime;                     // 进入桌子时的时间戳
    uint64_t leaveTime;                     // 退出桌子时的时间戳
    uint32_t readyOrLeaveTick;
    uint32_t fillOrLeaveRobotMs;
    SUserInfo userInfo;
    CPlayerUserInfo player;
    TRobotInfo robotInfo;
    map<int, int> vsUserIds;        // server only 和自己比过的userId
    uint32_t offlineTick;           // server only
    bool hasDoBet;                  // server only 本轮是否下注过
    int64_t roundIncBean;           // server only 积分局用，计算积分
    int64_t roundRevenue;           // server only 积分局用，计算抽水
	vector<int> betAry;				// 每次押注数
	int loseRound;					// 输局数
	int winRound;					// 赢局数
	int hustraightFlushCount;		// 同花顺数
	int sct3OfAKindCount;			// 豹子数
	CCalcTimeTick betTick;

    bool isAbnormal;                // 账号异常

	virtual bool GetIsRobot() const { return isRobot; }

	virtual int GetUsTate() const { return ustate; }

	virtual SUserInfo& GetSUserInfo() { return userInfo; }
};

struct SArrangeTableItem
{
    int tableHandle;
    int selScore;
    int curUser;
    int canSitMaxUser;                                      // 最多能进入多少人
    int arrangeLen;
	NF_SHARE_PTR <SUserInfo> arrangeUserAry[MAX_TABLE_USER_COUNT];

    SArrangeTableItem();
};

class NFGameTable :public NFMachineState
{
public:
	NFGameTable(int handle, NFITableManagerModule * p);
    ~NFGameTable();
	void Init();

	void AfterInit();
	/*
   * 获取桌子的Tick
   */
	uint32_t GetTablePassTick() {
		return m_lastRunTick.GetTimePassMillionSecond();
	}
	/*
	* 添加桌子Tick
	* ms : 添加的tick
	*/
	void AddTableMsTick(uint32_t ms) {
		m_lastRunTick.SetNowTime();
	}
public:
	//todo
	int ProcMatchLogReportToDb(uint32_t taskId, uint32_t serverId, int32_t gameRoomId, int32_t isLockGame,
		uint64_t matchId, uint32_t typeId, uint32_t userCount, uint64_t startTime,
		uint64_t endTime, uint32_t handle, uint32_t selScore, uint32_t tax,
		const char* strChannelId, uint32_t winuser);
	
	int ProcUserScoreReportToDb(uint32_t taskId, uint32_t serverId, int32_t gameRoomId, int32_t isLockGame, 
		string tableNum, string openSeriesNum, string gameStartMsStamp, int32_t basescore, 
		int32_t roundFee, int32_t vipRoomType, int32_t isVipRoomEnd);

	int ProcUserLockGameRoomToDb(uint32_t taskId, int32_t gameRoomId, int32_t userId, int32_t type);
	//to go
	int ProcClientChat(const nlohmann::json& p, int chairIndex);
	int ProcClientReady(const nlohmann::json& p, int chairIndex);
    int ProcClientTrust(const nlohmann::json& p, int chairIndex);
    int ProcClientBet(const nlohmann::json& p, int chairIndex);
	int ProcClientSee(const nlohmann::json& p, int chairIndex);
    void OnReportScoreSuccess(const nlohmann::json& p, int index);
    void OnReportConsumeSpecialGoldSuccess(T_VECTOR_OBJECT* p, int index, bool isPin);
    void OnReportScoreError(int code, const char* errorMsg);

    bool CheckRunTime();
    void SetNotActive();
    bool EnterTable(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex, bool isRobot);            // 入座，可能是断线返回
    bool LeaveTable(int userId);                                                // 离开座位，可能引起断线
	NF_SHARE_PTR<CTableUser> OnlyNomalLeaveTable(int userId, bool& mayOffline);              // 只可以正常离开，不会引起断线

    void ClientUpdateUserInfo(NFGamespace::DB_UserBaseInfo& baseInfo, int chairIndex);
    void SendStartGameNotify(int chairIndex);
    int GetEmptyChairAry(int* chairAry);
    int GetRealUserCount();
    int GetRealUserMaxSelScore();
    void ClearRobotUserIfNoRealUser();
    bool AllowUserGaming(int userId);                           // 是否可以参与游戏，包括已经统计积分的user
    void UpdateCurBaseScore();

    inline int GetHandle() const
    {
        return m_handle;
    }
    inline void SetControlValue(int controlValue)
    {
        m_controlValue = controlValue;
    }
    inline int GetControlValue() const
    {
        return m_controlValue;
    }
    inline void SetControlLimit(int64_t controlLimit)
    {
        m_controlLimit = controlLimit;
    }
    inline int64_t GetControlLimit() const
    {
        return m_controlLimit;
    }
    inline bool GetIsActive() const
    {
        return m_isActive;
    }
    inline bool GetHasStartGame() const
    {
        return m_hasStartGame;
    }
    inline int GetTstate() const
    {
        return m_tstate;
    }
    inline bool IsGaming()
    {
        return m_tstate >= tbsDealCard;
    }
    inline int GetCurUserCount() const
    {
        return (int)m_sitUserList.size();
    }
    inline bool IsChairIndexValid(int chairIndex)
    {
        return (chairIndex >= 0) && (chairIndex < MAX_TABLE_USER_COUNT);
    }
    inline bool HasFd(NF_SHARE_PTR<CTableUser> pTUser)
    {
        return (pTUser->ustate != tusNone && pTUser->ustate <= tusNomal && !pTUser->isRobot);
    }
    inline void SetHasPin(int hasPin)
    {
        m_hasPin = hasPin;
    }
    inline void SetBetRule(SConfigScoreItem* pItem)
    {
        m_betRule.CopyFrom(*pItem);
    }
	inline void SetTableTule(TableRule tableRule)
	{
		m_tableRule = tableRule;
	}
    inline int GetMinBean() const
    {
        return m_minBean;
    }
    inline void SetMinBean(int minBean)
    {
        m_minBean = minBean;
    }
    inline int GetCreateUserId() const
    {
        return m_createUserId;
    }
    inline void SetCreateUserId(int userId)
    {
        m_createUserId = userId;
    }
    inline string GetTableNum() const
    {
        return m_tableNum;
    }
    inline void SetTableNum(string& tableNum)
    {
        m_tableNum = tableNum;
    }
    inline int GetMinSpecialGold() const
    {
        return m_minSpecialGold;
    }
    inline void SetMinSpecialGold(int minSpecialGold)
    {
        m_minSpecialGold = minSpecialGold;
    }
    inline int GetVipRoomType() const
    {
        return m_vipRoomType;
    }
    inline void SetVipRoomType(int roomType)
    {
        m_vipRoomType = roomType;
    }
    inline int GetCurBaseScore() const
    {
        return m_betRule.maxTotalBet;
    }
    inline bool InWinUsers(int userId)
    {
        return (m_robotWinUsers.find(userId) != m_robotWinUsers.end());
    }
private:
    void RunTime();
    void RobotAction();
    void SetIsActive();
    void ClearTableUser(int errCode, const char* errMsg);
    void NoUserClearData();
    void RoundStopClearData();
    void AddGameRound();
    void AddUser(NF_SHARE_PTR<SUserInfo> pUser, int chairIndex);
    void ClearUser(NF_SHARE_PTR<CTableUser> pTUser);                     // 清理用户，彻底离开桌子  外层不能用m_sitUserList循环
	NF_SHARE_PTR<CTableUser> FindUserById(int userId);                   // 根据id查找用户，入座的用户
	NF_SHARE_PTR<CTableUser> FindUserByChairIndex(int chairIndex);       // 根据index查找用户，包括未入座的用户
	NF_SHARE_PTR<CTableUser> FindSitUserByChairIndex(int chairIndex);    // 根据index查找用户，不包括未入座的用户
    //CMailBox* GetTUserMailBox(NF_SHARE_PTR<CTableUser> pTUser);          // 获得mailbox，机器人和断线用户是NULL
    void ClearTotalScore();
    void CheckMayClearTotalScore();
    int64_t GetUserTotalScore(int userId);
    void AddUserTotalScore(int userId, int64_t incScore);
	bool CheckHaveMenCard();
	
    void SendBroadTablePluto(CPluto* pu, int ignoreUserId); // pu will be deleted
	void SendBroadTablePluto(const uint16_t nMsgID, const std::string& strMsg , int ignordUserId);
	void SendBroadTablePluto(const uint16_t nMsgId, const nlohmann::json & jsonObject, int ignordUserId);

	void SendPlutoToUser(const uint16_t nMsgId, const nlohmann::json & jsonObject, const NFGUID& clientGuid);   // pu will be send or will be deleted if not find mailbox
	void SendPlutoToUser(const uint16_t nMsgID, const std::string& strMsg, const NFGUID& self);
	void SendPlutoToUser(const uint16_t nMsgID, const std::string& strMsg, const int& userId);
	void SendForceLeaveNotify(NF_SHARE_PTR<CTableUser> pTUser, int code, const char* errorMsg);

    void DoUserReady(NF_SHARE_PTR<CTableUser> pTUser);
    void DoUserTrust(NF_SHARE_PTR<CTableUser> pTUser, int isTrust, int ignoreUserId);
    void DoUserBet(NF_SHARE_PTR<CTableUser> pTUser, int betType, int hideBetValue, int vsUserId);
    void DoUserSeeCard(NF_SHARE_PTR<CTableUser> pTUser);
    void CheckCanDealCard();
    void StartBet();
    void StartReportScore();
    void StartReportUserLog(NF_SHARE_PTR<CTableUser> pTUser);
    void StartReportMatchLog();
    void StartGetGameControl();
    void CalcWinUsers(map<int, int>& winUsers);
    int GetVsCardUserId(int selfId);
    void DoRobotBet(NF_SHARE_PTR<CTableUser> pTUser);
    int GetActionTimeCountByCardCount(NF_SHARE_PTR<CTableUser> pTUser);

    void SendTableStateNotify();
    void SendUserStateNotify(int userId, int tuserState, int64_t& bean);
    void SendGameResultNotify();
	void collectTypeNum();
    int CanPin();
    int CanVs();
    inline bool CanLeaveWhenGaming(NF_SHARE_PTR<CTableUser> pTUser)
    {
        // 百家乐：没下注可以直接离开
        // 梭哈：下局才进入游戏可以离开
        return (pTUser->ustate < tusNomal);
    }
	int NormalCardStatNum();								//没弃牌的人数

    void UnlockOrlockUser(NF_SHARE_PTR<CTableUser> pTUser, int unlockOrlock);
    int GetGameType();
private:
    int m_handle;
    int m_controlValue;                                     // 控制信息 -1表示未收到控制信息, 0为输, 1为赢, 2为不控制
    int64_t m_controlLimit;                                 // 控制上限
    int m_getControlTimes;
    bool m_isActive;
    bool m_hasStartGame;                                    // 是否已经开局
    char m_gameStartMsStamp[100];
    uint32_t m_endGameTick;
	uint64_t m_StartGameTick;
    //CCalcTimeTick m_lastRunTick;
    int m_tstate;
    vector<NF_SHARE_PTR<CTableUser>> m_userList;
    map<int, NF_SHARE_PTR<CTableUser>> m_sitUserList;                    // userId和用户的映射，用id查找用户方便，用m_userList的内存
    int m_firstBetUserId;                                   // 首次bet的user
    int m_totalBet;                                         // 总bet
    int m_hideMinBet;                                       // 闷最低bet多少
    int m_decTimeCount;                                     // 倒计时
    int m_curUserId;                                        // 当前操作用户id
    int m_resultType;                                       // game result type
    int m_isInPin;                                          // 是否在血拼
    int m_betRound;                                         // 第几轮下注
    map<int, int> m_showCardUsers;                          // 结算时，对所有玩家都显示牌的用户id
    int m_hasPin;                                           // 规则：是否有血拼
    SConfigScoreItem m_betRule;                             // 配置bet规则
    int m_minBean;                                          // 进入条件
    int m_createUserId;                                     // 创建者
    string m_tableNum;                                      // 桌号
    int m_roundFee;                                         // 本局收费
    int m_curRound;                                         // 当前是第几局 0表示不能再玩了。
    int m_forceLeaveSec;                                    // 倒计时强制离开
    int m_clearRemainSec;                                   // 都离开后，还剩余多少秒才清理桌子。
    int m_minSpecialGold;									// 进入计分局的条件
    int m_vipRoomType;                                      // 当前的游戏是金币局还是记分局（0初始化，为1是金币局，为2是记分局（局费人人平摊），3为记分局（房主承担局费））
    map<int, int64_t> m_totalScore;                         // 总分统计
	int m_winUserId;
	bool m_dissolveTable;									// 是否解散桌子
    int m_isReportFinished;                                 // 是否上报结束
    
    SMatchLog m_matchLog;                                   //每局比赛日志

	uint32_t m_clearTableTick;								// server only  散桌请求timetick
	TableRule m_tableRule;

    map<int, int> m_robotWinUsers;                          // sever only for robot 赢的玩家列表
    int m_lastUserId;                                       // tmp for robot logic


	NFITableManagerModule * m_pTableManagerModule;
	//NFIWorldGameAreaModule* m_pWorldGameAreaModule;
	//NFIControlCardModule  * m_pControlCardModule;
	NFIConfigAreaModule   * m_pConfigAreaModule;
	NFIGameServerNet_ServerModule * m_pGameServerNet_ServerModule;
	NFIGameLogicModule    * m_pGameLogicModule;
	NFIRobotModule        * m_pRobotMoudle;
	NFILogModule          * m_pLogModule;
	NFIQsTaskScheduleModule		* m_pQsTaskScheduleModule;
	NFINetClientModule	  * m_pNetClientModule;
	NFIGameServerToDBModule * m_pNFIGameServerToDBModule;
	NFTimer					m_lastRunTick;
};

struct SArrangeUserList
{
	NF_SHARE_PTR <SUserInfo> UserAry[MAX_FILE_HANDLE_COUNT];
    int len;
    int selScore;

    SArrangeUserList();
    void Clear();
    int SortIfTimeout(uint32_t msTime);         // 返回超时人数数量，0表示没人超时
};

struct SGraphItem
{
	NF_SHARE_PTR <SUserInfo> value;                           // 节点值
    list<SGraphItem*> edge;                     // 边
    int color;                                  // 颜色，DFS访问用，0=white 1=gray 2=black

    SGraphItem();
};

// 附近的人构成的图
class CNearUserGraph
{
public:
    list<SArrangeUserList*> NearList;                                       // 结果

    CNearUserGraph();
    ~CNearUserGraph();

    void Clear();
    void UpdateGraph(int distance);                                         // 刷新图 distance表示距离多少
private:
    void DfsVisit(SGraphItem* pItem);
public:
	SArrangeUserList m_userList;                                            // 节点用户
private:
    vector<SGraphItem*> m_graph;                                            // 图
    SArrangeUserList* m_pCurNear;                                           // 当前处理的树
};

//////////////////////////////////////////////////////////////////////////
/// CGameTableMgr - 游戏桌子管理器
class NFCTableManagerModule
	:public NFITableManagerModule
{
public:
	NFCTableManagerModule(NFIPluginManager* p);
    ~NFCTableManagerModule();
	virtual bool Awake() override;

	virtual bool Init() override;

	virtual bool AfterInit() override;

	virtual bool Execute() override;

	virtual bool Shut() override;
public:

public:
	void OnNodeDbGidReadUserInfoCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void OnMsGidScoreReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void OnMsGidGameControlCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void OnMsGidMatchReportCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void OnMsGidLockGameRoomCallback(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
public:
    void RunTime();
    void OnUserOffline(int userId);

	virtual NF_SHARE_PTR<NFGameTable> GetUserTable(int userId, int& chairIndex);
	NF_SHARE_PTR<NFGameTable> GetPUserTable(const SUserInfo& pUser, int& chairIndex) override;
    bool AddSitUser(int userId, int tableHandle, int chairIndex);
    bool RemoveSitUser(int userId);
    bool RemoveUserCreateTable(int userId);

    inline  virtual NF_SHARE_PTR<NFGameTable> GetTableByHandle(int handle)
    {
        if(handle >= 0 && handle < (int)m_tablelist.size())
            return m_tablelist[handle];
        else
            return NULL;
    }
    inline int GetActiveTableCount() const
    {
        return m_activeTableCount;
    }
    virtual inline int GetUserCount() 
    {
        return (int)m_sitAllUserList.size();
    }
private:
    bool FindSitUser(int userId, int& retTableHandle, int&retChairIndex);
    void ArrangeTableForNearUser();
    void ArrangeTableForRealUser();
    bool UpdateCanArrangeTableList();                       // 获得可以入座的桌子列表
    void EnterTableForArrangeUser();                        // 开始入座，真实加入桌子, 这时候才去查找座位
    SArrangeTableItem* FindEmptyTable(int selScore, int minEmptyChair, int maxEmptyChair, int ignoreHandle, NF_SHARE_PTR <SUserInfo> pUser);
    void AddUserToArrange(SArrangeUserList* pUserList, SArrangeTableItem* pItem, int delUserIndex, NF_SHARE_PTR <SUserInfo> pUser);

    bool CanSitInSameTable(NF_SHARE_PTR <SUserInfo> pUser1, NF_SHARE_PTR <SUserInfo> pUser2);
private:
    vector<NF_SHARE_PTR<NFGameTable>> m_tablelist;
    int m_activeTableCount;
    map<int, uint32_t> m_sitAllUserList;                // value为桌子和座位的合并值 (chairIndex<<16) | (tablehandle & 0xFFFF), 断线返回也可以从这里查找
    map<int, int> m_userId2CreateTable;                 // userId和他创建的桌子的对应关系
    CCalcTimeTick m_arrangeTableTime;
    CNearUserGraph m_graph;
    map<int, SArrangeUserList*> m_arrangeUserMap;
    SArrangeTableItem* m_arrangeTableAry[MAX_FILE_HANDLE_COUNT];
    int m_arrangeTableLen;
    int m_emptyChairAry[MAX_TABLE_USER_COUNT];
	NFILogModule *                  m_pLogModule;
	//NFIWorldGameAreaModule *        m_pWorldGameAreaModule;
	NFIGameServerNet_ServerModule * m_pGameServerNet_ServerModule;
//	NFINetCommonModule *            m_pNetCommonModule;
	NFIGameLogicModule *            m_pGameLogicModule;
	NFIConfigAreaModule *           m_pConfigAreaModule;
	NFIRobotModule *                m_pRobotModule;
	NFITableManagerModule *         m_pTableManagerModule;
	NFINetClientModule *			m_pNetClientModule;
	NFIQsTaskScheduleModule * m_pQsTaskScheduleModule;
public :
	//用不到的api
	virtual void AddTable(NF_SHARE_PTR<NFGameTable> newTable){}
	virtual int GetSelScoreByHandle(int handle) { return 0; }
	//玩家通过选择底分的方式进入房间
	virtual bool EnterRoomBySelectSocre(NF_SHARE_PTR<SUserInfo> pUser, int ignoreTableHandle, std::function<void()> successCallBack) { return false; }
	virtual bool userChangeTable(SUserInfo* pUser, NFGameTable* oldTable, std::function<void()> successCallBack) { return false; }
	/*
   * 发送邮件给单个用户
   * share_pu : 消息包
   * fd : socket 描述符
   */
	virtual void SendMailToTableUser(const int nMsgId, const nlohmann::json& jsonSend, const NFGUID& fd) 
	{
		if (!m_pGameLogicModule->FindUserByGuid(fd))
			m_pLogModule->LogError("share_pu == nullptr", __FUNCTION__, __LINE__);

		m_pGameLogicModule->SendMsgToClient(nMsgId, jsonSend, fd);
	}

	/*
	 * 广播邮件给一个桌子上的所有用户
	 * share_pu : 消息包
	 * ignoreUserId : 忽略的用户ID
	 * isIgnoreState : 忽略的用户状态，处于此状态的用户，不发包
	 * tableUserMap : 用户列表
	 */
	virtual void SendMailToUserAtTheSameTable(const int nMsgId, const nlohmann::json& jsonSend, const int ignoreUserId, int isIgnoreState, std::map<int, NF_SHARE_PTR<NFITableUserModule>>& tableUserMap) 
	{
	}

	/*
	* 广播邮件给所有桌子上的所有用户
	* share_pu : 消息包
	* ignoreUserId : 忽略的用户ID
	* tableVector : 桌子列表
	*/
	virtual void SendMailToAllTableUser(const int nMsgId, const nlohmann::json& jsonSend, int ignoreUserId, std::vector<NF_SHARE_PTR<NFGameTable>> & tableVector) 
	{
	}

};


#endif
