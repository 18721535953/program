// -------------------------------------------------------------------------
//    @FileName			:		NFCWS.cpp
//    @Author			:		Stone.xin
//    @Date				:		2016-12-22
//    @Module			:		NFCWS
// -------------------------------------------------------------------------

#include "NFCWS.h"
#include <string.h>
#include <atomic>

bool NFCWS::Execute()
{
    if (mbWorking)
    {
        ExecuteClose();
        m_EndPoint.poll_one();
    }
    return true;
}

int NFCWS::Initialization(const unsigned int nMaxClient, const unsigned short nPort, const int nCpuCount)
{
    if (mbWorking)
    {
        return 1;
    }
	std::cout << "连接--->" << nPort << std::endl;

    mnPort = nPort;
    mnMaxConnect = nMaxClient;
    mnCpuCount = nCpuCount;

    m_EndPoint.init_asio();

    m_EndPoint.set_message_handler(std::bind(
            &NFCWS::OnMessageHandler, this,
            std::placeholders::_1, std::placeholders::_2
    ));

    m_EndPoint.set_open_handler(std::bind(
            &NFCWS::OnOpenHandler, this,
            std::placeholders::_1));

    m_EndPoint.set_close_handler(std::bind(
            &NFCWS::OnCloseHandler, this,
            std::placeholders::_1));

    m_EndPoint.set_fail_handler(std::bind(
            &NFCWS::OnFailHandler, this,
            std::placeholders::_1));

    m_EndPoint.set_pong_handler(std::bind(
            &NFCWS::OnPongHandler, this,
            std::placeholders::_1, std::placeholders::_2));

    m_EndPoint.set_interrupt_handler(std::bind(
            &NFCWS::OnInterruptHandler, this,
            std::placeholders::_1));

    m_EndPoint.set_pong_timeout_handler(std::bind(
            &NFCWS::OnPongTimeOutHandler, this,
            std::placeholders::_1, std::placeholders::_2));


    m_EndPoint.listen(nPort);
    m_EndPoint.start_accept();

    mbWorking = true;

    return 0;
}

bool NFCWS::Final()
{
    CloseSocketAll();
    m_EndPoint.stop_listening();

    return true;
}

bool NFCWS::SendMsgToAllClient(const char* msg, const uint32_t nLen, NF_WS_MSG_DATA_TYPE msg_data_type /* = TEXT */)
{
    if (nLen <= 0)
    {
        return false;
    }

    session_list::iterator it = mmObject.begin();
    while (it != mmObject.end())
    {
        WSObjectPtr pWSObject = it->second;
        if (pWSObject && !pWSObject->NeedRemove())
        {
            try
            {
                m_EndPoint.send(it->first, msg, nLen, (websocketpp::frame::opcode::value) msg_data_type);
            }
            catch (websocketpp::exception& e)
            {
                std::cout << "websocket exception: " << e.what() << " this conn will be removed." << std::endl;
                RemoveConnection(it->first, NF_WS_EVENT_CLOSE);
            }
        }
        it++;
    }

    return true;
}

bool
NFCWS::SendMsgToClient(const char* msg, const uint32_t nLen, const std::vector<websocketpp::connection_hdl>& conn_list,
                       NF_WS_MSG_DATA_TYPE msg_data_type /* = TEXT */)
{
    for (auto vIt: conn_list)
    {
        auto pWSObject = GetNetObject(vIt);
        if (pWSObject && !pWSObject->NeedRemove())
        {
            try
            {
                m_EndPoint.send(vIt, msg, nLen, (websocketpp::frame::opcode::value) msg_data_type);
                return true;
            }
            catch (websocketpp::exception& e)
            {
                std::cout << "websocket exception: " << e.what() << " this conn will be removed." << std::endl;
                RemoveConnection(vIt, NF_WS_EVENT_CLOSE);
            }
        }
    }
    return false;
}

bool NFCWS::SendMsgToClient(const char* msg, const int nMsgID,const uint32_t nLen, websocketpp::connection_hdl hd,
                            NF_WS_MSG_DATA_TYPE msg_data_type /* = TEXT */)
{

    auto pWSObject = GetNetObject(hd);
    if (pWSObject && !pWSObject->NeedRemove() ||true)
    {
        try
        {
			std::string strOutData(msg, nLen);
			//int len = EnCode(nMsgID, msg, nLen, strOutData);
            m_EndPoint.send(hd, msg, nLen, (websocketpp::frame::opcode::value) msg_data_type);
            return true;
        }
        catch (websocketpp::exception& e)
        {
            std::cout << "websocket exception: " << e.what() << " this conn will be removed." << std::endl;
            RemoveConnection(hd, NF_WS_EVENT_CLOSE);
        }
    }
    return false;
}

// bool NFCWS::SendWsMsgToClient(const char* msg, const int nMsgID, const uint32_t nLen, websocketpp::connection_hdl hd,
// 	NF_WS_MSG_DATA_TYPE msg_data_type /* = TEXT */)
// {
// 
// 	auto pWSObject = GetNetObject(hd);
// 	if (pWSObject && !pWSObject->NeedRemove() || true)
// 	{
// 		try
// 		{
// 			std::string strOutData;
// 			//int len = EnCode(nMsgID, msg, nLen, strOutData);
// 	
// 			m_EndPoint.send(hd, strOutData.c_str(), nLen, (websocketpp::frame::opcode::value) msg_data_type);
// 			return true;
// 		}
// 		catch (websocketpp::exception& e)
// 		{
// 			std::cout << "websocket exception: " << e.what() << " this conn will be removed." << std::endl;
// 			RemoveConnection(hd, NF_WS_EVENT_CLOSE);
// 		}
// 	}
// 	return false;
// }

bool NFCWS::AddNetObject(websocketpp::connection_hdl hd, WSObjectPtr pWSObject)
{
    auto pObject = GetNetObject(hd);
    if (pObject)
    {
        return false;
    }
    mmObject.emplace(session_list::value_type(hd, pWSObject));
    return true;
}

WSObjectPtr NFCWS::GetNetObject(websocketpp::connection_hdl hd)
{
    session_list::iterator it = mmObject.find(hd);
    if (it == mmObject.end())
    {
        return nullptr;
    }
    return it->second;
}

void NFCWS::ExecuteClose()
{
    for (auto vIt : mvRemoveObject)
    {
        CloseObject(vIt);
    }
    mvRemoveObject.clear();
}

bool NFCWS::CloseSocketAll()
{
    session_list::iterator it = mmObject.begin();
    while (it != mmObject.end())
    {
        mvRemoveObject.push_back(it->first);
        it++;
    }

    ExecuteClose();

    mmObject.clear();

    return true;
}

void NFCWS::CloseObject(websocketpp::connection_hdl hd, NF_WS_CLOSE_CODE nCloseCode/* =CLOSE_CODE_NORMAL */,
                        const std::string& strCloseReason/* ="" */)
{
	try {
		m_EndPoint.close(hd, nCloseCode, strCloseReason);

	}
	catch (exception e)
	{

	}
}

void NFCWS::OnMessageHandler(websocketpp::connection_hdl hd, NFWebSockConf::message_ptr msg)
{
	if (msg->get_payload().compare("ping")==0) {
		m_EndPoint.send(hd, "pong", 4, (websocketpp::frame::opcode::value) TEXT);
		return;
	}
    auto pObject = GetNetObject(hd);
    if (!pObject)
    {
        return;
    }
	pObject->AddBuff(msg->get_payload().c_str(), msg->get_payload().length());

	// //解析
	
	// //当前消息被正确处理，否则循环处理

		//    int len = pObject->GetBuffLen();
		int len = msg->get_payload().length();
		std::cout << "msg len:" << len << " ";
		if (len > NFIMsgHead::NF_Head::NF_HEAD_LENGTH)
		{
			NFCMsgHead xHead;
			int nMsgBodyLength = DeCode(pObject->GetBuff(), len, xHead);
			std::cout << "shou dao de xiaoxi ID " << xHead.GetMsgID() << "----->" << nMsgBodyLength << std::endl;
			if (nMsgBodyLength >= 0 && xHead.GetMsgID() > 0)
			{
				pObject->RemoveBuff(0, NFIMsgHead::NF_Head::NF_HEAD_LENGTH);
				auto t = pObject->GetBuff();
				pObject->RemoveBuff(NFIMsgHead::NF_Head::NF_HEAD_LENGTH, len);
				if (mRecvMsgPack)
				{
					mRecvMsgPack(hd, xHead.GetMsgID(), t, nMsgBodyLength);
				}
			}

		}
	
	std::cout << "shou dao de xiaoxi: " << msg->get_payload() << std::endl;
	if (mRecvWsPack)
	{
		mRecvWsPack(hd, msg->get_payload().c_str(), len );

		//mRecvCB(hd, xHead.GetMsgID(), pObject->GetBuff() + NFIMsgHead::NF_Head::NF_HEAD_LENGTH, len - 6);
	}
}

void NFCWS::OnOpenHandler(websocketpp::connection_hdl hd)
{
	std::cout << "NFCWS open....." << std::endl;
    WSObjectPtr pWSObject(NF_NEW(WSObject));
    if (AddNetObject(hd, pWSObject))
    {
        if (mEventCB)
        {
            mEventCB(hd, NF_WS_EVENT_OPEN);

        }
		if (mRecvMsgPack)
		{
			//ReqAccountLogin xx;
			//120 EGMI_REQ_CONNECT_KEY
			NFMsg::ReqAccountLogin xReqAccMsg;
			std::string strReqMsg;
			if (!xReqAccMsg.SerializeToString(&strReqMsg))
			{
				std::cout << "NFCWS EGMI_REQ_CONNECT_KEY is null....." << std::endl;
				return;
			}


			mRecvMsgPack(hd, NFMsg::EGMI_REQ_CONNECT_KEY, "", 0);
			//EGMI_REQ_SELECT_SERVER

			
			/*NFMsg::ReqSelectServer xMsg;
			xMsg.set_world_id(-1);
			std::string strMsg;
			if (!xMsg.SerializeToString(&strMsg))
			{
				std::cout << "NFCWS EGMI_REQ_SELECT_SERVER is null....." << std::endl;
				return;
			}
			mRecvMsgPack(hd, NFMsg::EGMI_REQ_SELECT_SERVER, strMsg.c_str(), sizeof(xMsg));*/
		}
    }
}

bool NFCWS::RemoveConnection(websocketpp::connection_hdl hd, NF_WS_EVENT evt, NF_WS_CLOSE_CODE nCloseCode /* = 1000 */,
                             const std::string& strCloseReason /* = "" */)
{
	try {
		std::cout << "NFCWS RemoveConnection....." << std::endl;
		session_list::iterator it = mmObject.find(hd);
		if (it != mmObject.end())
		{
			mvRemoveObject.push_back(hd);
			return true;
		}
	}

	catch (exception e) {

	}

    return false;
}

void NFCWS::OnCloseHandler(websocketpp::connection_hdl hd)
{
	std::cout << "NFCWS close....." << std::endl;
    RemoveConnection(hd, NF_WS_EVENT_CLOSE);
}

void NFCWS::OnFailHandler(websocketpp::connection_hdl hd)
{
	std::cout << "NFCWS Fail....." << std::endl;

    RemoveConnection(hd, NF_WS_EVENT_FAIL);
}

void NFCWS::OnInterruptHandler(websocketpp::connection_hdl hd)
{
	std::cout << "NFCWS OnInterruptHandler....." << std::endl;
    RemoveConnection(hd, NF_WS_EVENT_INTERRUPT);
}

void NFCWS::OnPongHandler(websocketpp::connection_hdl hd, std::string str)
{
	std::cout << "NFCWS OnPongHandler....." << std::endl;
}

void NFCWS::OnPongTimeOutHandler(websocketpp::connection_hdl hd, std::string str)
{
	std::cout << "NFCWS OnPongHandler....." << std::endl;
    RemoveConnection(hd, NF_WS_EVENT_PONG_TIMEOUT);
}
