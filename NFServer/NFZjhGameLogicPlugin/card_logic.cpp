/*----------------------------------------------------------------
// 模块名：card_logic
// 模块描述：牌逻辑处理
//----------------------------------------------------------------*/


#include "card_logic.h"
#include "NFServer/NFGameServerNet_ServerPlugin/logger.hpp"
#include <limits.h>
#include "NFComm/NFPluginModule/NFIKernelModule.h"
#include "NFComm/NFMessageDefine/NFProtocolDefine.hpp"
#include "NFComm/NFMessageDefine/NFMsgPreGame.pb.h"
static CGameLogicMgr* gStaticGameLogic = NULL;

//////////////////////////////////////////////////////////////////////////
CGameLogicMgr::CGameLogicMgr()
{
	NFMsg::MsgBase x;
    memset(m_deckOfCards, 0, sizeof(m_deckOfCards));
}

CGameLogicMgr::~CGameLogicMgr()
{

}

CGameLogicMgr* CGameLogicMgr::Inst()
{

	if (gStaticGameLogic == NULL)
		gStaticGameLogic = new CGameLogicMgr();
	return gStaticGameLogic;
}

void CGameLogicMgr::FreeInst()
{
	if (gStaticGameLogic != NULL)
	{
		delete gStaticGameLogic;
		gStaticGameLogic = NULL;
	}
}

bool CGameLogicMgr::Int2GameCard(int src, TGameCard& dst)
{
    dst.Color = src % 10;
    dst.Value = src / 10;

    return dst.IsValid();
}

int CGameLogicMgr::GameCard2Int(TGameCard& src)
{
    return (src.Value * 10 + src.Color);
}

string CGameLogicMgr::GameCardAry2Str(TGameCardAry& src)
{
    char buffer[20];
    string Result = "[";
    for(int i = 0; i < src.cardCount; ++i)
    {
        if(0 == i)
            snprintf(buffer, sizeof(buffer), "%d", GameCard2Int(src.cardList[i]));
        else
            snprintf(buffer, sizeof(buffer), ",%d", GameCard2Int(src.cardList[i]));
        Result += buffer;
    }
    Result += "]";

    return Result;
}

void CGameLogicMgr::DealCard(vector<CPlayerCard*>& userCards, int selectId)
{
    RandomCardDeck();

    TCardScanItemAry LScanAry;
    int fromIndex = 0;
    int len = (int)userCards.size();
    for (int i = 0; i < len; ++i)
    {
        userCards[i]->DealNCard(m_deckOfCards, 3, fromIndex);
        CalcCardAryType(*userCards[i]->GetGameCard(), LScanAry, userCards[i]->m_cardType);
    }

    // selectId为-1时表示不控制
    if (selectId == -1)
        return;

    for (int i = 0; i < len; i++)
    {
        if (i == selectId)
            continue;

        if ((*userCards[i]).m_cardType.CompareCardType((*userCards[selectId]).m_cardType) > 0)
        {
            // 当前玩家的牌比选中人的牌大，将它们交换
            LogInfo("CGameLogicMgr::DealCard", "交换玩家与选中机器人的牌");
            CPlayerCard tmp = *userCards[selectId];
            *userCards[selectId] = *userCards[i];
            *userCards[i] = tmp;
        }
    }
}

void CGameLogicMgr::CalcCardAryType(TGameCardAry& ADecSortCardAry, TCardScanItemAry& RetScanAry, TLordCardType& RetCardType)
{
    const int CMaxCardValue = 20;                              // 为了方便比较大小
    int LScanAryLen;
    // 计算牌型
    RetCardType.Clear();
    GetCardScanTable(ADecSortCardAry, RetScanAry);

    if (3 == ADecSortCardAry.cardCount)
    {
        LScanAryLen = RetScanAry.scanCount;
        TCardScanItem& scan0Item = RetScanAry.scanList[0];
        switch (LScanAryLen)
        {
            case 1:
            {
                RetCardType.TypeValue.CopyFrom(scan0Item.Card);
                RetCardType.SameTypeValue = scan0Item.Card.Value;
                RetCardType.TypeNum = sct3OfAKind;

                break;
            }
            case 2:
            {
                // 可以为 2+1
                RetCardType.TypeValue.CopyFrom(scan0Item.Card);
                RetCardType.SameTypeValue = scan0Item.Card.Value * CMaxCardValue + RetScanAry.scanList[1].Card.Value;
                RetCardType.TypeNum = sct1Pair;
                break;
            }
            case 3:
            {
                bool LIsFlush;
                bool LIsStraight;
                int LStraightCount;
                // 可以为 同花,顺子,同花顺,散牌
                LIsFlush = true;
                {
                    // CalcIsFlush
                    int LFirstColor = ADecSortCardAry.cardList[0].Color;
                    for (int i = 1; i < ADecSortCardAry.cardCount; ++i)
                    {
                        if (ADecSortCardAry.cardList[i].Color != LFirstColor)
                        {
                            LIsFlush = false;
                            break;
                        }
                    }
                }
                LStraightCount = 0;
                {
                    //CalcMaxStraightCount
                    // 计算最多有几个连续的顺子
                    int LMaxCount = 1;
                    int LCount = 1;
                    int LLastValue = ADecSortCardAry.cardList[0].Value;

                    for (int i = 1; i < ADecSortCardAry.cardCount; ++i)
                    {
                        TGameCard& item = ADecSortCardAry.cardList[i];
                        if (LLastValue - item.Value == 1)
                        {
                            ++LCount;
                        }
                        else
                        {
                            if (LMaxCount < LCount)
                                LMaxCount = LCount;

                            LCount = 1;
                        }

                        LLastValue = item.Value;
                    }
                    if (LMaxCount < LCount)
                        LMaxCount = LCount;

                    LStraightCount = LMaxCount;
                }

                RetCardType.TypeValue.CopyFrom(ADecSortCardAry.cardList[0]);
                RetCardType.SameTypeValue = scan0Item.Card.Value * CMaxCardValue * CMaxCardValue 
                    + RetScanAry.scanList[1].Card.Value * CMaxCardValue + RetScanAry.scanList[2].Card.Value;
                if (3 == LStraightCount)
                {
                    LIsStraight = true;
                }
                else if (2 == LStraightCount)
                {
                    // A32   A23的顶张为3，A算1，属于最小的顺子
                    LIsStraight = (scvBA == ADecSortCardAry.cardList[0].Value) && (scv3 == ADecSortCardAry.cardList[1].Value);
                    if (LIsStraight)
                        RetCardType.TypeValue.CopyFrom(ADecSortCardAry.cardList[1]);
                }
                else
                {
                    LIsStraight = false;
                }

                if (LIsStraight)
                {
                    if (LIsFlush)
                    {
                        RetCardType.TypeNum = sctStraightFlush;
                    }
                    else
                    {
                        RetCardType.TypeNum = sctStraight;
                    }
                }
                else if (LIsFlush)
                {
                    RetCardType.TypeNum = sctFlush;
                }
                else
                {
                    // 532
                    if (scv5 == ADecSortCardAry.cardList[0].Value
                        && scv3 == ADecSortCardAry.cardList[1].Value && scv2 == ADecSortCardAry.cardList[2].Value)
                    {
                        RetCardType.TypeNum = sctSpecial235;
                    }
                    else
                    {
                        RetCardType.TypeNum = sctHighCard;
                    }
                }

                break;
            }
        }
    }
}

void CGameLogicMgr::RandomCardDeck()
{
    int index = 0;
    for (int value = scv2; value <= scvBA; ++value)
    {
        for (int color = sccDiamond; color <= sccSpade; ++color)
        {
            m_deckOfCards[index++] = value * 10 + color;
        }
    }

    for (int randCnt = 0; randCnt < 3; ++randCnt)
    {
        // random cards
        for (int i = 0; i < DECK_CARDS_COUNT; ++i)
        {
            int newIndex = rand() % DECK_CARDS_COUNT;
            if (newIndex != i)
            {
                int tmp = m_deckOfCards[i];
                m_deckOfCards[i] = m_deckOfCards[newIndex];
                m_deckOfCards[newIndex] = tmp;
            }
        }
    }
}

void CGameLogicMgr::GetCardScanTable(TGameCardAry& ADecSortCardAry, TCardScanItemAry& RetScanArySortByCount)
{
    // 计算牌的扫描表格
    // 根据牌值生成一个按照牌的数量从多到少排序的扫描表
    if (ADecSortCardAry.cardCount < 1)
    {
        RetScanArySortByCount.scanCount = 0;
        return;
    }

    int scanIndex = -1;
    TGameCard lastCard;
    RetScanArySortByCount.RaiseScanAry(ADecSortCardAry, 0, scanIndex, lastCard);
    int len = ADecSortCardAry.cardCount;

    for (int i = 1; i < len; ++i)
    {
        if (lastCard.Value == ADecSortCardAry.cardList[i].Value)
            ++RetScanArySortByCount.scanList[scanIndex].Count;
        else
            RetScanArySortByCount.RaiseScanAry(ADecSortCardAry, i, scanIndex, lastCard);
    }

    RetScanArySortByCount.Sort();
}


