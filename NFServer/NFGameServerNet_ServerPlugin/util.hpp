#ifndef __UTIL__HEAD__
#define __UTIL__HEAD__

//#include "win32def.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <list>
//#include "exception.h"

//#include <dirent.h>
#include <stdint.h>
#include <fcntl.h>
#include "NFComm/NFPluginModule/NFIModule.h"
#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"
//#include <sys/time.h>
//#include <unistd.h>
//#include "exception.hpp"
#include "NFComm/NFMessageDefine/NFDefine.pb.h"
#include "NFComm/NFCore/NFUtil.hpp"
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::ios;
using std::list;

enum
{
	MAX_FILE_HANDLE_COUNT = 1024,
	MAX_EPOLL_SIZE = 9999,
	CLIENT_TIMEOUT = 20,
	OTHERSERVER_TIMEOUT = 5,
};

/*

//删除字符串左边的空格
extern string& Ltrim(string& s);
//删除字符串右边的空格
extern string& Rtrim(string& s);
//删除字符串两边的空格
inline string& Trim(string& s) { return Rtrim(Ltrim(s)); }
//删除字符串左边的空格
extern char* Ltrim(char* p);
//删除字符串右边的空格
extern char* Rtrim(char* p);
//删除字符串两边的空格
inline char* Trim(char* s) { return Rtrim(Ltrim(s)); }

//Aes解密
extern void AesDecryptStr(const string& base64Str, const string& aesKey, string& retStr);
*/
//获得字符串的md5值
inline static void GetStrMd5(const string& src, string& retStr)
{
	retStr = MD5(src).toStr();
}
/*
//比较一个字符串的大写是否匹配一个大写的字符串
extern bool UpperStrCmp(const char* src, const char* desc);
//4字节包头是否为 GET+空格
extern bool IsPlutoHeaderGet(const char* szHeader);
//http数据包是否接收完毕 从尾部查找4字节结束符合
extern char * GetPlutoReceiveEndPos(char* szBuffer, int nLen);

//按照分隔符nDelim拆分字符串
extern list<string> SplitString(const string& s1, int nDelim);
extern void SplitString(const string& s1, int nDelim, list<string>& ls);
extern void SplitStringToVector(const string& s1, int nDelim, vector<string>& ls);
extern void SplitStringToMap(const string& s1, int nDelim1, char nDelim2, map<string, string>& dict);

//替换string中第一次出现的某个部分
extern string& xReplace(string& s1, const char* pszSrc, const char* pszRep);
//判断一个字符串是否全部由数字字符组成
extern bool IsDigitStr(const char* pszStr);

//测试文件strFileName是否存在
extern bool IsFileExist(const char* pszFileName);
extern bool IsFileExist(const string& strFileName);

//根据字符串获得区域编号
int GetAreaNumBy6TableNum(string& tableNum, int& tableHandle);
//根据桌子handle获得桌子编号
string GetTableNumByHandle(int areaNum, int tableHandle);
// 根据字符串获得区域编号(tableInnerIdx：指的是桌子内部用于产生桌号的累加索引值)
int GetAreaNumBy6TableNum(string& tableNum, int& tableHandle, int& tableInnerIdx);
// 根据桌子handle获得桌子编号
string GetTableNumByHandle(int areaNum, int tableHandel, int tableInnerIdx);
//计算两个经纬度之间的距离(weidu: 纬度(-90..90)  jingdu：经度(-180..180) 返回单位：米)
extern int get_distance(double weidu1, double jingdu1, double weidu2, double jingdu2);

//获取随机值[min,max](包括min和max)
extern int GetRandomRange(int min, int max);
//获取随机值[min,max](包括min和max)
extern int64_t GetRandomRange(int64_t min, int64_t max);
//是否命中百分比概率
extern bool IsFix100Rate(int rate); */
//获取从系统启动到现在的毫秒数，改变系统不会影响此值，返回值一定大于0(注意：此函数大约每50天会溢出归零一次，建议使用GetNowMsTick64)
inline static uint32_t GetNowMsTick(){
	return (uint32_t)time(NULL);
}
//获取从系统启动到现在的毫秒数，改变系统不会影响此值，返回值一定大于0

inline static uint64_t GetNowMsTick64()
{
	return (uint64_t)time(NULL);
}

//获取两个时间戳之间的间隔
inline static uint64_t GetMsTickDiff(uint64_t oldMs, uint64_t newMs)
{
	return newMs - oldMs;
}

/*
//获取今天日期字符串，格式为20080101
extern void GetTodayDateStr(string& strCurTime);
//获取昨天日期字符串，格式为20080101
extern void GetYesterdayDateStr(string& strYesterday);
//计算当前时间到指定时间的差值(单位：秒)
extern int CalcSecondDiffFromNow(const string& strDateStr, int nHour);
//获取当前时间字符串，格式为20080101020202
extern void GetNowTimeStr(char* szOut, size_t nLen);
*/
//获取当前时间戳(毫秒)
inline static uint64_t GetTimeStampInt64Ms()
{
	return (uint64_t)time(NULL);
}
//获取当前时间戳(秒)
inline static uint64_t GetTimeStampInt64Sec()
{
	return (uint64_t)time(NULL);
}

//时间计时器
class CCalcTimeTick
{
    public:
		CCalcTimeTick() {  }
        ~CCalcTimeTick(){}

    private:
        //void GetNowTick(struct timespec& tm);
       // uint32_t GetMsTickByTime(struct timespec& tm);
       // uint64_t GetUsTickByTime(struct timespec& tm);
       // uint64_t GetNsTickByTime(struct timespec& tm);
    public:
        //获取当前时间和上次的流逝毫秒
		uint32_t GetPassMsTick() { return m_msTick; }
        //获取当前时间和上次的流逝微秒
		uint64_t GetPassUsTick() { return m_msTick; }
        //获取当前时间和上次的流逝纳秒
		uint64_t GetPassNsTick() { return m_msTick; }
        //设置当前时间为tick
        void SetNowTime(){   }
        //添加毫秒
        void AddMsTick(uint32_t add){}
        //减少毫秒
        void DecMsTick(uint32_t dec){}
    private:
        //毫秒数
		int  m_msTick;
        //微秒数
        //uint64_t m_usTick;
        //纳秒数
        //uint64_t m_nsTick;
};
/*
//Mutex线程锁
class CMutexLock
{
    pthread_mutex_t m_Mutex; 
public :
    CMutexLock( ){ pthread_mutex_init( &m_Mutex , NULL );} ;
    ~CMutexLock( ){ pthread_mutex_destroy( &m_Mutex) ; } ;
    
	void Lock( ){ pthread_mutex_lock(&m_Mutex); } ;
    void Unlock( ){ pthread_mutex_unlock(&m_Mutex); } ;
};
*/
//用于清理一个指针容器(std::list std::vector等，内容为对象指针，清理时会释放对象)
template <typename TP, template <typename ELEM, typename ALLOC = std::allocator<ELEM>> class TC>
void CLEAR_POINTER_CONTAINER(TC<TP, std::allocator<TP> >& c1)
{
	while (!c1.empty())
	{
		typename TC<TP>::iterator it = c1.begin();
		delete *it;
		*it = NULL;
		c1.erase(it);
	}
}

//用于清理一个map,第二个类型为指针(内容为对象指针，清理时会释放对象)
 template<typename T1, typename T2, template <class _Kty, class _Ty, class _Pr = std::less<_Kty>, class _Alloc = std::allocator<std::pair<const _Kty, _Ty> > > class M>
 void CLEAR_POINTER_MAP(M<T1, T2, std::less<T1>, std::allocator<std::pair<const T1, T2> > >& c1)
 {
 	typename M<T1, T2>::iterator it = c1.begin();
 	for (; it != c1.end(); ++it)
 	{
 		delete it->second;
 		it->second = NULL;
 	}
 	c1.clear();
 }

//extern bool g_authFailed;
//extern void CheckAutoAuth(bool isSart);

//从VOBJECT*中读取字段宏定义
#define VOBJECT_GET_SSTR(p) *(p->vv.s)
#define VOBJECT_GET_STR(p) p->vv.s->c_str()
#define VOBJECT_GET_U8(p) p->vv.u8
#define VOBJECT_GET_U16(p) p->vv.u16
#define VOBJECT_GET_U32(p) p->vv.u32
#define VOBJECT_GET_U64(p) p->vv.u64
#define VOBJECT_GET_I8(p) p->vv.i8
#define VOBJECT_GET_I16(p) p->vv.i16
#define VOBJECT_GET_I32(p) p->vv.i32
#define VOBJECT_GET_I64(p) p->vv.i64
#define VOBJECT_GET_F32(p) p->vv.f32
#define VOBJECT_GET_EMB(p) p->vv.emb
#define VOBJECT_GET_BLOB(x) (x->vv.p)
#endif
