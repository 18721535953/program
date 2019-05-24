/*
            This file is part of: 
                NoahFrame
            https://github.com/ketoo/NoahGameFrame

   Copyright 2009 - 2018 NoahFrame(NoahGameFrame)

   File creator: lvsheng.huang
   
   NoahFrame is open-source software and you can redistribute it and/or modify
   it under the terms of the License; besides, anyone who use this file/software must include this copyright announcement.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "NFGameLogicPlugin.h"
#include "NFCBuffModule.h"
// #include "NFCGamePVPModule.h"
// #include "NFCMapModule.h"
// #include "NFCCreateRoleModule.h"
// #include "NFCTileModule.h"
// #include "NFCSurvivalModule.h"
// #include "NFCTileMapModule.h"
// #include "NFCHomeModule.h"
#include "NFCSyncModule.h"
#include "NFCGameLogicModule.h"

#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin( NFIPluginManager* pm )
{
    CREATE_PLUGIN( pm, NFZjhGameLogicPlugin )

};

NF_EXPORT void DllStopPlugin( NFIPluginManager* pm )
{
    DESTROY_PLUGIN( pm, NFZjhGameLogicPlugin )
};

#endif
//////////////////////////////////////////////////////////////////////////

const int NFZjhGameLogicPlugin::GetPluginVersion()
{
    return 0;
}

const std::string NFZjhGameLogicPlugin::GetPluginName()
{
	return GET_CLASS_NAME(NFZjhGameLogicPlugin);
}

void NFZjhGameLogicPlugin::Install()
{
	REGISTER_MODULE(pPluginManager, NFIBuffModule, NFCBuffModule)
	REGISTER_MODULE(pPluginManager, NFITableManagerModule, NFCTableManagerModule)
		//REGISTER_MODULE(pPluginManager, NFIGamePVPModule, NFCGamePVPModule)
		REGISTER_MODULE(pPluginManager, NFISyncModule, NFCSyncModule)
		//REGISTER_MODULE(pPluginManager, NFIMapModule, NFCMapModule);
		//REGISTER_MODULE(pPluginManager, NFITileModule, NFCTileModule);
		//REGISTER_MODULE(pPluginManager, NFISurvivalModule, NFCSurvivalModule);
		//REGISTER_MODULE(pPluginManager, NFITileMapModule, NFCTileMapModule);
		//REGISTER_MODULE(pPluginManager, NFIHomeModule, NFCHomeModule);
		REGISTER_MODULE(pPluginManager, NFIGameLogicModule, NFCGameLogicModule);
}

void NFZjhGameLogicPlugin::Uninstall()
{
	UNREGISTER_MODULE(pPluginManager, NFITableManagerModule, NFCTableManagerModule)
	//UNREGISTER_MODULE(pPluginManager, NFITileMapModule, NFCTileMapModule);
	//UNREGISTER_MODULE(pPluginManager, NFISurvivalModule, NFCSurvivalModule);
	//UNREGISTER_MODULE(pPluginManager, NFITileModule, NFCTileModule);
	//UNREGISTER_MODULE(pPluginManager, NFIMapModule, NFCMapModule);
	UNREGISTER_MODULE(pPluginManager, NFISyncModule, NFCSyncModule)
	//UNREGISTER_MODULE(pPluginManager, NFIGamePVPModule, NFCGamePVPModule)
    //UNREGISTER_MODULE(pPluginManager, NFICreateRoleModule, NFCCreateRoleModule)
    UNREGISTER_MODULE(pPluginManager, NFIBuffModule, NFCBuffModule)
	UNREGISTER_MODULE(pPluginManager, NFIGameLogicModule, NFCGameLogicModule);

}
