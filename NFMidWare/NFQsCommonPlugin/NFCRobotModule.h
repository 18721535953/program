#ifndef NFC_ROBOT_MODULE_H
#define NFC_ROBOT_MODULE_H
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFComm/NFPluginModule/NFIClassModule.h"
#include "NFComm/NFPluginModule/NFIElementModule.h"
#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"
#include "NFComm/NFCore/NFUtil.hpp"
#include "NFCQsTaskScheduleModule.h"
#define READ_ROBOT_USER_COUNT 5

class CCfgReader;
class SUserBaseInfo;

class NFCRobotModule
    :public NFIRobotModule
{
public:
    NFCRobotModule(NFIPluginManager* p);

    virtual ~NFCRobotModule();

    virtual bool Awake();

    virtual bool Init();

    virtual bool AfterInit();

    virtual bool Execute();
    /////////////////////////////////////////////

    virtual bool IsRobot(int userId);

    virtual bool FreeRobotUser(int userId);

    virtual NF_SHARE_PTR<SUserInfo> AllocRobotUser(int selScore);

    virtual void ReadUserInfoCallback(int retCode, const char*  retMsg, int userIndex, NFGamespace::DB_UserBaseInfo& baseInfo);

    virtual bool GetIsInit() const;

	virtual void InitCfg(NFGamespace::QS_GameServer_Type gameType) override;

    void CheckStartReadUserInfo();

private:
    void Read1RobotUserInfo(int lastIndex, bool reRead);
    void StartReadRobotUserInfo();
private:
    bool m_isInit;
    bool m_isReading;
    int m_readSuccessCount;
    NFTimer m_checkReadRobotUserInfoTime;             //定时检测是否需要读取机器人信息
    vector<int> m_allUserId;
    map<int, NF_SHARE_PTR<SUserInfo>> m_allRobot;
    vector<NF_SHARE_PTR<SUserInfo>> m_idleRobot;
    std::map<int, NF_SHARE_PTR<SUserInfo>> m_gameRobot;

private:
    NFILogModule * m_pLogModule;
	NFIClassModule * m_pClassModule;
	NFIElementModule * m_pElementModule;
    NFIConfigAreaModule * m_pConfigAreaModule;
    NFIGameLogicModule * m_pGameLogicModule;
	NFIGameServerToDBModule * m_pNFIGameServerToDBModule;
	NFIQsTaskScheduleModule * m_pQsTaskScheduleModule;
};
#endif
