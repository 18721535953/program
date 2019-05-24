// #include "epoll_gamearea.h"
// #include "world_gamearea.h"
// #include "global_var.h"
// //#include "debug.h"
// #include "signal.h"
// //#include "world.h"
// //#include "pluto.h"
// #include "NFServer/NFGameServerNet_ServerPlugin/util.hpp"
// //#include "http.h"
// #include "type_mogo.h"

// int main(int argc, char* argv[])
// {
//     if(argc < 3)
//     {
//         printf("Usage:%s etc_fn server_id \n", argv[0]);
//         return -1;
//     }
// 
//     srand((unsigned int)time(NULL));
//     g_taskIdAlloctor = rand();
// 
//     //命令行参数,依次为: 配置文件路径,server_id
//     const char* szCfgFile = argv[1];
//     uint32_t nServerId = (uint32_t)atoi(argv[2]);
// 
//     signal(SIGPIPE, SIG_IGN);
//     CDebug::Init();
//     
// 	//创建世界管理器
// 	CWorld::InitInst(new CWorldGameArea());
// 
// 	//初始化世界控制器
// 	int nRet = WORLD_MGR->init(szCfgFile);
// 	if (nRet != 0)
// 	{
// 		printf("world init error:%d\n", nRet);
// 		return nRet;
// 	}
// 
// 	//设置EPoll服务器
//     CEpollGameAreaServer epollSrv;
//     epollSrv.SetBindServerID(nServerId);
//     epollSrv.SetWorld(WORLD_MGR);
// 	WORLD_MGR->SetServer(&epollSrv);
// 
// 	uint16_t unPort = WORLD_MGR->GetServerPort(nServerId);
//     epollSrv.Service("", unPort);
// 
// 	WORLD_MGR->FreeInst();
//     delete CONFIG_AREA;
//     delete ROBOT_MGR;
//     delete TABLE_MGR;
//     delete LOGIC_MGR;
// }
