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
#include "NFCProxyServerToGameModule.h"
#include "NFProxyServerNet_ClientPlugin.h"
#include "NFComm/NFPluginModule/NFIClassModule.h"
#include "NFComm/NFMessageDefine/NFProtocolDefine.hpp"
#include <json/json.hpp>

bool NFCProxyServerToGameModule::Init()
{
	m_pNetClientModule = pPluginManager->FindModule<NFINetClientModule>();
	m_pKernelModule = pPluginManager->FindModule<NFIKernelModule>();
	m_pProxyServerNet_ServerModule = pPluginManager->FindModule<NFIProxyServerNet_ServerModule>();
	m_pElementModule = pPluginManager->FindModule<NFIElementModule>();
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	m_pClassModule = pPluginManager->FindModule<NFIClassModule>();

    return true;
}

bool NFCProxyServerToGameModule::Shut()
{
    //Final();
    //Clear();
    return true;
}

bool NFCProxyServerToGameModule::Execute()
{
	return true;
}

bool NFCProxyServerToGameModule::AfterInit()
{
	//m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_GAME, NFMsg::EGMI_ACK_ENTER_GAME, this, &NFCProxyServerToGameModule::OnAckEnterGame);
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_GAME, this, &NFCProxyServerToGameModule::Transpond);
	//m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_GAME, NFMsg::EGMI_REQ_CONNECT_KEY, this, );
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_GAME, NFMsg::MSGID_CLIENT_LOGIN_RESP, this, &NFCProxyServerToGameModule::OnLoginResp);
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_GAME, NFMsg::EGMI_ACK_OFFLINE_NOTIFY , this, &NFCProxyServerToGameModule::OnClientOfflineResp);
	m_pNetClientModule->AddReceiveCallBack(NF_SERVER_TYPES::NF_ST_GAME, NFMsg::MSGID_CLIENT_FORCE_LEAVE_NOTIFY, this, &NFCProxyServerToGameModule::OnDelayOfflineProcess);
	m_pNetClientModule->AddEventCallBack(NF_SERVER_TYPES::NF_ST_GAME, this, &NFCProxyServerToGameModule::OnSocketGSEvent);
	m_pNetClientModule->ExpandBufferSize();

    return true;
}

void NFCProxyServerToGameModule::OnSocketGSEvent(const NFSOCK nSockIndex, const NF_NET_EVENT eEvent, NFINet* pNet)
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
        Register(pNet);
    }
}

void NFCProxyServerToGameModule::Register(NFINet* pNet)
{
    NF_SHARE_PTR<NFIClass> xLogicClass = m_pClassModule->GetElement(NFrame::Server::ThisName());
    if (xLogicClass)
    {
		const std::vector<std::string>& strIdList = xLogicClass->GetIDList();
		for (int i = 0; i < strIdList.size(); ++i)
		{
			const std::string& strId = strIdList[i];

            const int nServerType = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::Type());
            const int nServerID = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::ServerID());
            if (nServerType == NF_SERVER_TYPES::NF_ST_PROXY && pPluginManager->GetAppID() == nServerID)
            {
                const int nPort = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::Port());
                const int nMaxConnect = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::MaxOnline());
                //const int nCpus = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::CpuCount());
                const std::string& strName = m_pElementModule->GetPropertyString(strId, NFrame::Server::Name());
                const std::string& strIP = m_pElementModule->GetPropertyString(strId, NFrame::Server::IP());
				const int nGameType = m_pElementModule->GetPropertyInt32(strId, NFrame::Server::GameType());

                NFMsg::ServerInfoReportList xMsg;
                NFMsg::ServerInfoReport* pData = xMsg.add_server_list();

                pData->set_server_id(nServerID);
                pData->set_server_name(strName);
                pData->set_server_cur_count(0);
                pData->set_server_ip(strIP);
                pData->set_server_port(nPort);
                pData->set_server_max_online(nMaxConnect);
                pData->set_server_state(NFMsg::EST_NARMAL);
                pData->set_server_type(nServerType);
				pData->set_game_type(nGameType);

                NF_SHARE_PTR<ConnectData> pServerData = m_pNetClientModule->GetServerNetInfo(pNet);
                if (pServerData)
                {
                    int nTargetID = pServerData->nGameID;
                    m_pNetClientModule->SendToServerByPB(nTargetID, NFMsg::EGameMsgID::EGMI_PTWG_PROXY_REGISTERED, xMsg);

                    m_pLogModule->LogNormal(NFILogModule::NLL_INFO_NORMAL, NFGUID(0, pData->server_id()), pData->server_name(), "Register");
                }
            }
        }
    }
}

void NFCProxyServerToGameModule::OnAckEnterGame(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
{
    NFGUID nPlayerID;
    NFMsg::AckEventResult xData;
    if (!NFINetModule::ReceivePB( nMsgID, msg, nLen, xData, nPlayerID))
    {
        return;
    }
 
	const NFGUID& xClient = NFINetModule::PBToNF(xData.event_client());
	const NFGUID& xPlayer = NFINetModule::PBToNF(xData.event_object());

	m_pProxyServerNet_ServerModule->EnterGameSuccessEvent(xClient, xPlayer);
	m_pProxyServerNet_ServerModule->Transpond(nSockIndex, nMsgID, msg, nLen);
}

void NFCProxyServerToGameModule::LogServerInfo(const std::string& strServerInfo)
{
    m_pLogModule->LogNormal(NFILogModule::NLL_INFO_NORMAL, NFGUID(), strServerInfo, "");
}

void NFCProxyServerToGameModule::Transpond(const NFSOCK nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen)
{
	if (nMsgID < NFMsg::EGameMsgID::EGMI_UNKNOW)
	{
		m_pProxyServerNet_ServerModule->OnlyWsTranspond(nSockIndex, nMsgID, msg, nLen);
		return;
	}
	m_pProxyServerNet_ServerModule->Transpond(nSockIndex, nMsgID, msg, nLen);
}

 void NFCProxyServerToGameModule::OnLoginResp(const NFSOCK nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen)
 {
	 /*NFGUID nPlayerID;
	 NFMsg::AckEventResult xData;
	 if (!NFINetModule::ReceivePB(nMsgID, msg, nLen, xData, nPlayerID))
	 {
		 return;
	 }

	 const NFGUID& xClient = NFINetModule::PBToNF(xData.event_client());*/
 
 	m_pProxyServerNet_ServerModule->OnlyWsTranspond(nSockIndex, nMsgID, msg, nLen);
 
 }

 void NFCProxyServerToGameModule::OnClientOfflineResp(const NFSOCK nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen)
 {
	 CLIENT_MSG_PROCESS_NO_OBJECT(nMsgID, msg, nLen, NFMsg::RoleOfflineNotify);

	 const NFGUID& xClient = NFINetModule::PBToNF(*xMsg.mutable_self());

	 m_pProxyServerNet_ServerModule->OnClientClose(xClient, false);
 }

 void NFCProxyServerToGameModule::OnDelayOfflineProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen)
 {
	 CLIENT_MSG_PROCESS_NO_OBJECT(nMsgID, msg, nLen, NFMsg::RoleDelayOffline);

	 NFGUID xClientId = NFINetModule::PBToNF(*xMsg.mutable_self());

	 m_pProxyServerNet_ServerModule->OnClientClose(xClientId, true);

	 nlohmann::json jsonObject;
	 try
	 {
		 jsonObject["action"] = nMsgID;
		 jsonObject["code"] = xMsg.code();

		 std::string xData = jsonObject.dump();

		 NFMsg::MsgBase xMsgBase;
		 xMsgBase.set_msg_data(xData.data(), xData.length());

		 NFMsg::Ident* pPlayerID = xMsgBase.mutable_player_id();
		 *pPlayerID = NFINetModule::NFToPB(xClientId);

		 std::string strMsg;
		 if (!xMsg.SerializeToString(&strMsg))
		 {
			 std::ostringstream stream;
			 stream << " SendMsgPB Message to  " << nSockIndex;
			 stream << " Failed For Serialize of MsgBase, MessageID " << nMsgID;
			 m_pLogModule->LogError(stream, __FUNCTION__, __LINE__);

			 return;
		 }
		 m_pProxyServerNet_ServerModule->OnlyWsTranspond(nSockIndex, nMsgID, strMsg.data(), strMsg.length());

	 }
	 catch (const nlohmann::detail::exception& ex)
	 {
		 m_pLogModule->LogError(ex.what(), __FUNCTION__, __LINE__);
		 return;
	 }
 }

