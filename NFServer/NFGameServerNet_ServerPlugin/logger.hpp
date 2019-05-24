#ifndef __LOGGER__HEAD__
#define __LOGGER__HEAD__

//#include "win32def.h"
//#include "my_stl.h"
//#include "net_util.h"
#include "cfg_reader.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <ostream>
//#include <sys/time.h>

using std::ofstream;
using std::ostream;
using std::ios;


class CLogger
{
    public:
		CLogger() {}
		~CLogger() {}

    public:
		//void InitCfg(CCfgReader& cfg) {}
		void SendLog(const char * pMsg, size_t len) {}
    private:
        int m_socket;
        //struct sockaddr_in m_toAddr;
};

enum {MAX_LOG_BUFF_SIZE = 4096,};

//extern void Log(const char* section, const char* key, const char* msg, va_list& ap);
inline static void LogDebug(const char* key, const char* msg, ...)
{
	std::cout << "LogDebug: "<<key<<","<< msg << std::endl;
}
inline static void LogInfo(const char* key, const char* msg, ...)
{
	std::cout << "LogInfo: " << key << "," << msg << std::endl;
}
inline static void LogWarning(const char* key, const char* msg, ...)
{
	std::cout << "LogWarning: " << key << "," << msg << std::endl;
}
inline static void LogError(const char* key, const char* msg, ...)
{
	std::cout << "LogError: " << key << "," << msg << std::endl;
}
inline static void LogCritical(const char* key, const char* msg, ...)
{
	std::cout << "LogCritical: " << key << "," << msg << std::endl;
}
inline static void LogScript(const char* level, const char* msg, ...)
{
	std::cout << "LogScript: " << level << "," << msg << std::endl;
}
inline static void Error(const char* level, const char* msg, ...)
{
	std::cout << "Error: " << level << "," << msg << std::endl;
}


//CLogger g_logger;

//g_logger对应的线程锁
//extern pthread_mutex_t* g_logger_mutex;


#endif
