// -------------------------------------------------------------------------
//    @FileName			:    NFCWebsocketModule.cpp
//    @Author           :    Stone.xin<xinxinst@163.com>
//    @Date             :    2016-12-22
//    @Module           :    NFCWebsocketModule
//    @Desc             :
// -------------------------------------------------------------------------

#include "NFCWebsocketModule.h"
#include <common/lexical_cast.hpp>
NFCWebsocketModule::NFCWebsocketModule(NFIPluginManager* p)
{
	pPluginManager = p;
}

bool NFCWebsocketModule::Init()
{
	m_pWSServer = NF_NEW NFCWS(this, &NFCWebsocketModule::OnWebsocketMessage, &NFCWebsocketModule::OnWebsocketEvent , &NFCWebsocketModule::OnCallbalckWs);
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	return true;
}

bool NFCWebsocketModule::AfterInit()
{

	return true;
}

bool NFCWebsocketModule::Shut()
{
	return true;
}

bool NFCWebsocketModule::BeforeShut()
{
	return true;
}

bool NFCWebsocketModule::Execute()
{
	m_pWSServer->Execute();
	return true;
}

void NFCWebsocketModule::AddReceiveCallBack(const int nMsgID, NF_WS_MSG_CALL_BACK_PTR functorPtr)
{
	if (mxReceiveCallBack.find(nMsgID) == mxReceiveCallBack.end())
	{
		std::list<NF_WS_MSG_CALL_BACK_PTR> xList;
		xList.push_back(functorPtr);
		mxReceiveCallBack.insert(std::map<int, std::list<NF_WS_MSG_CALL_BACK_PTR>>::value_type(nMsgID, xList));
		return;
	}

	std::map<int, std::list<NF_WS_MSG_CALL_BACK_PTR>>::iterator it = mxReceiveCallBack.find(nMsgID);
	it->second.push_back(functorPtr);
}

void NFCWebsocketModule::AddReceiveCallBack(NF_WS_MSG_CALL_BACK_PTR functorPtr)
{
	mxCallBackList.push_back(functorPtr);
}

void NFCWebsocketModule::AddRecWsCallBack(NF_WS_CALL_BACK_PTR functorPtr)
{
	mxWsCallBackList.push_back(functorPtr);
}

void NFCWebsocketModule::SetReceiveCallBack(NF_WS_MSG_CALL_BACK_PTR functorPtr)
{
	m_pRecvMsgCB = functorPtr;
}

void NFCWebsocketModule::SetEventCallBack(NF_WS_EVENT_CALL_BACK_PTR functorPtr)
{
	m_pEvtCB = functorPtr;
}

int NFCWebsocketModule::Initialization(const unsigned int nMaxClient, const unsigned short nPort,
	const int nCpuCount /*= 4*/)
{
	return m_pWSServer->Initialization(nMaxClient, nPort, nCpuCount);
}

bool NFCWebsocketModule::SendMsgToAllClient(const char* msg, const uint32_t nLen,
	NF_WS_MSG_DATA_TYPE msg_data_type /*= TEXT*/)
{
	return m_pWSServer->SendMsgToAllClient(msg, nLen, msg_data_type);
}

bool NFCWebsocketModule::SendMsgToClient(const char* msg, const uint32_t nLen,
	const std::vector<websocketpp::connection_hdl>& conn_list,
	NF_WS_MSG_DATA_TYPE msg_data_type /*= TEXT*/)
{
	return m_pWSServer->SendMsgToClient(msg, nLen, conn_list, msg_data_type);
}

bool NFCWebsocketModule::SendMsgToClient(const char* msg, const uint32_t nLen, websocketpp::connection_hdl conn,
	NF_WS_MSG_DATA_TYPE msg_data_type /*= TEXT*/)
{
	return false;
}

bool NFCWebsocketModule::SendMsgToClient(const int nMsgID, const char * msg, const uint32_t nLen, websocketpp::connection_hdl conn, NF_WS_MSG_DATA_TYPE msg_data_type)
{
	return m_pWSServer->SendMsgToClient(msg, nMsgID,nLen, conn, msg_data_type);
}


void NFCWebsocketModule::OnWebsocketMessage(websocketpp::connection_hdl hd, const int nMsgID,const char * strPayload,
	const uint32_t length)
{

	std::map<int, std::list<NF_WS_MSG_CALL_BACK_PTR>>::iterator it = mxReceiveCallBack.find(nMsgID);
	if (mxReceiveCallBack.end() != it)
	{
		std::list<NF_WS_MSG_CALL_BACK_PTR>& xFunList = it->second;
		for (std::list<NF_WS_MSG_CALL_BACK_PTR>::iterator itList = xFunList.begin(); itList != xFunList.end(); ++itList)
		{
			NF_WS_MSG_CALL_BACK_PTR& pFunPtr = *itList;
			NF_WS_MSG_CALL_BACK* pFunc = pFunPtr.get();
			pFunc->operator()(hd, nMsgID,strPayload, length);
		}
	}
	else
	{
		for (std::list<NF_WS_MSG_CALL_BACK_PTR>::iterator itList = mxCallBackList.begin(); itList != mxCallBackList.end(); ++itList)
		{
			NF_WS_MSG_CALL_BACK_PTR& pFunPtr = *itList;
			NF_WS_MSG_CALL_BACK* pFunc = pFunPtr.get();
			pFunc->operator()(hd, nMsgID,strPayload, length);
		}
	}
	//TODO ×¢ƒÔIDÊÂ¼þº¯”µ


}

void NFCWebsocketModule::OnCallbalckWs(websocketpp::connection_hdl hd, const char * strPayload, const uint32_t length)
{
	for (std::list<NF_WS_CALL_BACK_PTR>::iterator itWsList = mxWsCallBackList.begin(); itWsList != mxWsCallBackList.end(); ++itWsList)
	{
		NF_WS_CALL_BACK_PTR& pFunPtr = *itWsList;
		NF_WS_CALL_BACK* pFunc = pFunPtr.get();
		pFunc->operator()(hd, strPayload, length);
	}
}

void NFCWebsocketModule::OnWebsocketEvent(websocketpp::connection_hdl hd, NF_WS_EVENT evt)
{
	if (m_pEvtCB.get())
	{
		m_pEvtCB->operator()(hd, evt);
	}
}

bool NFCWebsocketModule::SendMsgPB(const uint16_t nMsgID, const google::protobuf::Message & xData, websocketpp::connection_hdl conn)
{
	NFMsg::MsgBase xMsg;
	if (!xData.SerializeToString(xMsg.mutable_msg_data()))
	{
		std::ostringstream stream;
		stream << " Failed For Serialize of MsgData, MessageID " << nMsgID;
		m_pLogModule->LogError(stream, __FUNCTION__, __LINE__);

		return false;
	}

	NFMsg::Ident* pPlayerID = xMsg.mutable_player_id();
	*pPlayerID = NFINetModule::NFToPB(NFGUID());

	std::string strMsg;
	if (!xMsg.SerializeToString(&strMsg))
	{
		std::ostringstream stream;
		stream << " Failed For Serialize of MsgBase, MessageID " << nMsgID;
		m_pLogModule->LogError(stream, __FUNCTION__, __LINE__);

		return false;
	}
	m_pWSServer->SendMsgToClient(strMsg.data(), nMsgID,strMsg.length(), conn, NF_WS_MSG_DATA_TYPE::TEXT);
	return true;
}

bool NFCWebsocketModule::SendMsgWithOutHead(const int nMsgID, const std::string & msg, websocketpp::connection_hdl conn)
{
	
	m_pWSServer->SendMsgToClient(msg.data(), nMsgID, msg.length(), conn, NF_WS_MSG_DATA_TYPE::TEXT);

	return false;
}

void NFCWebsocketModule::RemoveReceiveCallBack(const int nMsgID)
{
	auto it = mxReceiveCallBack.find(nMsgID);
	if ( it== mxReceiveCallBack.end())
	{
		return;
	}
	mxReceiveCallBack.erase(it);
}
