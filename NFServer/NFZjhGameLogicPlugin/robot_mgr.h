#ifndef __ROBOT__MGR__HEAD__
#define __ROBOT__MGR__HEAD__

#include <stdlib.h>
#include <inttypes.h>
#include <list>
//#include "win32def.h"
#include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
//#include "memory_pool.h"
//#include "json_helper.h"
#include "NFServer/NFGameServerNet_ServerPlugin/cfg_reader.hpp"
using std::list;
#include "type_mogo.h"

class CRobotMgr
{
public:
    CRobotMgr();
    ~CRobotMgr();

public:
	static CRobotMgr* Inst();
	static void FreeInst();

public:
	int ProcUserInfoToDb(uint32_t taskId, int32_t userId, string accessToken, int32_t gameRoomId, int32_t type, int srcFd);
   // void InitCfg();
    void CheckStartReadUserInfo();
    void ReadUserInfoCallback(int retCode, const char*  retMsg, int userIndex, SUserBaseInfo& baseInfo);
    bool IsRobot(int userId);
	NF_SHARE_PTR<SUserInfo> AllocRobotUser(int selScore);
    bool FreeRobotUser(int userId);

    inline bool GetIsInit() const
    {
        return m_isInit;
    }
private:
    void Read1RobotUserInfo(int lastIndex, bool reRead);
    void StartReadRobotUserInfo();
private:
    bool m_isInit;
    bool m_isReading;
    int m_readSuccessCount;
    vector<int> m_allUserId;
    map<int, NF_SHARE_PTR<SUserInfo>> m_allRobot;
    vector<NF_SHARE_PTR<SUserInfo>> m_idleRobot;
    map<int, NF_SHARE_PTR<SUserInfo>> m_gameRobot;
};


#endif
