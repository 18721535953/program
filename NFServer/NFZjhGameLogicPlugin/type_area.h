#ifndef __TYPE__AREA__HEAD__
#define __TYPE__AREA__HEAD__

//#include "win32def.h"
#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
//#include "memory_pool.h"
//#include "json_helper.h"
#include "type_mogo.h"
#include <stdlib.h>
#include <list>
using std::list;
#include <inttypes.h>

// 游戏相关
// 最少人数
#define MIN_TABLE_USER_COUNT 2
// 最多人数
#define MAX_TABLE_USER_COUNT 5
// 游戏中是否可以入座  百家乐、梭哈等
#define GAME_CAN_ENTER_WHEN_GAMING false
// 游戏中进入的人是否可以直接游戏，百家乐可以
#define GAME_CAN_DIRECT_GAME false
// 每人最多有多少牌
#define MAX_USER_CARD_COUNT 3
// 发牌动画时间
#define TIME_COUNT_DEAL_CARD 2

// 通用
#define MAX_TABLE_HANDLE_COUNT 1024
#define MIN_CLIENT_VERSTION 1
#define ERROR_CODE_BEAN_TOO_LITTLE 800
#define ERROR_CODE_VERSION_TOO_LITTLE 801
#define ERROR_CODE_TO_MAX_ROUND 802
#define ERROR_CODE_SPECIALGOLD_TOO_LITTLE 803  
#define ERROR_CODE_SOMEONE_LEAVE 804
#define ERROR_CODE_CREATE_TABLE_FAILED 805
#define ERROR_CODE_USER_CLEAR_TABLE 806
#define ERROR_CODE_GAME_OVER 807
#define INVALID_TIME_COUNT -1
#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif

#define FS_REPORT_TYPE_START 1
#define FS_REPORT_TYPE_TICK 2

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
    scvNone = 0,
    scv2 = 2,
    scv3 = 3, 
    scv4 = 4, 
    scv5 = 5,
    scv6 = 6, 
    scv7 = 7, 
    scv8 = 8, 
    scv9 = 9, 
    scv10 = 10, 
    scvJ = 11, 
    scvQ = 12, 
    scvK = 13,
    scvBA = 14, 
};

struct TGameCard
{
    int Color;
    int Value;

    TGameCard();
    bool IsValid();
    void Clear();
    void CopyFrom(TGameCard& src);
    int Compare(TGameCard& dec);
};

// 所有的牌默认都是倒序排好的
struct TGameCardAry
{
    int cardCount;
    TGameCard cardList[MAX_USER_CARD_COUNT];

    TGameCardAry();
    void Clear();
    void WriteToPluto(nlohmann::json& u);
    void Sort();                                            // dec sort only for logic return card
    void CopyFrom(TGameCardAry& src);
    void Get1MinCard(TGameCardAry& ret);
    void AddCardToCardAry(TGameCardAry& add);
    void AddGameCardAry(TGameCardAry& add, int AFromIndex, int AddLen);
};

struct TGameCardAryAry
{
    TGameCardAry ary[MAX_TABLE_USER_COUNT];

    TGameCardAryAry();
    void Clear();

    inline int Size()
    {
        return MAX_TABLE_USER_COUNT;
    }
};

struct TCardScanItem
{
    TGameCard Card;
    int Count;
    int Index;                      // Card在原来牌中的索引

    TCardScanItem();
    void Clear();
    void CopyFrom(TCardScanItem& src);
    int Compare(TCardScanItem& dec);
};

struct TCardScanItemAry
{
    int scanCount;
    TCardScanItem scanList[MAX_USER_CARD_COUNT];

    TCardScanItemAry();
    void Clear();
    void CopyFrom(TCardScanItemAry& src);
    void Sort();                                // dec sort
    void RaiseScanAry(TGameCardAry& decCardAry, int cardIndex, int& scanIndex, TGameCard& lastCard);
};

enum
{
    bombTypeMix = 0,            // 软炸弹
    bombTypeNoLz = 1,           // 硬炸弹
    bombTypeLz = 2,             // 纯癞子炸弹
    bombTypeRocket = 3,         // 火箭
};

// 这样为了动态改变类型的大小
extern int sctNone;
extern int sctSpecial235;
extern int sctHighCard;
extern int sct1Pair;
extern int sctFlush;
extern int sctStraight;
// 预留6， 如果是扎金花 自动设置sctFlush=6
extern int sctFlush6666;
extern int sctStraightFlush;
extern int sct3OfAKind;
extern int sctBluff;

struct TLordCardType
{
    int TypeNum;
    TGameCard TypeValue;
    int SameTypeValue;  // 当TypeNum和TypeValue一样大的时候， 这个才去比较大小

    TLordCardType();
    void Clear();
    void CopyFrom(TLordCardType& src);
    bool IsValid();
    int CompareCardType(TLordCardType& decType);
};

// 玩家牌
class CPlayerCard
{
public:
    CPlayerCard();
    ~CPlayerCard();
    void Clear();

    void Sort(); // dec sort
//    bool FromVObj(T_VECTOR_OBJECT& o);
    void WriteToPluto(nlohmann::json& p);                           // only for deal card
    void DealNCard(int* pDeck, int n, int& fromIndex);

    inline TGameCardAry* GetGameCard()
    {
        return &m_gameCardAry;
    }
public:
    TLordCardType m_cardType;
private:
    int m_cardCount;                            // will not be changed after deal
    int m_cardList[MAX_USER_CARD_COUNT];        // will not be changed after deal
    bool m_isSort;
    TGameCardAry m_gameCardAry;                 // sort之后，才有数据 will be changed when discard
};

// 游戏用户信息
class CPlayerUserInfo
{
public:
    CPlayerUserInfo();
    ~CPlayerUserInfo();
    void Clear();
    void RoundClear();
public:
    CPlayerCard cardList;    
};

struct TableRule
{
	TableRule()
	{
		clear();
	};

	TableRule(TableRule& other);

	bool baoZiJiangLi;
	bool pkDouble;
	bool jieSanSuanFen;
	bool outTimeQiPai;
	int fengDingKaiPai;
	int pkRound;
	int menPiRound;

	void setTableRule(int bzjl, bool pkd, bool jssf, bool otqp,int fdkp, int pkr, int mpr);
	void clear();
	void WriteRulePluto(nlohmann::json& u);

};
#endif
