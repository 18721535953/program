/*----------------------------------------------------------------
// 模块名：type_area
// 模块描述：area相关数据类型定义。
//----------------------------------------------------------------*/


#include "type_area.h"
#include "global_var.h"

// 这样为了动态改变类型的大小
int sctNone = 0;
int sctSpecial235 = 1;      // 2 3 5
int sctHighCard = 2;		// 散牌
int sct1Pair = 3;			//对子
int sctFlush = 4;			// 同花
int sctStraight = 5;		//顺子
// 预留6， 如果是扎金花 自动设置sctFlush=6
int sctFlush6666 = 6;
int sctStraightFlush = 7;	//同花顺
int sct3OfAKind = 8;			//豹子
int sctBluff = 9;


CPlayerUserInfo::CPlayerUserInfo()
{
    Clear();
}

CPlayerUserInfo::~CPlayerUserInfo()
{

}

void CPlayerUserInfo::Clear()
{
    cardList.Clear();
}

void CPlayerUserInfo::RoundClear()
{
    cardList.Clear();
}

CPlayerCard::CPlayerCard()
{
    Clear();
}

CPlayerCard::~CPlayerCard()
{

}

void CPlayerCard::Clear()
{
    m_cardCount = 0;
    m_isSort = false;
    m_gameCardAry.Clear();
    m_cardType.Clear();
}

void CPlayerCard::Sort()
{
    if(m_isSort)
        return;

    for(int i = 0; i < m_cardCount - 1; ++i)
    {
        int maxI = i;
        for(int j = i + 1; j < m_cardCount; ++j)
        {
            if(m_cardList[j] > m_cardList[maxI])
                maxI = j;
        }
        if(maxI != i)
        {
            int tmp = m_cardList[i];
            m_cardList[i] = m_cardList[maxI];
            m_cardList[maxI] = tmp;
        }
    }

    m_gameCardAry.cardCount = m_cardCount;
    for(int i = 0; i < m_cardCount; ++i)
    {
        if(!LOGIC_MGR->Int2GameCard(m_cardList[i], m_gameCardAry.cardList[i]))
        {
//            LogError("CPlayerCard::Sort", "Int2GameCard failed");
            return;
        }
    }

    m_isSort = true;
}
// 
// bool CPlayerCard::FromVObj(T_VECTOR_OBJECT& o)
// {
//     if(o.size() > MAX_USER_CARD_COUNT)
//         return false;
// 
//     m_isSort = false;
//     m_cardCount = (int)o.size();
//     m_gameCardAry.cardCount = m_cardCount;
//     for(int i = 0; i < m_cardCount; ++i)
//     {
//         m_cardList[i] = o[i]->vv.i32;
// 
//         // 要求排序倒序
//         if(i > 0)
//         {
//             if(m_cardList[i] > m_cardList[i-1])
//                 return false;
//         }
// 
//         if(!LOGIC_MGR->Int2GameCard(m_cardList[i], m_gameCardAry.cardList[i]))
//             return false;
//     }
// 
//     m_isSort = true;
//     return true;
// }

void CPlayerCard::WriteToPluto(nlohmann::json& p)
{
//     uint16_t len = (uint16_t)m_cardCount;
//     u << len;
//     for(int i = 0; i < m_cardCount; ++i)
//     {
//         u << m_cardList[i];
//     }
	try
	{
		nlohmann::json jsonStruct;
		for(int i = 0; i < m_cardCount; ++i)
		{
			jsonStruct.push_back(m_cardList[i]);
		}
		nlohmann::json jsonStructObject = nlohmann::json::object({ {"cards",jsonStruct} });

		p.insert(jsonStructObject.begin(), jsonStructObject.end());
	}
	catch (const nlohmann::detail::exception& ex)
	{
		NFASSERT(0, ex.what(), __FILE__, __FUNCTION__);
	}
}

void CPlayerCard::DealNCard(int* pDeck, int n, int& fromIndex)
{
    Clear();
    if(n <= 0)
        return;

    m_cardCount = n;
	//if (fromIndex == 0)
	//{
	//	/*for (int i = 0; i < n; ++i)
	//	{
	//	int item = pDeck[fromIndex++];
	//	m_cardList[i] = item;
	//	}*/
	//	m_cardList[0] = 111;
	//	m_cardList[1] = 112;
	//	m_cardList[2] = 113;
	//	fromIndex++;
	//	
	//}
	//else if (fromIndex == 1)
	//{
	//	m_cardList[0] = 51;
	//	m_cardList[1] = 61;
	//	m_cardList[2] = 71;
	//}
	//else
	{
		for (int i = 0; i < n; ++i)
		{
			int item = pDeck[fromIndex++];
			m_cardList[i] = item;
		}
	}

  

    Sort();
}

TGameCard::TGameCard()
{
    Clear();
}

bool TGameCard::IsValid()
{
    return (Color >= sccDiamond && Color <= sccSpade && Value >= scv2 && Value <= scvBA);
}

void TGameCard::Clear()
{
    Color = sccNone;
    Value = scvNone;
}

void TGameCard::CopyFrom(TGameCard& src)
{
    Color = src.Color;
    Value = src.Value;
}

int TGameCard::Compare(TGameCard& dec)
{
    int ret = Value - dec.Value;
    if(0 == ret)
        ret = Color - dec.Color;

    return ret;
}

TGameCardAry::TGameCardAry()
{
    Clear();
}

void TGameCardAry::Clear()
{
    cardCount = 0;
}


void TGameCardAry::WriteToPluto(nlohmann::json& u)
{
     uint16_t len = (uint16_t)cardCount;
//     u << len;
//     for(int i = 0; i < cardCount; ++i)
//     {
//         u << LOGIC_MGR->GameCard2Int(cardList[i]);
//     }

	std::string str = "";
	for (int i = 0; i < len; ++i)
	{
		str += std::to_string(LOGIC_MGR->GameCard2Int(cardList[i]));
		//if ((i + 1) < m_cardCount)
			str += ",";
	}
	
	u["cards"] =str;
}

void TGameCardAry::Sort()
{
    TGameCard tmp;
    for(int i = 0; i < cardCount - 1; ++i)
    {
        int maxI = i;
        for(int j = i + 1; j < cardCount; ++j)
        {
            if(cardList[j].Compare(cardList[maxI]) > 0)
            {
                maxI = j;
            }
        }

        if(maxI != i)
        {
            tmp.CopyFrom(cardList[i]);
            cardList[i].CopyFrom(cardList[maxI]);
            cardList[maxI].CopyFrom(tmp);
        }
    }
}

void TGameCardAry::CopyFrom(TGameCardAry& src)
{
    cardCount = src.cardCount;
    for(int i = 0; i < cardCount; ++i)
        cardList[i].CopyFrom(src.cardList[i]);
}

void TGameCardAry::Get1MinCard(TGameCardAry& ret)
{
    if (cardCount < 1)
    {
        ret.cardCount = 0;
        return;
    }

    ret.cardCount = 1;
    ret.cardList[0].CopyFrom(cardList[cardCount - 1]);
}

void TGameCardAry::AddCardToCardAry(TGameCardAry& add)
{
//     int addLen = add.cardCount;
//     if(addLen <= 0)
//         return;
//     int sourceLen = cardCount;
//     if(sourceLen <= 0)
//     {
//         CopyFrom(add);
//         return;
//     }
// 
//     TGameCardAry* pSourceAry = new TGameCardAry();
//     auto_new1_ptr<TGameCardAry> autoCardAry(pSourceAry);
//     pSourceAry->CopyFrom(*this);
// 
//     cardCount += addLen;
//     int addIndex = 0;
//     int sourceIndex = 0;
//     for(int i = 0; i < cardCount; ++i)
//     {
//         if(addIndex >= addLen)
//         {
//             cardList[i].CopyFrom(pSourceAry->cardList[sourceIndex++]);
//         }
//         else if(sourceIndex >= sourceLen)
//         {
//             cardList[i].CopyFrom(add.cardList[addIndex++]);
//         }
//         else
//         {
//             if(pSourceAry->cardList[sourceIndex].Compare(add.cardList[addIndex]) > 0)
//                 cardList[i].CopyFrom(pSourceAry->cardList[sourceIndex++]);
//             else
//                 cardList[i].CopyFrom(add.cardList[addIndex++]);
//         }
//     }
}

void TGameCardAry::AddGameCardAry(TGameCardAry& add, int AFromIndex, int AddLen)
{
    // 为Dest添加牌：  ASource的从AFromIndex开始，长度为AddLen
    if(AddLen <= 0 || AFromIndex < 0)
        return;
    if(AFromIndex + AddLen > add.cardCount)
        return;
    if(cardCount + AddLen > MAX_USER_CARD_COUNT)
    {
//        LogError("TGameCardAry::AddGameCardAry", "out of index");
        return;
    }
    int dstIndex = cardCount;
    cardCount += AddLen;
    for(int i = 0; i < AddLen; ++i)
        cardList[dstIndex+i].CopyFrom(add.cardList[AFromIndex+i]);
}

TCardScanItemAry::TCardScanItemAry()
{
    Clear();
}

void TCardScanItemAry::Clear()
{
     scanCount = 0;
}

void TCardScanItemAry::CopyFrom(TCardScanItemAry& src)
{
    scanCount = src.scanCount;
    for(int i = 0; i < scanCount; ++i)
        scanList[i].CopyFrom(src.scanList[i]);
}

void TCardScanItemAry::Sort()
{
    TCardScanItem tmp;
    for(int i = 0; i < scanCount - 1; ++i)
    {
        int maxI = i;
        for(int j = i + 1; j < scanCount; ++j)
        {
            if(scanList[j].Compare(scanList[maxI]) > 0)
            {
                maxI = j;
            }
        }

        if(maxI != i)
        {
            tmp.CopyFrom(scanList[i]);
            scanList[i].CopyFrom(scanList[maxI]);
            scanList[maxI].CopyFrom(tmp);
        }
    }
}

void TCardScanItemAry::RaiseScanAry(TGameCardAry& decCardAry, int cardIndex, int& scanIndex, TGameCard& lastCard)
{
    ++scanIndex;
    scanCount = scanIndex + 1;
    lastCard.CopyFrom(decCardAry.cardList[cardIndex]);

    TCardScanItem& item = scanList[scanIndex];
    item.Card.CopyFrom(lastCard);
    item.Count = 1;
    item.Index = cardIndex;
}

TLordCardType::TLordCardType()
{
    Clear();
}

void TLordCardType::Clear()
{
    TypeNum = sctNone;
    SameTypeValue = 0;
    TypeValue.Clear();
}

void TLordCardType::CopyFrom(TLordCardType& src)
{
    TypeNum = src.TypeNum;
    SameTypeValue = src.SameTypeValue;
    TypeValue.CopyFrom(src.TypeValue);
}

bool TLordCardType::IsValid()
{
    return (TypeNum != sctNone && TypeValue.IsValid());
}

int TLordCardType::CompareCardType(TLordCardType& decType)
{
    if (sctSpecial235 == TypeNum && sct3OfAKind == decType.TypeNum)
        return 1;
    if (sct3OfAKind == TypeNum && sctSpecial235 == decType.TypeNum)
        return -1;

    int ret = TypeNum - decType.TypeNum;
    if (0 == ret)
    {
        ret = TypeValue.Value - decType.TypeValue.Value;
        if (0 == ret)
            ret = SameTypeValue - decType.SameTypeValue;
    }

    return ret;
}

TCardScanItem::TCardScanItem()
{
    Clear();
}

void TCardScanItem::Clear()
{
    Card.Clear();
    Count = 0;
    Index = -1;
}

void TCardScanItem::CopyFrom(TCardScanItem& src)
{
    Card.CopyFrom(src.Card);
    Count = src.Count;
    Index = src.Index;
}

int TCardScanItem::Compare(TCardScanItem& dec)
{
    int ret = Count - dec.Count;
    if (0 == ret)
    {
        ret = Card.Compare(dec.Card);
    }

    return ret;
}

TGameCardAryAry::TGameCardAryAry()
{
    Clear();
}

void TGameCardAryAry::Clear()
{
    int len = Size();
    for(int i = 0; i < len; ++i)
        ary[i].Clear();
}

void TableRule::setTableRule(int bzjl, bool pkd, bool jssf, bool otqp, int fdkp, int pkr, int mpr)
{
    baoZiJiangLi = bzjl != 0;
	pkDouble = pkd;
	jieSanSuanFen = jssf;
	outTimeQiPai = otqp;
	fengDingKaiPai = fdkp;
	pkRound = pkr;
	menPiRound = mpr;
}

void TableRule::clear()
{
	 baoZiJiangLi = false;
	 pkDouble = false;
	 jieSanSuanFen = false;
	 outTimeQiPai = false;
	 fengDingKaiPai = 5;
	 pkRound = 1;
	 menPiRound = 0;
}

TableRule::TableRule(TableRule& other)
{
	baoZiJiangLi = other.baoZiJiangLi;
	pkDouble = other.pkDouble;
	jieSanSuanFen = other.jieSanSuanFen;
	outTimeQiPai = other.outTimeQiPai;
	fengDingKaiPai = other.fengDingKaiPai;
	pkRound = other.pkRound;
	menPiRound = other.menPiRound;
}

void TableRule::WriteRulePluto(nlohmann::json& u)
{
	u["baoZiJiangLi"] = baoZiJiangLi;
	u["pkDouble"] = pkDouble;
	u["jieSanSuanFen"] = jieSanSuanFen;
	u["outTimeQiPai"] = outTimeQiPai;
	u["fengDingKaiPai"] = fengDingKaiPai;
	u["pkRound"] = pkRound;
	u["menPiRound"] = menPiRound;

//	p << baoZiJiangLi << pkDouble << jieSanSuanFen << outTimeQiPai << fengDingKaiPai << pkRound << menPiRound;
}
