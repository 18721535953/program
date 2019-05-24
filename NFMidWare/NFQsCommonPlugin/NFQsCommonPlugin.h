#ifndef NF_QS_COMMON_PLUGIN_H
#define NF_QS_COMMON_PLUGIN_H
#include "NFComm/NFPluginModule/NFIPlugin.h"
#include "NFComm/NFPluginModule/NFIPluginManager.h"

class NFQsCommonPlugin:public NFIPlugin
{
public:
	NFQsCommonPlugin(NFIPluginManager* p)
	{
		pPluginManager = p;
	}
	virtual const int GetPluginVersion();

	virtual const std::string GetPluginName();

	virtual void Install();

	virtual void Uninstall();

};

#endif // !NF_QS_COMMON_PLUGIN_H
