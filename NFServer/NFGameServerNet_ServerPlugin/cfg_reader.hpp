#ifndef __CFG_READER__HEAD__
#define __CFG_READER__HEAD__
#include <string>
#include <map>
using namespace std;
//#include "win32def.h"
// #include "exception.h"
// #include "my_stl.h"
// #include "util.h"

//自定义读配置类
class CCfgReader
{
    public:
        CCfgReader(const string& strFile){}
        ~CCfgReader(){}

        // throw exception
		string GetValue(const char* szSection, const char* szName) { return ""; }
        //
		string GetOptValue(const char* szSection, const char* szName, const string& strDefault) { return ""; }

    private:
        void ScanCfgFile(){}

    private:
        string m_strFile;
        map<string, map<string, string>*> m_CfgDict;
        bool m_bScan;

};


#endif
