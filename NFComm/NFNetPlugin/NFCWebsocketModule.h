// -------------------------------------------------------------------------
//    @FileName			:    NFCWebsocketModule.h
//    @Author           :    Stone.xin<xinxinst@163.com>
//    @Date             :    2016-12-22
//    @Module           :    NFCWebsocketModule
//    @Desc             :
// -------------------------------------------------------------------------
#ifndef NFC_WEBSOCKET_MODULE_H
#define NFC_WEBSOCKET_MODULE_H

#include "NFCWS.h"
#include "NFComm/NFPluginModule/NFIWebsocketModule.h"
#include "NFComm/NFPluginModule/NFILogModule.h"
#include "NFComm/NFNetPlugin/NFINet.h"
#include "NFComm/NFPluginModule/NFINetModule.h"


class NFCWebsocketModule : public NFIWebsocketModule
{
public:
	NFCWebsocketModule(NFIPluginManager* p);

	virtual NFCWS* GetNet() {
		return m_pWSServer;
	};
	virtual bool Init() override;

	virtual bool AfterInit() override;

	virtual bool Shut() override;

	virtual bool BeforeShut() override;

	virtual bool Execute() override;


	virtual int
		Initialization(const unsigned int nMaxClient, const unsigned short nPort, const int nCpuCount = 4) override;

	virtual bool
		SendMsgToAllClient(const char* msg, const uint32_t nLen, NF_WS_MSG_DATA_TYPE msg_data_type = TEXT) override;

	virtual bool
		SendMsgToClient(const char* msg, const uint32_t nLen, const std::vector<websocketpp::connection_hdl>& conn_list,
			NF_WS_MSG_DATA_TYPE msg_data_type = TEXT) override;

	virtual bool SendMsgToClient(const char* msg, const uint32_t nLen, websocketpp::connection_hdl conn,
		NF_WS_MSG_DATA_TYPE msg_data_type = TEXT) override;

	virtual bool SendMsgToClient(const int nMsgID, const char* msg, const uint32_t nLen, websocketpp::connection_hdl conn, NF_WS_MSG_DATA_TYPE msg_data_type) override;




public:
private:
	virtual void AddReceiveCallBack(const int nMsgID, NF_WS_MSG_CALL_BACK_PTR functorPtr);
	virtual void AddReceiveCallBack(NF_WS_MSG_CALL_BACK_PTR functorPtr);

	virtual void AddRecWsCallBack(NF_WS_CALL_BACK_PTR functorPtr);

	virtual void SetReceiveCallBack(NF_WS_MSG_CALL_BACK_PTR functorPtr) override;

	virtual void SetEventCallBack(NF_WS_EVENT_CALL_BACK_PTR functorPtr) override;

	void OnWebsocketMessage(websocketpp::connection_hdl, const int nMsgID,const char * strPayload, const uint32_t length);

	void OnCallbalckWs(websocketpp::connection_hdl, const char * strPayload, const uint32_t length);

	void OnWebsocketEvent(websocketpp::connection_hdl, NF_WS_EVENT);
	int DeCode(const char* strData, const uint32_t unAllLen, NFCMsgHead& xHead)
	{

		if (unAllLen < NFIMsgHead::NF_Head::NF_HEAD_LENGTH)
		{

			return -1;
		}

		if (NFIMsgHead::NF_Head::NF_HEAD_LENGTH != xHead.DeCode(strData))
		{

			return -2;
		}

		if (xHead.GetBodyLength() > (unAllLen - NFIMsgHead::NF_Head::NF_HEAD_LENGTH))
		{

			return -3;
		}

		return xHead.GetBodyLength();
	}
private:
	NFCWS* m_pWSServer;
	NF_WS_MSG_CALL_BACK_PTR m_pRecvMsgCB;
	NF_WS_EVENT_CALL_BACK_PTR m_pEvtCB;
	NFILogModule* m_pLogModule;


	std::map<int, std::list<NF_WS_MSG_CALL_BACK_PTR>> mxReceiveCallBack;
	std::list<NET_EVENT_FUNCTOR_PTR> mxEventCallBackList;
	std::list<NF_WS_MSG_CALL_BACK_PTR> mxCallBackList;

	std::list<NF_WS_CALL_BACK_PTR> mxWsCallBackList;

	// 通过 NFIWebsocketModule 继承
	virtual bool SendMsgPB(const uint16_t nMsgID, const google::protobuf::Message & xData, websocketpp::connection_hdl conn) override;


	// 通过 NFIWebsocketModule 继承
	virtual bool SendMsgWithOutHead(const int nMsgID, const std::string & msg, websocketpp::connection_hdl conn) override;


	// 通过 NFIWebsocketModule 继承
	virtual void RemoveReceiveCallBack(const int nMsgID) override;

};

#endif