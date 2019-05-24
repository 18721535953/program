
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
#ifndef NFC_PROXYSERVER_SERVER_MODULE_H
#define NFC_PROXYSERVER_SERVER_MODULE_H

#include "NFComm/NFCore/NFConsistentHash.hpp"
#include "NFComm/NFMessageDefine/NFMsgDefine.h"
#include "NFComm/NFPluginModule/NFIProxyServerNet_ServerModule.h"
#include "NFComm/NFPluginModule/NFIProxyServerToWorldModule.h"
#include "NFComm/NFPluginModule/NFIKernelModule.h"
#include "NFComm/NFPluginModule/NFIClassModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFIElementModule.h"
#include "NFComm/NFPluginModule/NFIProxyServerToGameModule.h"
#include "NFComm/NFPluginModule/NFINetClientModule.h"
#include "NFComm/NFPluginModule/NFISecurityModule.h"
#include "NFComm/NFPluginModule/NFIWebsocketModule.h"
#include "NFComm/NFPluginModule/NFIRequest.h"
#include "NFComm/NFCore/NFUtil.hpp"


// class NFRequestAction : public NFIRequest
// {
// public:
// 	std::string action;
// 	vector<std::string> skipKey;
// };


//AJSON(NFRequestAction,action , skipKey)

class NFCProxyServerNet_ServerModule : public NFIProxyServerNet_ServerModule
{
public:
    NFCProxyServerNet_ServerModule(NFIPluginManager* p)
    {
        pPluginManager = p;
    }

    virtual bool Init();
    virtual bool Shut();
    virtual bool Execute();
	virtual bool ReadyExecute();

	void OnOtherMessageWS(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen);

	void OnJsonMessageWS(websocketpp::connection_hdl nSockIndex,  const char * msg, const uint32_t nLen);

    virtual bool AfterInit();

    virtual int Transpond(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	void OnClientConnectedWS(websocketpp::connection_hdl hdl);

    
    virtual int EnterGameSuccessEvent(const NFGUID xClientID, const NFGUID xPlayerID);

	//game
	virtual int OnlyWsTranspond(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	virtual int	OnClientOtherPlaceResp(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	virtual void OnClientClose(const NFGUID xClientID, const bool delayOff);
protected:

    void OnSocketClientEvent(const NFSOCK nSockIndex, const NF_NET_EVENT eEvent, NFINet* pNet);

	void OnClientDisconnectWS(websocketpp::connection_hdl nAddress);
	void ConClientPing(websocketpp::connection_hdl nAddress);

    void OnClientDisconnect(const NFSOCK nAddress);
	void OnSelectServerProcessWS(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen);
    void OnClientConnected(const NFSOCK nAddress);

	void OnReqRoleListProcessWS(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen);

    void OnConnectKeyProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnSocketClientEventWS(websocketpp::connection_hdl hdl, NF_WS_EVENT event);
    void OnReqServerListProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
    void OnSelectServerProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnReqServerListProcessWS(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen);
    void OnReqRoleListProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnReqCreateRoleProcessWS(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen);
    void OnReqCreateRoleProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
    void OnReqDelRoleProcess(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnReqEnterGameServerWS(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen , NFGUID clientID);
    void OnReqEnterGameServer(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);
	void OnInterceptLoginGameServer(websocketpp::connection_hdl nSockIndex, const int nMsgID, const char * msg, const uint32_t nLen);

	void OnWebSocketReciveTest(websocketpp::connection_hdl, const int nMsgID, const char *, const uint32_t nLen);

    //////////////////////////////////////////////////////////////////////////

	void OnOtherMessage(const NFSOCK nSockIndex, const int nMsgID, const char* msg, const uint32_t nLen);

	//////////////////////////////////////////////////////////////////////////

	void OnNetJsonClientRegisterProcess(const websocketpp::connection_hdl nSocketIndex, const NFGUID nClientGuid, const nlohmann::json& jsonObject);
private:
	typedef std::function<void(const websocketpp::connection_hdl nSocketIndex, const NFGUID nClientGuid, const nlohmann::json& jsonObject)> NET_JSON_MSG_EVENT_FUNC;
	typedef std::shared_ptr<NET_JSON_MSG_EVENT_FUNC> NET_JSON_MSG_EVENT_FUNC_PTR;

	std::map<const int, std::list<NET_JSON_MSG_EVENT_FUNC_PTR>> mxNetJsonWithoutHeadCallBack;

	template<typename BaseType>
	inline void AddNetJsonEventWithoutMsgHead(const int nMsgId, BaseType* pBaseType, void(BaseType::*handleRecieve)(const websocketpp::connection_hdl, const NFGUID, const nlohmann::json&))
	{
		NET_JSON_MSG_EVENT_FUNC functor = std::bind(handleRecieve, pBaseType, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		NET_JSON_MSG_EVENT_FUNC_PTR functionPtr(new NET_JSON_MSG_EVENT_FUNC(functor));

		AddNetJsonEventWithoutMsgHead(nMsgId, functionPtr);
	}
	void AddNetJsonEventWithoutMsgHead(const int nMsgID, const NET_JSON_MSG_EVENT_FUNC_PTR& cb);

	inline bool OnNetJsonWithoutHeadFunc(const int nMsgID, const websocketpp::connection_hdl nSocketIndex, const NFGUID clientID, const nlohmann::json& jsonObject)
	{
		std::map<const int, std::list<NET_JSON_MSG_EVENT_FUNC_PTR>>::iterator it = mxNetJsonWithoutHeadCallBack.find(nMsgID);
		if (it == mxNetJsonWithoutHeadCallBack.end()) return false;

		for (auto functionIter : it->second)
		{
			functionIter->operator()(nSocketIndex, clientID, jsonObject);
		}
		return true;
	}

protected:
	//webSocket
	NFMapEx<NFGUID, websocketpp::connection_hdl> mxClientIdentWS;
    NFMapEx<NFGUID, NFSOCK> mxClientIdent;
protected:
    NFINetClientModule* m_pNetClientModule;
    NFIKernelModule* m_pKernelModule;
    NFILogModule* m_pLogModule;
    NFIElementModule* m_pElementModule;
    NFIClassModule* m_pClassModule;
	NFINetModule* m_pNetModule;
	NFIWebsocketModule * m_pWebSocketModule;
	NFISecurityModule* m_pSecurityModule;
	NFIProxyServerToWorldModule* m_pProxyToWorldModule;

	NFTimer m_pDelayCloseSocketTimer;
	NFList<NFGUID> m_pDelayCloseSocketList;
};

#endif