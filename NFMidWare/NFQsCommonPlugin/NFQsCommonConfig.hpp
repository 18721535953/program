#ifndef NF_QS_COMMON_CONFIG_HPP
#define NF_QS_COMMON_CONFIG_HPP
#include <map>
#include "NFComm/NFCore/NFSingleton.hpp"
namespace NFGamespace
{
	enum QS_GameServer_Type
	{
		EG_FIGHE_LAND_LORD = 10101,				//10101;		// 斗地主
		EG_GALLOP = 40101,						//40101;		// 奔驰宝马
		EG_FIGHT_CATTLES = 50001,				//50001;		// 百人牛牛
		EG_THE_CHINESE_BOXER = 50002,			//50002;		// 龙虎斗
		EG_BACCARAT = 50003,					//50003;		// 百家乐
		EG_GOLDEN_FLOWERS = 80001,				//80001;		// 扎金花
	};

	class NFCommonConfig : public NFSingleton<NFCommonConfig>
	{
	public:
		NFCommonConfig()
		{
			m_ConfigMap.insert(make_pair(QS_GameServer_Type::EG_FIGHT_CATTLES, "FightCattles"));
			m_ConfigMap.insert(make_pair(QS_GameServer_Type::EG_GALLOP, "Gallop"));
			m_ConfigMap.insert(make_pair(QS_GameServer_Type::EG_FIGHE_LAND_LORD, "FightLandLord"));
			m_ConfigMap.insert(make_pair(QS_GameServer_Type::EG_THE_CHINESE_BOXER, "TheChineseBoxer"));
			m_ConfigMap.insert(make_pair(QS_GameServer_Type::EG_BACCARAT, "Baccarat"));
			m_ConfigMap.insert(make_pair(QS_GameServer_Type::EG_GOLDEN_FLOWERS, "GoldenFlowers"));
		}

		bool GetGameServerTypeID(QS_GameServer_Type type,std::string & typeID)
		{
			if (m_ConfigMap.find(type) == m_ConfigMap.end())
			{
				NFASSERT(type, "not gameType", __FILE__, __FUNCTION__);
				return false;
			}

			typeID = m_ConfigMap[type];
			return true;
		}

	private:
		std::map<QS_GameServer_Type, std::string> m_ConfigMap;
	};

	

	
}



#endif