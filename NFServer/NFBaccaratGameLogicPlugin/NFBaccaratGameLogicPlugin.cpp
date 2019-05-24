
#ifndef NFC_BACCARATGAMELOGICPLUGIN_CPP
#define NFC_BACCARATGAMELOGICPLUGIN_CPP


#include "NFBaccaratGameLogicPlugin.h"
#include "NFComm/NFPluginModule/NFITableManagerModule.h"
#include "NFTableModule/NFCTableManagerModule.h"
#include "NFComm/NFPluginModule/NFIRobotModule.h"
#include "NFComm/NFPluginModule/NFIGameLogicModule.h"
#include "NFGameLogicModule/NFCGameLogicModule.h"
#include "NFComm/NFPluginModule/NFIControlCardModule.h"
#include "NFGameLogicModule/NFCControlCardModule.h"
#include "NFComm/NFPluginModule/NFIConfigAreaModule.h"

#include "NFComm/NFPluginModule/NFIGameServerNet_ClientModule.h"
//#include "NFCGameServerNet_ClientModule.h"

#include "NFComm/NFPluginModule/NFIGameServerToDBModule.h"
#include "../NFGameServerNet_ClientPlugin/NFCGameServerToDBModule.h"
//
//
#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
	CREATE_PLUGIN(pm, NFBaccaratGameLogicPlugin)

};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
	DESTROY_PLUGIN(pm, NFBaccaratGameLogicPlugin)
};

#endif
////////////////////////////////////////////////////

const int NFBaccaratGameLogicPlugin::GetPluginVersion()
{
	return 0;
}

const std::string NFBaccaratGameLogicPlugin::GetPluginName()
{
	return GET_CLASS_NAME(NFBaccaratGameLogicPlugin);
}

void NFBaccaratGameLogicPlugin::Install()
{
	REGISTER_MODULE(pPluginManager, NFITableManagerModule, NFCTableManagerModule)
	REGISTER_MODULE(pPluginManager, NFIGameLogicModule, NFCGameLogicModule)
	REGISTER_MODULE(pPluginManager, NFIControlCardModule, NFCControlCardModule)
//	REGISTER_MODULE(pPluginManager, NFINetCommonModule, NFCNetCommonModule)

//	REGISTER_MODULE(pPluginManager, NFIGameServerNet_ClientModule, NFCGameServerNet_ClientModule)
//	REGISTER_MODULE(pPluginManager, NFIGameServerToDBModule, NFCGameServerToDBModule)

///REGISTER_MODULE(pPluginManager, NFIQsCommonP, NFCNetCommonModule)
	
}

void NFBaccaratGameLogicPlugin::Uninstall()
{
	UNREGISTER_MODULE(pPluginManager, NFITableManagerModule, NFCTableManagerModule)
	UNREGISTER_MODULE(pPluginManager, NFIGameLogicModule, NFCGameLogicModule)
	UNREGISTER_MODULE(pPluginManager, NFIControlCardModule, NFCControlCardModule)
//	UNREGISTER_MODULE(pPluginManager, NFINetCommonModule, NFCNetCommonModule)

//	UNREGISTER_MODULE(pPluginManager, NFIGameServerToDBModule, NFCGameServerToDBModule)
}
#endif