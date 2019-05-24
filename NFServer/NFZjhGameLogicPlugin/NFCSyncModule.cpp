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


#include "NFCSyncModule.h"
#include "NFComm/NFMessageDefine/NFProtocolDefine.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"


NFCSyncModule::~NFCSyncModule() 
{
	if (m_pWorldGameArea)
	{
		delete m_pWorldGameArea; 
		m_pWorldGameArea = nullptr;
	}
	
};

bool NFCSyncModule::Init()
{
	m_pScheduleModule = pPluginManager->FindModule<NFIScheduleModule>();
	m_pNetModule = pPluginManager->FindModule<NFINetModule>();
	m_pKernelModule = pPluginManager->FindModule<NFIKernelModule>();
	m_pLogicClassModule = pPluginManager->FindModule<NFIClassModule>();
	m_pElementModule = pPluginManager->FindModule<NFIElementModule>();
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();
	m_pSceneAOIModule = pPluginManager->FindModule<NFISceneAOIModule>();
	m_pClassModule = pPluginManager->FindModule<NFIClassModule>();
	
	m_pWorldGameArea = pPluginManager->FindModule<NFIGameLogicModule>();

    return true;
}

bool NFCSyncModule::Shut()
{
    return true;
}

bool NFCSyncModule::Execute()
{
	//m_pWorldGameArea->OnThreadRun();
    return true;
}

bool NFCSyncModule::AfterInit()
{
	m_pScheduleModule->AddSchedule("NFCSyncModule", this, &NFCSyncModule::SyncHeart, 0.1f, -1);

	m_pKernelModule->AddClassCallBack(NFrame::NPC::ThisName(), this, &NFCSyncModule::OnNPCClassEvent);
	m_pKernelModule->AddClassCallBack(NFrame::Player::ThisName(), this, &NFCSyncModule::OnPlayerClassEvent);

	//51
	//m_pGameServerNet_ServerModule->AddNetGameDispFunc(1,NFMsg::MSGID_CLIENT_LOGIN, this, &NFCSyncModule::OnClientLogin);

// 	51
 	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFMsg::MSGID_CLIENT_LOGIN, this,  &NFCSyncModule::OnClientLogin);
// 
// 	//54 MSGID_CLIENT_SIT
 //	m_pGameServerNet_ServerModule->AddNetGameDispFunc(NFMsg::MSGID_CLIENT_SIT, this, &NFCSyncModule::OnClientSit);
// 

    return true;
}

int NFCSyncModule::SyncHeart(const std::string & strHeartName, const float fTime, const int nCount)
{
	//std::cout << strHeartName << " " << fTime << " " << nCount << std::endl;

	return 0;
}

int NFCSyncModule::OnNPCClassEvent(const NFGUID & self, const std::string & strClassName, const CLASS_OBJECT_EVENT eClassEvent, const NFDataList & var)
{
	if (CLASS_OBJECT_EVENT::COE_CREATE_FINISH == eClassEvent)
	{
		m_pKernelModule->AddPropertyCallBack(self, NFrame::NPC::Position(), this, &NFCSyncModule::OnNPCPositionEvent);
	}

	return 0;
}

int NFCSyncModule::OnNPCPositionEvent(const NFGUID & self, const std::string & strPropertyName, const NFData & oldVar, const NFData & newVar)
{
	return 0;
}

int NFCSyncModule::OnPlayerClassEvent(const NFGUID & self, const std::string & strClassName, const CLASS_OBJECT_EVENT eClassEvent, const NFDataList & var)
{
	if (CLASS_OBJECT_EVENT::COE_CREATE_FINISH == eClassEvent)
	{
		m_pKernelModule->AddPropertyCallBack(self, NFrame::Player::Position(), this, &NFCSyncModule::OnPlayePositionPEvent);
	}

	return 0;
}

int NFCSyncModule::OnPlayePositionPEvent(const NFGUID & self, const std::string & strPropertyName, const NFData & oldVar, const NFData & newVar)
{
	return 0;
}


int NFCSyncModule::OnClientLogin(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	std::string token = jsonObject.at("accessToken").get<std::string>();
	std::string mac = jsonObject.at("mac").get<std::string>();
	int whereFrom = jsonObject.at("whereFrom").get<int>();
	int version = jsonObject.at("version").get<int>();
	//NFGamespace::ClientLoginStruct xxx;
	//T_VECTOR_OBJECT xxx;
	//m_pWorldGameArea->ProcClientLogin(xxx, clientID.nData64);
	return 0;
}

//54 MSGID_CLIENT_SIT
int  NFCSyncModule::OnClientSit(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
{
	NFGamespace::ClientSitStruct xobj;
	NFGamespace::from_json(jsonObject, xobj);
	return 0;
}
// 56 //MSGID_CLIENT_READY
// int  NFCSyncModule::OnClientReady(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
// {
// 	NFGamespace::ClientReadyStruct xobj;
// 	NFGamespace::from_json(jsonObject, xobj);
// 	return 0;
// }
// 58 MSGID_CLIENT_G_BET
// int  NFCSyncModule::OnClienGBet(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
// {
// 	NFGamespace::ClientGBetStruct xobj;
// 	NFGamespace::from_json(jsonObject, xobj);
// 	return 0;
// }
