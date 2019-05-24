#ifndef GAME_CONFIG_HPP
#define GAME_CONFIG_HPP

// 游戏相关
// 最少人数
#define MIN_TABLE_USER_COUNT 1
// 最多人数
#define MAX_TABLE_USER_COUNT 512
// 游戏中是否可以入座  百家乐、梭哈等
#define GAME_CAN_ENTER_WHEN_GAMING true
// 游戏中进入的人是否可以直接游戏，百家乐不可以
#define GAME_CAN_DIRECT_GAME false
// 每人最多有多少牌
#define MAX_USER_CARD_COUNT 3
//展示结果动画时间
#define TIME_COUNT_RESULT   10
//玩家下注时间
#define TIME_COUNT_BET   15

///库存相关
// 库存容量
#define INVENTORY_MAX_CAPACTICY 10
// 盈利多少场 ( <= INVENTORY_MAX_CAPACTICY)
#define PROFIT_COUNT 8

//玩家几次没有操作将被踢出
#define TIMES_NO_OPERATOR 6

#define TABLE_RECORD_COUNT 200

#define DECK_CARDS_COUNT 416        // 8副牌  剔除大小王

// 通用
#define MAX_TABLE_HANDLE_COUNT 2
#define ERROR_CODE_BEAN_TOO_LITTLE 800
#define INVALID_TIME_COUNT -1
#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif

#define C_GAME_ID 5

#define PLAY_TYPE_ID 500
#define GAME_ROOM_ID 50003

#define MS_PER_SEC 1000
#define INVALID_USERID -1
#define INVALID_INDEX -1

enum TableSitState
{
    tusNone = 0,
    tusWaitNextRound = 1,           // 等待下一局游戏【可变人数游戏用到】【百家乐等百人游戏除外，游戏中进入也可以下注】
    tusNomal = 2,
    tusOffline = 3,
    tusFlee = 4,
};

enum TableState
{
    TABLE_IDLE      = 0,            //桌子空闲状态
    TABLE_WAIT_GAME_START = 1,      //桌子等待游戏开始          （已经至少有一个玩家进入）
    TABLE_WAGER_1 = 2,              //桌子押注状态(第一轮)
    TABLE_WAGER_2 = 3,              //桌子押注状态(第二轮)
    TABLE_DEAL = 4,                 //发牌                        （牌值的计算也在这个阶段）
    TABLE_FILL_CARD = 5,            //补牌
    TABLE_CALCULATE_RESULT = 6,     //计算结果，上报积分
    TABLE_WAIT_FOR_NEXT_GAME = 7,   //游戏结束，等待下一局游戏
    TABLE_WITH_ANIMATION = 8,       //游戏等待动画（发牌动画等）
    TABLE_CONTINUE = 100,
    TABLE_ERROR = -1
};


enum TCardColor
{
    sccNone = 0,
    sccDiamond = 1,         // 方块
    sccClub = 2,            // 梅花
    sccHeart = 3,           // 红心
    sccSpade = 4,           // 黑桃
    sccJoker = 5,           // 王
};

enum TCardValue
{
    CARD_NONE = 0,
    CARD_A = 1,
    CARD_2 = 2,
    CARD_3 = 3,
    CARD_4 = 4,
    CARD_5 = 5,
    CARD_6 = 6,
    CARD_7 = 7,
    CARD_8 = 8,
    CARD_9 = 9,
    CARD_10 = 10,
    CARD_J = 11,
    CARD_Q = 12,
    CARD_K = 13,
};


// 百家乐押注区域
enum WagerAreaType
{
    //0,4,5,6,8
    ARBITRARILY_DOUBLE = 0,     //任意对子
    DOGFALL            = 1,     //和
    BIG                = 2,     //大
    NOT_DEALEAR_DOUBLE = 3,     //闲对
    PERFECT_DOUBLE     = 4,     //完美对子
    SMALL              = 5,     //小
    DEALER_DOUBLE      = 6,     //庄对
    DEALER             = 7,     //庄
    NOT_DEALEAR        = 8,     //闲
    NONE               = -1
};

enum CardID
{
    DEALER_ID = 1,
    NOT_DEALER_ID = 2
};

enum WorldID
{
    GAME_AREA = 0,
    BD_MGR    = 1,
    AREA_GMR  = 2,

    NONE_WORLD      = -1
};

#endif