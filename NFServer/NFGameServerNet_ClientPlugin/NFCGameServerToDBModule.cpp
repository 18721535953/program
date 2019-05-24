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


#include "NFCGameServerToDBModule.h"
#include "NFGameServerNet_ClientPlugin.h"
#include "NFComm/NFMessageDefine/NFMsgDefine.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFMessageDefine/NFProtocolDefine.hpp"
#include "NFComm/NFMessageDefine/NFProxyStruct.hpp"

bool NFCGameServerToDBModule::Init()
{
	m_pNetClientModule = pPluginManager->FindModule<NFINetClientModule>();
	m_pKernelModule = pPluginManager->FindModule<NFIKernelModule>();
	m_pClassModule = pPluginManager->FindModule<NFIClassModule>();
	m_pElementModule = pPluginManager->FindModule<NFIElementModule>();
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pGameServerNet_ServerModule = pPluginManager->FindModule<NFIGameServerNet_ServerModule>();

	return true;
}

bool NFCGameServerToDBModule::Shut()
{

	return true;
}


bool NFCGameServerToDBModule::Execute()
{
	return true;
}

bool NFCGameServerToDBModule::AfterInit()
{
	//m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_WORLD, this, &NFCGameServerToDBModule::TransPBToProxy);
	//m_pNetClientModule->AddEventCallBack(NF_SERVER_TYPES::NF_ST_WORLD, this, &NFCGameServerToDBModule::OnSocketWSEvent);
	
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_DB, this,  &NFCGameServerToDBModule::TransPBToNodeDb);
	m_pNetClientModule->ExpandBufferSize();

	NF_SHARE_PTR<NFIClass> xLogicClass = m_pClassModule->GetElement(NFrame::Server::ThisName());
	if (xLogicClass)
	{
		const std::vector<std::string>& strIdList = xLogicClass->GetIDList();

		const int nCurAppID = pPluginManager->GetAppID();
		std::vector<std::string>::const_iterator itr =
			std::find_if(strIdList.begin(), strIdList.end(), [&](const std::string& strConfigId)
		{
			return nCurAppID == m_pElementModule->GetPropertyInt32(strConfigId, NFrame::Server::ServerID());
		});

		if (strIdList.end() == itr)
		{
			std::ostringstream strLog;
			strLog << "Cannot find current server, AppID = " << nCurAppID;
			m_pLogModule->LogNormal(NFILogModule::NLL_ERROR_NORMAL, NULL_OBJECT, strLog, __FUNCTION__, __LINE__);
			NFASSERT(-1, "Cannot find current server", __FILE__, __FUNCTION__);
			exit(0);
		}

		const int nCurArea = m_pElementModule->GetPropertyInt32(*itr, NFrame::Server::Area());

		for (int i = 0; i < strIdList.size(); ++i)
		{
			const std::string& strId = strIdList[i];

			const int nServerType = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::Type());
			const int nServerID = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::ServerID());
			const int nServerArea = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::Area());
			if (nServerType == NF_SERVER_TYPES::NF_ST_DB && nCurArea == nServerArea)
			{
				const int nPort = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::Port());
				//const int nMaxConnect = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::MaxOnline());
				//const int nCpus = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::CpuCount());
				const std::string& strName = m_pElementModule->GetPropertyString(strId, NFrame::Server::Name());
				const std::string& strIP = m_pElementModule->GetPropertyString(strId, NFrame::Server::IP());
				const int nGameType = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::GameType());

				ConnectData xServerData;

				xServerData.nGameID = nServerID;
				xServerData.eServerType = (NF_SERVER_TYPES)nServerType;
				xServerData.strIP = strIP;
				xServerData.nPort = nPort;
				xServerData.strName = strName;
				xServerData.nGameType = nGameType;
				m_pNetClientModule->AddServer(xServerData);
			}
		}
	}

	return true;
}

void NFCGameServerToDBModule::OnSocketWSEvent(const NFSOCK nSockIndex, const NF_NET_EVENT eEvent, NFINet* pNet)
{
	if (eEvent & NF_NET_EVENT_EOF)
	{
	}
	else if (eEvent & NF_NET_EVENT_ERROR)
	{
	}
	else if (eEvent & NF_NET_EVENT_TIMEOUT)
	{
	}
	else  if (eEvent & NF_NET_EVENT_CONNECTED)
	{
		m_pLogModule->LogNormal(NFILogModule::NLL_INFO_NORMAL, NFGUID(0, nSockIndex), "NF_NET_EVENT_CONNECTED", "connected success", __FUNCTION__, __LINE__);
	}
}

void NFCGameServerToDBModule::TransPBToProxy(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	m_pNetClientModule->SendBySuitWithOutHead(NF_SERVER_TYPES::NF_ST_DB, nSockIndex, nMsgID, std::string(msg, nLen));

	return;
}


void NFCGameServerToDBModule::TransmitToDB(const int nHashKey, const int nMsgID, const google::protobuf::Message& xData)
{
	m_pNetClientModule->SendSuitByPB(NF_SERVER_TYPES::NF_ST_DB, nHashKey, nMsgID, xData);
}

void NFCGameServerToDBModule::SendDataToDb(const int nMsgID, NFINT32 task, NFINT64 gameRoomId, short sdataLen, const char * strData)
{
	std::cout << "strData:" << strData << std::endl;
	NFGamespace::GameToDbPack xxPack;
	xxPack.taskId = task;
	xxPack.gameId = gameRoomId;
	xxPack.dataLen = sdataLen;
	int len = sizeof(NFGamespace::GameToDbPack) + sdataLen;
	char * buffer = new char[len];
	memset(buffer, 0, len);
	memcpy(buffer, (void*)(&xxPack), sizeof(xxPack));
	memcpy(buffer + sizeof(xxPack), strData, sdataLen);
	std::cout << " head:" << buffer << " len:" << sizeof(buffer) << std::endl;
	std::string strMsg;

	strMsg.clear();
	strMsg.append(buffer, len);

	std::cout << std::to_string(len) << "length:" << (uint32_t)strMsg.length();
	delete[] buffer;
	buffer = nullptr;
	NF_SHARE_PTR<ConnectData> dbData = m_pNetClientModule->GetServerNetInfo(NF_ST_DB);
	if (!dbData || dbData->eState != ConnectDataState::NORMAL)
	{
		// 登录失败
		std::cout << "db服务器维护中"<<std::endl;
		return  ;
	}

	m_pNetClientModule->SendByServerID(100, nMsgID, strMsg);

	                    
}
void NFCGameServerToDBModule::SendReadUserInfo(NFINT32 task, NFINT64 gameRoomId, short sdataLen, const char * strData)
{
	//std::cout << "strData:" << strData << std::endl;
	NFGamespace::GameToDbPack xxPack;
	xxPack.taskId = task;
	xxPack.gameId = gameRoomId;
	xxPack.dataLen = sdataLen;
	int len = sizeof(NFGamespace::GameToDbPack) + sdataLen;
	char * buffer = new char[len];
	memset(buffer, 0, len);
	memcpy(buffer, (void*)(&xxPack), sizeof(xxPack));
	memcpy(buffer+sizeof(xxPack), strData, sdataLen);
	//std::cout << " head:" << buffer << " len:" << sizeof(buffer) << std::endl;
	std::string strMsg;
	
	strMsg.clear();
	strMsg.append(buffer, len);
	
	//std::cout<<std::to_string(len) << "length:" << (uint32_t)strMsg.length();

	m_pNetClientModule->SendByServerID(100,  NFMsg::MSGID_DBMGR_READ_USERINFO_TODB, strMsg);
	delete [] buffer ;
	buffer = nullptr;
}
void NFCGameServerToDBModule::TransPBToNodeDb(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
	m_pLogModule->LogNormal(NFILogModule::NLL_INFO_NORMAL, NFGUID(0, nSockIndex), "NF_TransPBToNodeDb", "recv success", __FUNCTION__, __LINE__);


	std::ostringstream strLog;
	strLog << " NFCGameServerToDBModule::TransPBToNodeDb" << nSockIndex << " msgid = " << std::to_string(nMsgID);
	strLog<< ";msg: = " << msg << ";msg len = " << std::to_string(nLen);
	m_pLogModule->LogInfo(strLog, __FUNCTION__, __LINE__);
	return;
}

void  NFCGameServerToDBModule::SendGameControl(NFINT32 task, NFINT64 gameRoomId)
{
	 
	NFGamespace::GameToDbPack xxPack;
	xxPack.taskId = task;
	xxPack.gameId = gameRoomId;
	xxPack.dataLen = 0;
	int len = sizeof(NFGamespace::GameToDbPack);
	char * buffer = new char[len];
	memset(buffer, 0, len);
	memcpy(buffer, (void*)(&xxPack), sizeof(xxPack));

	std::cout << " head:" << buffer << " len:" << sizeof(buffer) << std::endl;
	std::string strMsg;

	strMsg.clear();
	strMsg.append(buffer, len);

	std::cout << std::to_string(len) << "length:" << (uint32_t)strMsg.length();

	m_pNetClientModule->SendByServerID(100, NFMsg::MSGID_DBMGR_READ_USERINFO_TODB, strMsg);
	delete[] buffer;
	buffer = nullptr;
	return;
}