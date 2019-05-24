#ifndef __TYPE__MOGO__DEF__HEAD__
#define __TYPE__MOGO__DEF__HEAD__

//#include "win32def.h"
#include "util.hpp"
//#include "memory_pool.h"
//#include "json_helper.h"
#include <stdlib.h>
#include <list>
using std::list;
#include <inttypes.h>

#define MS_PER_SEC 1000
#define INVALID_USERID -1
#define INVALID_INDEX -1

typedef float float32_t;
typedef double float64_t;
class CPluto;

typedef uint32_t TENTITYID;
typedef uint64_t TDBID;
typedef uint16_t TENTITYTYPE;
typedef uint32_t TSPACEID;
typedef unsigned short T_INTEREST_SIZE;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint16_t pluto_msgid_t;

struct VOBJECT;

enum VTYPE
{
    V_TYPE_ERR      = -1,

    V_OBJ_STRUCT    = 1,        // 结构体
    V_STR           = 2,
    V_INT8          = 3,
    V_UINT8         = 4,
    V_INT16         = 5,
    V_UINT16        = 6,
    V_INT32         = 7,
    V_UINT32        = 8,
    V_INT64         = 9,
    V_UINT64        = 10,
    V_FLOAT32       = 11,
    V_FLOAT64       = 12,
    V_OBJ_ARY       = 13,       // 数组
};

struct VTYPE_OJBECT;
typedef vector<VTYPE_OJBECT*> T_VECTOR_VTYPE_OBJECT;
struct VTYPE_OJBECT
{
    VTYPE vt;                   // Item的类型
    string vtName;              // 该类型对应json的名称
    T_VECTOR_VTYPE_OBJECT* o;   // 如果是V_OBJ_ARY，第1个元素表示数组元素的类型 如果是V_OBJ_STRUCT，这里保存每个元素的类型

	VTYPE_OJBECT()
	{
		vt = V_TYPE_ERR;
		o = NULL;
	}

	~VTYPE_OJBECT()
	{
		switch (vt)
		{
		case V_OBJ_STRUCT:
		case V_OBJ_ARY:
		{
			if (NULL != o)
			{
				CLEAR_POINTER_CONTAINER(*o);
				delete o;
			}
			break;
		}
		}
	}
};

struct VOBJECT;
typedef vector<VOBJECT*> T_VECTOR_OBJECT;

union VVALUE
{
    string* s;
    T_VECTOR_OBJECT* oOrAry;

    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float32_t f32;
    float64_t f64;
};

struct VOBJECT
{
    VTYPE vt;
    VVALUE vv;

	VOBJECT()
	{
		vt = V_TYPE_ERR;
		memset(&vv, 0, sizeof(vv));
	}

	~VOBJECT()
	{
		switch (vt)
		{
		case V_STR:
		{
			if (NULL != vv.s)
				delete vv.s;
			break;
		}
		case V_OBJ_STRUCT:
		case V_OBJ_ARY:
		{
			if (NULL != vv.oOrAry)
			{
				CLEAR_POINTER_CONTAINER(*vv.oOrAry);
				delete vv.oOrAry;
			}
			break;
		}
		}

		vt = V_TYPE_ERR;
	}
};
    
// 自动指针，为了用栈自动释放new的内存
template<class _Ty>
class auto_new1_ptr
{
public:
    // explicit表示必须显式构造，赋值构造不行
    explicit auto_new1_ptr(_Ty *_Ptr)
        : _Myptr(_Ptr)
    {    // construct from object pointer
    }

    ~auto_new1_ptr()
    {    // destroy the object
        if(NULL != _Myptr)
            delete _Myptr;
    }

    // 可能存在一定情况下释放的情况
    void OverridePtr(_Ty *_Ptr)
    {
        _Myptr = _Ptr;
    }
private:
    _Ty *_Myptr;
};

// 自动指针，为了用栈自动释放new的数组内存
template<class _Ty>
class auto_new_array_ptr
{
public:
    explicit auto_new_array_ptr(_Ty *_Ptr)
        : _Myptr(_Ptr)
    {    // construct from object pointer
    }

    ~auto_new_array_ptr()
    {    // destroy the object
        if(NULL != _Myptr)
            delete [] _Myptr;
    }

    // 可能存在一定情况下释放的情况
    void OverridePtr(_Ty *_Ptr)
    {
        _Myptr = _Ptr;
    }
private:
    _Ty *_Myptr;
};


template < template <typename ELEM,
            typename ALLOC = std::allocator<ELEM>
            > class TC
            >
void ClearTListObject(TC<VOBJECT*, std::allocator<VOBJECT*> >* c1)
{
    CLEAR_POINTER_CONTAINER(*c1);
    delete c1;
}


template<typename T>
void CopyEntityIdSet(const T& from, T& to)
{
    typename T::const_iterator it = from.begin();
    for(; it != from.end(); ++it)
    {
        to.insert(*it);
    }
}


#endif
