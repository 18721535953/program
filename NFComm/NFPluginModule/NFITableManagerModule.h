#ifndef NFI_TABLE_MANAGER_MODULE_H
#define NFI_TABLE_MANAGER_MODULE_H
#include <vector>
#include <map>
#include "Dependencies/json/json.hpp"
#include "NFIModule.h"
#include "NFITableUserModule.h"
class NFGameTable;
class NFTableUser;
struct SUserInfo;

class NFITableManagerModule :public NFIModule
{
public:
    virtual void OnUserOffline(int userId) = 0;

    virtual bool AddSitUser(int userId, int tableHandle, int chairIndex) = 0;

    virtual bool RemoveSitUser(int userId) = 0;

    virtual NF_SHARE_PTR<NFGameTable> GetUserTable(int usreId, int & chairIndex) = 0;

    virtual NF_SHARE_PTR<NFGameTable> GetPUserTable(const SUserInfo& pUser, int& chairIndex) = 0;

    virtual NF_SHARE_PTR<NFGameTable> GetTableByHandle(int handle) = 0;

    virtual void AddTable(NF_SHARE_PTR<NFGameTable> newTable) = 0;

    virtual inline int GetUserCount() = 0;

    virtual int GetSelScoreByHandle(int handle) = 0;

    //���ͨ��ѡ��׷ֵķ�ʽ���뷿��
    virtual bool EnterRoomBySelectSocre(NF_SHARE_PTR<SUserInfo> pUser, int ignoreTableHandle, std::function<void()> successCallBack) = 0; 

    virtual bool userChangeTable(SUserInfo* pUser, NFGameTable* oldTable, std::function<void()> successCallBack) = 0;

    /*
     * �����ʼ��������û�
     * share_pu : ��Ϣ��
     * fd : socket ������
     */
	virtual void SendMailToTableUser(const int nMsgId, const nlohmann::json& jsonSend, const NFGUID& fd) = 0;

    /*
     * �㲥�ʼ���һ�������ϵ������û�
     * share_pu : ��Ϣ��
     * ignoreUserId : ���Ե��û�ID
     * isIgnoreState : ���Ե��û�״̬�����ڴ�״̬���û���������
     * tableUserMap : �û��б�
     */
    virtual void SendMailToUserAtTheSameTable(const int nMsgId, const nlohmann::json& jsonSend, const int ignoreUserId, int isIgnoreState, std::map<int, NF_SHARE_PTR<NFITableUserModule>>& tableUserMap) = 0;

    /*
    * �㲥�ʼ������������ϵ������û�
    * share_pu : ��Ϣ��
    * ignoreUserId : ���Ե��û�ID
    * tableVector : �����б�
    */
    virtual void SendMailToAllTableUser(const int nMsgId, const nlohmann::json& jsonSend, int ignoreUserId, std::vector<NF_SHARE_PTR<NFGameTable>> & tableVector) = 0;
};

#endif