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

#ifndef NFI_GAMESERVERNET_SERVERMODULE_H
#define NFI_GAMESERVERNET_SERVERMODULE_H

#include <iostream>
#include "NFIModule.h"
#include "NFINetModule.h"
#include "NFComm/NFPluginModule/NFIRequest.h"

class NFIGameServerNet_ServerModule
    : public NFIModule
{
public:

    struct GateBaseInfo
    {
		enum ERoleStatus
		{
			E_LOADING = 1,
			E_LOADED = 2,
		};

        GateBaseInfo()
        {
            nActorID = 0;
            nGateID = 0;
			eStatus = E_LOADING;
        }

        GateBaseInfo(const int gateID, const NFGUID xIdent)
        {
            nActorID = 0;
            nGateID = gateID;
            xClientID = xIdent;
        }

        int nActorID;
		int nGateID;
		ERoleStatus eStatus;
        NFGUID xClientID;
    };

    struct GateServerInfo
    {
        ServerData xServerData;

        std::map<NFGUID, NFSOCK> xRoleInfo;
    };

protected:
	typedef std::function<int(const NFSOCK nSockIndex, const NFGUID clientID, const nlohmann::json& jsonObject)> NET_GAME_DESP_FUNC;
	typedef std::shared_ptr<NET_GAME_DESP_FUNC> NET_GAME_DESP_FUNC_PTR;

public:
	virtual void SendMsgPBToGate(const uint16_t nMsgID, google::protobuf::Message& xMsg, const NFGUID& self) = 0;
	virtual void SendMsgToGate(const uint16_t nMsgID, const std::string& strMsg, const NFGUID& self) = 0;

    virtual bool AddPlayerGateInfo(const NFGUID& nRoleID, const NFGUID& nClientID, const int nGateID) = 0;
    virtual bool RemovePlayerGateInfo(const NFGUID& nRoleID) = 0;
    virtual NF_SHARE_PTR<GateBaseInfo> GetPlayerGateInfo(const NFGUID& nRoleID) = 0;   
	virtual NF_SHARE_PTR<GateServerInfo> GetGateServerInfo(const int nGateID) = 0;
	virtual NF_SHARE_PTR<GateServerInfo> GetGateServerInfoBySockIndex(const NFSOCK nSockIndex) = 0;
	
	// 分发不同GameServer的msg
	template<typename BaseType>
	inline int AddNetGameDispFunc(const int nMsgID, BaseType* pBase, int (BaseType::*handler)(const NFSOCK, const NFGUID clientID, const nlohmann::json& jsonObject))
	{
		NET_GAME_DESP_FUNC functor = std::bind(handler, pBase, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		NET_GAME_DESP_FUNC_PTR functionPtr(new NET_GAME_DESP_FUNC(functor));

		return AddNetGameDispFunc(nMsgID, functionPtr);
	}

	// 分发不同GameServer的msg
	template<typename BaseType>
	inline int AddNetGameDispFunc(const int nGameType ,const int nMsgID, BaseType* pBase, int (BaseType::*handler)(const NFSOCK, const NFGUID clientID, const nlohmann::json& jsonObject))
	{
		NET_GAME_DESP_FUNC functor = std::bind(handler, pBase, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		NET_GAME_DESP_FUNC_PTR functionPtr(new NET_GAME_DESP_FUNC(functor));

		return AddNetGameDispFunc(nGameType, nMsgID, functionPtr);
	}

	virtual int AddNetGameDispFunc(const int nMsgID, const NET_GAME_DESP_FUNC_PTR& cb) = 0;
	virtual int AddNetGameDispFunc(const int nGameType,const int nMsgID, const NET_GAME_DESP_FUNC_PTR& cb) = 0;

	virtual int GetGameType() = 0;
};

#endif