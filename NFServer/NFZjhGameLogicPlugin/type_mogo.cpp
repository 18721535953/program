/*----------------------------------------------------------------
// 模块名：type_mogo
// 模块描述：def相关数据类型定义。
//----------------------------------------------------------------*/


#include "type_mogo.h"
//#include "pluto.h"
#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
#include "NFServer/NFGameServerNet_ServerPlugin/logger.hpp"
//#include "rpc_mogo.h"

uint32_t g_taskIdAlloctor;

SUserBaseInfo::SUserBaseInfo()
{
    Clear();
}

void SUserBaseInfo::Clear()
{
    userId = INVALID_USERID;
    userType = 0;
    channelId = 0;
    score = 0;
    bean = 0;
    userName = "";
    nickName = "";
    sex = 0;
    level = 0;
    faceId = 0;
    faceUrl = "";
    specialGold = 0;
}

void SUserBaseInfo::CopyFrom(NFGamespace::DB_UserBaseInfo& src)
{
    userId = src.userId;
    userType = src.userType;
    score = stoll(src.score);
    bean = stoll(src.bean);
    userName = src.userName;
    nickName = src.nickName;
    sex = src.sex;
    level = src.level;
    faceId = src.faceId;
    faceUrl = src.faceUrl;
    specialGold = stoll(src.specialGold);
    channelId = src.channelId;
	gameRoomLockStatus = src.gameRoomLockStatus;
	lastLockGameRoomId = src.lastLockGameRoomId;
}

void SUserBaseInfo::WriteToPluto(nlohmann::json& p)
{
//	p << userId << userType << score << bean << userName << nickName << sex << level << faceId << faceUrl << specialGold ;

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
		jsonStruct["specialGold"] = specialGold;
		//jsonStruct["gameRoomLockStatus"] = gameRoomLockStatus;
		//jsonStruct["lastLockGameRoomId"] = lastLockGameRoomId;
		//jsonStruct["totalBetCount"] = totalBetCount;
		//jsonStruct["winCount"] = winCount;

		nlohmann::json jsonStructObject = nlohmann::json::object({ {"baseInfo",jsonStruct} });

		p.insert(jsonStructObject.begin(), jsonStructObject.end());
	}
	catch (const nlohmann::detail::exception& ex)
	{
		NFASSERT(0, ex.what(), __FILE__, __FUNCTION__);
	}
}

// void SUserBaseInfo::ReadFromJson(cJSON* pJsObj)
// {
//     FindJsonItemIntValue(pJsObj, "userId", userId);
//     FindJsonItemIntValue(pJsObj, "userType", userType);
//     FindJsonItemIntValue(pJsObj, "channelId", channelId);
//     FindJsonItemInt64Value(pJsObj, "score", score);
//     FindJsonItemInt64Value(pJsObj, "bean", bean);
//     FindJsonItemStrValue(pJsObj, "userName", userName);
//     FindJsonItemStrValue(pJsObj, "nickName", nickName);
//     FindJsonItemIntValue(pJsObj, "sex", sex);
//     FindJsonItemIntValue(pJsObj, "level", level);
//     FindJsonItemIntValue(pJsObj, "faceId", faceId);
//     FindJsonItemStrValue(pJsObj, "faceUrl", faceUrl);
//  	FindJsonItemInt64Value(pJsObj, "specialGold", specialGold);  
// 	FindJsonItemIntValue(pJsObj, "gameRoomLockStatus", gameRoomLockStatus);
// 	FindJsonItemIntValue(pJsObj, "lastLockGameRoomId", lastLockGameRoomId); 
// }

void SUserBaseInfo::ReadFromVObj(T_VECTOR_OBJECT& o, int& index)
{
	//LogError("6666666666666666666666666r", "%d", 1);
    userId = o[index++]->vv.i32;
	//LogError("3 userId ReadFromVObj", "%d", userId);
    userType = o[index++]->vv.i32;
	//LogError("3userType333333333333333error", "%d", userType);
	string buffscore;
	buffscore = *o[index++]->vv.s;
	score = stoll(buffscore);
	//LogError("33score33333333333333333error", "%lld", score);
	string buffBean;
	buffBean = *o[index++]->vv.s;
    bean = stoll(buffBean);
	//LogError("3333bean ----3333333333333error", "%lld", bean);
    userName = *o[index++]->vv.s;
	//LogError("3userName333333333333333error", "%s", userName.c_str());
    nickName = *o[index++]->vv.s;
	//LogError("33nickName33333333333333error", "%s", userName.c_str());
    sex = o[index++]->vv.i32;
	//LogError("33s3333333333333333333333error", "%d", sex);
    level = o[index++]->vv.i32;
	//LogError("33333l3333333333333333333error", "%d", level);
    faceId = o[index++]->vv.i32;
	//LogError("3333333f33333333333333333error", "%d", faceId);
    faceUrl = *o[index++]->vv.s;
	//LogError("33faceUrl333333333333333333error", "%s", faceUrl.c_str());
	string gold = *o[index++]->vv.s;
    specialGold = stoll(gold);
	//LogError("33specialGold333333333333333333error", "%lld", specialGold);
	gameRoomLockStatus = o[index++]->vv.i32;
	//LogError("33gameRoomLockStatus333333333333333error", "%d", gameRoomLockStatus);
	lastLockGameRoomId = o[index++]->vv.i32;
	//LogError("33lastLockGameRoomId3333333333333333error", "%d", lastLockGameRoomId);
    channelId = o[index++]->vv.i32;
	//LogError("3channelId33333333333333333error", "%d", channelId);
}

SCisScoreReportRetItem::SCisScoreReportRetItem() : userId(-1), score(0), bean(0), specialGold(0), \
    incScore(0), incBean(0), experience(0), level(0), expands()
{

}


void SCisScoreReportRetItem::WriteToPluto(CPluto& p)
{
//    p << userId << score << bean << specialGold << incScore << incBean
//        << experience << level << expands;
}

// void SCisScoreReportRetItem::ReadFromJson(cJSON* pJsObj)
// {
//     FindJsonItemIntValue(pJsObj, "userId", userId);
//     FindJsonItemInt64Value(pJsObj, "score", score);
//     FindJsonItemInt64Value(pJsObj, "bean", bean);
//     FindJsonItemInt64Value(pJsObj, "specialGold", specialGold);
//     FindJsonItemInt64Value(pJsObj, "incScore", incScore);
//     FindJsonItemInt64Value(pJsObj, "incBean", incBean);
//     FindJsonItemIntValue(pJsObj, "experience", experience);
//     FindJsonItemIntValue(pJsObj, "level", level);
//     FindJsonItemStrValueForObject(pJsObj, "expands", expands);
// }

void SCisScoreReportRetItem::ReadFromVObj(T_VECTOR_OBJECT& o, int& index)
{
    userId = o[index++]->vv.i32;
	string strscore = *(o[index++]->vv.s);
    score = stoll(strscore);
	//LogError("score ----3333333333333error", "%lld", score);
	string strBean = *(o[index++]->vv.s);
    bean = stoll(strBean);
	//LogError("bean ----3333333333333error", "%lld", bean);
	string strGold = *(o[index++]->vv.s);
	specialGold = stoll(strGold);
	//LogError("gold ----3333333333333error", "%lld", specialGold);
	string strincScore = *(o[index++]->vv.s);
	incScore = stoll(strincScore);
	string strincBean = *(o[index++]->vv.s);
	incBean = stoll(strincBean);
	//LogError("incBean ----3333333333333error", "%lld", incBean);
    experience = o[index++]->vv.i32;
    level = o[index++]->vv.i32;
    expands = *(o[index++]->vv.s);
}

SCisSpecialGoldComsumeRetItem::SCisSpecialGoldComsumeRetItem() : userId(-1), specialGold(0)
{

}

void SCisSpecialGoldComsumeRetItem::WriteToPluto(CPluto& p)
{
//    p << userId << specialGold;
}

// void SCisSpecialGoldComsumeRetItem::ReadFromJson(cJSON* pJsObj)
// {
//     FindJsonItemIntValue(pJsObj, "userId", userId);
//     FindJsonItemInt64Value(pJsObj, "specialGold", specialGold);
// }

void  SCisSpecialGoldComsumeRetItem::ReadFromVObj(T_VECTOR_OBJECT& o, int& index)
{
    userId = o[index++]->vv.i32;
    specialGold = o[index++]->vv.i64;
}

SUserActiveInfo::SUserActiveInfo()
{
    Clear();
}

void SUserActiveInfo::CopyFrom(SUserActiveInfo& src)
{
    fd = src.fd;
    userState = src.userState;
    selScore = src.selScore;
    tableHandle = src.tableHandle;
    chairIndex = src.chairIndex;
    whereFrom = src.whereFrom;
    mac = src.mac;
    robotReadyMs = src.robotReadyMs;
	ip = src.ip;
}

void SUserActiveInfo::Clear()
{
    fd = -1;
    userState = EUS_NONE;
    selScore = -1;
    tableHandle = -1;
    chairIndex = -1;
    whereFrom = 0;
    mac = "";
    lastSitTableHandle = -1;
    jingDu = 0.0;
    weiDu = 0.0;
    yaoyaoTick = 0;
    isAddRobot = false;
    addRobotMs = 0;
    robotReadyMs = 0;
    enterTableTick = 0;
	ip = "";
}
