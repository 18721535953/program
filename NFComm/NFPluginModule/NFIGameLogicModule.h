#ifndef NFI_GAME_LOGIC_MODULE_H
#define NFI_GAME_LOGIC_MODULE_H
#include "NFIModule.h"
#include "../../Dependencies/json/json.hpp"

struct SUserBaseInfo;
struct SUserInfo;
struct SArrangeUserList;
class NFGameTable;
class NFIItemFillProcessModule;
class NFBaccaratExpendLogic;


class NFIGameLogicModule :public NFIModule
{
public:
    virtual void GameMsgSenderUnlock(int tableHandle, int userId, int roomId) = 0;

    virtual void GameMsgSenderLock(int tableHandle, int userId, int roomId) = 0;

    virtual void GameMsgSenderLockOrUnLock(uint32_t taskId, int32_t gameRoomId, int32_t userId, int32_t gameServerId, int32_t type) = 0;

    virtual void GameMsgSenderCheckerOpenDoor(int gameRoomId, SUserBaseInfo& baseInfo, int& retCode, std::string & retError) = 0;

    virtual int ClientLoginResponse(const NFGUID& clientID, const int32_t retCode, std::string retErrorMsg) = 0;

    virtual void ClientUpdateUserInfoResponse(const NFGUID& clientFd, const int32_t retCode, const char* retErrorMsg) = 0;

    virtual void AddUserToLeaveTableList(const int userId) = 0;

    virtual void DelUserFromLeaveTableList(const int userId) = 0;

    virtual void ProcUserInfoToDb(uint32_t taskId, int32_t userId, std::string accessToken, int32_t gameRoomId, int32_t gameLock, int srcFd) = 0;

    ///////////////////////////////////////////////////////////

	virtual inline bool FindUserByGuid(const NFGUID & guid) = 0;

	virtual inline void AddUserToGuid(NFGUID guid, NF_SHARE_PTR<SUserInfo> & userInfo) = 0;

	virtual inline NF_SHARE_PTR<SUserInfo> GetUserInfoByGuid(const NFGUID & guid) = 0;

	virtual inline void RemoveUserByGuid(const NFGUID& guid) = 0;

	virtual inline bool FindUserByUserId(int userId) = 0;

	virtual inline void AddUserToUserId(int userId, NF_SHARE_PTR<SUserInfo> & userInfo) = 0;

	virtual inline NF_SHARE_PTR<SUserInfo> GetUserInfoByUserId(int userId) = 0;

	virtual inline void RemoveUserByUserId(const int userId) = 0;

	virtual inline void SendMsgToClient(const uint16_t nMsgId, const nlohmann::json & jsonObject, const NFGUID& clientGuid) = 0;

	virtual inline bool OnCloseed(const NFGUID & guid) = 0;

	virtual int GetYaoyaoUserList(std::map<int, SArrangeUserList*>& retMap) = 0;
	virtual int GetYaoyaoUserListIgnoreSelScore(SArrangeUserList& retList) = 0;

public:
	// 百家乐扩展逻辑接口
	NF_SHARE_PTR<NFBaccaratExpendLogic> m_pNFBaccaratExpendLogic;

};

#endif
