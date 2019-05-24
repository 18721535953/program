#ifndef __CARD__LOGIC__HEAD__
#define __CARD__LOGIC__HEAD__

//#include "win32def.h"
#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
//#include "memory_pool.h"
//#include "json_helper.h"
#include "type_area.h"
#include <stdlib.h>
#include <list>
#include <vector>
using std::list;
#include <inttypes.h>

#define DECK_CARDS_COUNT 52
using namespace std;
//////////////////////////////////////////////////////////////////////////
/// CGameLogicMgr - 游戏逻辑管理器
class CGameLogicMgr
{
public:
    CGameLogicMgr();
    ~CGameLogicMgr();

public:
	static CGameLogicMgr* Inst();
	static void FreeInst();

public:
    bool Int2GameCard(int src, TGameCard& dst);
    int GameCard2Int(TGameCard& src);
    string GameCardAry2Str(TGameCardAry& src);

    void DealCard(vector<CPlayerCard*>& userCards, int selectId);
    void CalcCardAryType(TGameCardAry& ADecSortCardAry, TCardScanItemAry& RetScanAry, TLordCardType& RetCardType);
private:
    void RandomCardDeck();
    void GetCardScanTable(TGameCardAry& ADecSortCardAry, TCardScanItemAry& RetScanArySortByCount);
private:
    int m_deckOfCards[DECK_CARDS_COUNT];
};


#endif
