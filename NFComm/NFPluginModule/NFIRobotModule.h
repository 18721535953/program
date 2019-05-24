#ifndef NFI_ROBOT_MODULE_H
#define NFI_ROBOT_MODULE_H
#include "NFIModule.h"
#include "NFMidWare/NFQsCommonPlugin/NFQsCommonConfig.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
struct SUserBaseInfo;
struct SUserInfo;

class NFIRobotModule :public NFIModule
{
public:
	virtual void InitCfg(NFGamespace::QS_GameServer_Type gameType) = 0;

    virtual bool GetIsInit() const = 0;

    virtual bool FreeRobotUser(int userId) = 0;

    virtual NF_SHARE_PTR<SUserInfo> AllocRobotUser(int selScore) = 0;

    virtual void ReadUserInfoCallback(int retCode, const char*  retMsg, int userIndex, NFGamespace::DB_UserBaseInfo& baseInfo) = 0;

    virtual bool IsRobot(int userId) = 0;
};

#endif
