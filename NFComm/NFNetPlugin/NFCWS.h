// -------------------------------------------------------------------------
//    @FileName			:		NFCWS.h
//    @Author			:		Stone.xin
//    @Date				:		2016-12-22
//    @Module			:		NFCWS
// -------------------------------------------------------------------------

#ifndef NFC_WS_H
#define NFC_WS_H

#include "NFIWS.h"

#include "NFComm/NFNetPlugin/NFINet.h"
#pragma pack(push, 1)

//处理WebSocket 连接
class WSObject
{
public:

	WSObject() {
		mnLogicState = 0;
		mnGameID = 0;
		bNeedRemove = false;
	}

	virtual ~WSObject()
	{

	}

	int AddBuff(const char* str, size_t nLen)
	{
		mstrBuff.append(str, nLen);

		return (int)mstrBuff.length();
	}

	int CopyBuffTo(char* str, uint32_t nStart, uint32_t nLen)
	{
		if (nStart + nLen > mstrBuff.length())
		{
			return 0;
		}

		memcpy(str, mstrBuff.data() + nStart, nLen);

		return nLen;
	}

	int RemoveBuff(uint32_t nStart, uint32_t nLen)
	{
		if (nStart + nLen > mstrBuff.length())
		{
			return 0;
		}

		mstrBuff.erase(nStart, nLen);

		return (int)mstrBuff.length();
	}

	const char* GetBuff()
	{
		return mstrBuff.data();
	}

	int GetBuffLen() const
	{
		return (int)mstrBuff.length();
	}

	void* GetUserData()
	{
		return m_pUserData;
	}



	//////////////////////////////////////////////////////////////////////////
	const std::string& GetSecurityKey() const
	{
		return mstrSecurityKey;
	}

	void SetSecurityKey(const std::string& strKey)
	{
		mstrSecurityKey = strKey;
	}

	int GetConnectKeyState() const
	{
		return mnLogicState;
	}

	void SetConnectKeyState(const int nState)
	{
		mnLogicState = nState;
	}

	bool NeedRemove()
	{
		return bNeedRemove;
	}

	void SetNeedRemove(bool b)
	{
		bNeedRemove = b;
	}

	const std::string& GetAccount() const
	{
		return mstrUserData;
	}

	void SetAccount(const std::string& strData)
	{
		mstrUserData = strData;
	}

	int GetGameID() const
	{
		return mnGameID;
	}

	void SetGameID(const int nData)
	{
		mnGameID = nData;
	}

	const NFGUID& GetUserID()
	{
		return mnUserID;
	}

	void SetUserID(const NFGUID& nUserID)
	{
		mnUserID = nUserID;
	}

	const NFGUID& GetClientID()
	{
		return mnClientID;
	}

	void SetClientID(const NFGUID& xClientID)
	{
		mnClientID = xClientID;
	}

	const NFGUID& GetHashIdentID()
	{
		return mnHashIdentID;
	}

	void SetHashIdentID(const NFGUID& xHashIdentID)
	{
		mnHashIdentID = xHashIdentID;
	}

	NFSOCK GetRealFD()
	{
		return nFD;
	}

private:
	websocketpp::connection_hdl p_hdl;
	sockaddr_in sin;
	void* m_pUserData;
	std::string mstrBuff;
	std::string mstrUserData;
	std::string mstrSecurityKey;

	int32_t mnLogicState;
	int32_t mnGameID;
	NFGUID mnUserID;//player id
	NFGUID mnClientID;//temporary client id
	NFGUID mnHashIdentID;//hash ident, special for distributed
	//
	NFSOCK nFD;
	bool bNeedRemove;
};

typedef std::shared_ptr<WSObject> WSObjectPtr;

class NFCWS : public NFIWS
{
public:
    template<typename BaseType>
    NFCWS(BaseType* pBaseType,
          void (BaseType::*handleRecieve)(websocketpp::connection_hdl, const int ,const char *, const uint32_t nLen),
          void (BaseType::*handleEvent)(websocketpp::connection_hdl, NF_WS_EVENT),
		  void (BaseType::*handleRecWs)(websocketpp::connection_hdl,  const char *, const uint32_t nLen)
    )
    {
		mRecvMsgPack = std::bind(handleRecieve, pBaseType, std::placeholders::_1, std::placeholders::_2,
                            std::placeholders::_3, std::placeholders::_4);
        mEventCB = std::bind(handleEvent, pBaseType, std::placeholders::_1, std::placeholders::_2);

		mRecvWsPack = std::bind(handleRecWs, pBaseType, std::placeholders::_1, std::placeholders::_2,
			std::placeholders::_3 );

        //mstrIP = "";
        mnPort = 0;
        mnCpuCount = 0;
        //mbServer = true;
        mbWorking = false;

        mnSendMsgTotal = 0;
        mnReceiveMsgTotal = 0;
    }

    virtual ~NFCWS()
    {};

public:
    virtual bool Execute();

    virtual int Initialization(const unsigned int nMaxClient, const unsigned short nPort, const int nCpuCount = 4);

    virtual bool Final();

    virtual bool SendMsgToAllClient(const char* msg, const uint32_t nLen, NF_WS_MSG_DATA_TYPE msg_data_type = TEXT);

    virtual bool
    SendMsgToClient(const char* msg, const uint32_t nLen, const std::vector<websocketpp::connection_hdl>& conn_list,
                    NF_WS_MSG_DATA_TYPE msg_data_type = TEXT);

    virtual bool SendMsgToClient(const char* msg, const int nMsgID,const uint32_t nLen, websocketpp::connection_hdl conn,
                                 NF_WS_MSG_DATA_TYPE msg_data_type = TEXT);
	//virtual bool SendWsMsgToClient(const char* msg, const int nMsgID,const uint32_t nLen, websocketpp::connection_hdl conn,
    //                             NF_WS_MSG_DATA_TYPE msg_data_type = TEXT);
    virtual bool AddNetObject(websocketpp::connection_hdl conn, WSObjectPtr pObject);

    virtual WSObjectPtr GetNetObject(websocketpp::connection_hdl conn);

private:
    void ExecuteClose();

    bool CloseSocketAll();

    void CloseObject(websocketpp::connection_hdl hd, NF_WS_CLOSE_CODE nCloseCode = CLOSE_CODE_NORMAL,
                     const std::string& strCloseReason = "");

    void OnMessageHandler(websocketpp::connection_hdl hd, NFWebSockConf::message_ptr msg);

    void OnOpenHandler(websocketpp::connection_hdl hd);

    bool
    RemoveConnection(websocketpp::connection_hdl hd, NF_WS_EVENT evt, NF_WS_CLOSE_CODE nCloseCode = CLOSE_CODE_NORMAL,
                     const std::string& strCloseReason = "");

    void OnCloseHandler(websocketpp::connection_hdl hd);

    void OnFailHandler(websocketpp::connection_hdl hd);

    void OnInterruptHandler(websocketpp::connection_hdl hd);

    void OnPongHandler(websocketpp::connection_hdl hd, std::string str);

    void OnPongTimeOutHandler(websocketpp::connection_hdl hd, std::string str);
	int EnCode(const uint16_t unMsgID, const char* strData, const uint32_t unDataLen, std::string& strOutData)
	{
		NFCMsgHead xHead;
		xHead.SetMsgID(unMsgID);
		xHead.SetBodyLength(unDataLen);

		char szHead[NFIMsgHead::NF_Head::NF_HEAD_LENGTH] = { 0 };
		xHead.EnCode(szHead);

		strOutData.clear();
		strOutData.append(szHead, NFIMsgHead::NF_Head::NF_HEAD_LENGTH);
		strOutData.append(strData, unDataLen);

		return xHead.GetBodyLength() + NFIMsgHead::NF_Head::NF_HEAD_LENGTH;
	}
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
    //<fd,object>
    typedef std::map<websocketpp::connection_hdl, WSObjectPtr, std::owner_less<websocketpp::connection_hdl> > session_list;
    session_list mmObject;
    std::vector<websocketpp::connection_hdl> mvRemoveObject;

    int mnMaxConnect;
    int mnPort;
    int mnCpuCount;

    bool mbWorking;

    int64_t mnSendMsgTotal;
    int64_t mnReceiveMsgTotal;

    NFWebSockConf m_EndPoint;
    //////////////////////////////////////////////////////////////////////////

	//std::function<void(websocketpp::connection_hdl, const int ,const char *, const int)> mRecvMsgPack;
	NF_WS_MSG_CALL_BACK mRecvMsgPack;
    NF_WS_EVENT_CALL_BACK mEventCB;
	NF_WS_CALL_BACK mRecvWsPack;
	//std::function<void(websocketpp::connection_hdl, const char *, const int)> mRecvWsPack;

    //////////////////////////////////////////////////////////////////////////
};

#pragma pack(pop)

#endif
