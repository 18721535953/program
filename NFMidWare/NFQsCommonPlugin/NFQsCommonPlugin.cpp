#include "NFQsCommonPlugin.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFCRobotModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"
#include "NFCConfigAreaModule.h"
#include "NFComm/NFPluginModule/NFIQsTaskScheduleModule.h"
#include "NFCQsTaskScheduleModule.h"

#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
	CREATE_PLUGIN(pm, NFQsCommonPlugin)

};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
	DESTROY_PLUGIN(pm, NFQsCommonPlugin)
};

#endif
////////////////////////////////////////////////////

const int NFQsCommonPlugin::GetPluginVersion()
{
	return 0;
}

const std::string NFQsCommonPlugin::GetPluginName()
{
	return GET_CLASS_NAME(NFQsCommonPlugin);
}

void NFQsCommonPlugin::Install()
{
		REGISTER_MODULE(pPluginManager, NFIRobotModule, NFCRobotModule)
		REGISTER_MODULE(pPluginManager, NFIConfigAreaModule, NFCConfigAreaModule)
		REGISTER_MODULE(pPluginManager, NFIQsTaskScheduleModule, NFCQsTaskScheduleModule)
}

void NFQsCommonPlugin::Uninstall()
{
		UNREGISTER_MODULE(pPluginManager, NFIRobotModule, NFCRobotModule)
		UNREGISTER_MODULE(pPluginManager, NFIConfigAreaModule, NFCConfigAreaModule)
		UNREGISTER_MODULE(pPluginManager, NFIQsTaskScheduleModule, NFCQsTaskScheduleModule)
}
