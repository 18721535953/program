/*
    Copyright 2009 - 2018 LvSheng.Huang

   This source file is part of NoahGameFrame/NoahFrame.
   NoahGameFrame/NoahFrame is open-source software and you can redistribute it and/or modify
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
#include "NFNetPlugin.h"
#include "NFCNetModule.h"
#include "NFCNetClientModule.h"
#include "NFCHttpClientModule.h"
#include "NFCHttpServerModule.h"
#include "NFCWebsocketModule.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

#ifdef NF_DYNAMIC_PLUGIN

NF_EXPORT void DllStartPlugin(NFIPluginManager* pm)
{
    CREATE_PLUGIN(pm, NFNetPlugin)

};

NF_EXPORT void DllStopPlugin(NFIPluginManager* pm)
{
    DESTROY_PLUGIN(pm, NFNetPlugin)
};

#endif


//////////////////////////////////////////////////////////////////////////

const int NFNetPlugin::GetPluginVersion()
{
    return 0;
}

const std::string NFNetPlugin::GetPluginName()
{
	return GET_CLASS_NAME(NFNetPlugin);
}

void NFNetPlugin::Install()
{
    REGISTER_MODULE(pPluginManager, NFINetModule, NFCNetModule)
	REGISTER_MODULE(pPluginManager, NFIWebsocketModule, NFCWebsocketModule)
    REGISTER_MODULE(pPluginManager, NFINetClientModule, NFCNetClientModule)
	REGISTER_MODULE(pPluginManager, NFIHttpServerModule, NFCHttpServerModule)
	REGISTER_MODULE(pPluginManager, NFIHttpClientModule, NFCHttpClientModule)
}

void NFNetPlugin::Uninstall()
{
	UNREGISTER_MODULE(pPluginManager, NFIHttpClientModule, NFCHttpClientModule)
    UNREGISTER_MODULE(pPluginManager, NFINetClientModule, NFCNetClientModule)
    UNREGISTER_MODULE(pPluginManager, NFINetModule, NFCNetModule)
	UNREGISTER_MODULE(pPluginManager, NFIHttpServerModule, NFCHttpServerModule)
}