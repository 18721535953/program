#ifndef NFI_TABLE_USER_MODULE_H
#define NFI_TABLE_USER_MODULE_H
#include <vector>
#include <map>

class SUserInfo;

class NFITableUserModule
{
public:
	virtual bool GetIsRobot() const = 0;

	virtual int GetUsTate() const = 0;

	virtual SUserInfo& GetSUserInfo() = 0;
};
#endif