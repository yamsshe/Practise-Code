#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <thread>
#include <windows.h>
#include <io.h>
using namespace std;

#define MAX_BUF_SZIE 2048                           // 每行文件最大字符数
#define MAX_THREAD_NUM 5                            // 工作线程最大数量
#define MIN_THREAD_NUM 1                            // 工作线程最小数量

//文件行数信息
struct Cloc_Data
{
	int m_Files;                    // 文件数
    int m_TotalLine;				// 总行数
	int m_BlankLine;				// 空行数
	int m_CodeLine;				    // 代码行数
	int m_NoteLine;				    // 注释行数
};

bool g_RunFlag = true;
queue<string> g_FileQueue;									// 文件队列
Cloc_Data g_cloc_data = {0};                                // 统计数据
int g_thread_num = 0;                                       // 统计线程数量
CRITICAL_SECTION g_FileQueue_cs;							// 文件队列临界区
CRITICAL_SECTION g_Cloc_cs;								    // 统计数据临界区
CRITICAL_SECTION g_Thread_cs;								// 统计线程数量临界区

void ClocCodeC(string sPath);
void ClocCodeRUBY(string sPath);
void ClocFile(string sPath);

//统计代码的线程函数
DWORD WINAPI ClocThread(LPVOID lpParam)
{
	while (g_RunFlag)  
	{
		string spath;
        // 多线程操作保证线程安全
        EnterCriticalSection(&g_FileQueue_cs);
        // 当文件队列不为空时
        if (!g_FileQueue.empty())
        {
            spath = g_FileQueue.front();
            g_FileQueue.pop();

            ClocFile(spath);
        }
        else
        {
            g_RunFlag = false;
        }
        LeaveCriticalSection(&g_FileQueue_cs);
        
	}

    EnterCriticalSection(&g_Thread_cs);
    --g_thread_num;
    LeaveCriticalSection(&g_Thread_cs);
	return 0;
}

// 获取目录下所有文件
void GetFiles(string path, queue<string>& files)
{
	//文件句柄  
	long  hFile = 0;
	//文件信息  
	struct _finddata_t fileinfo;
	string p;
	if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			//如果是目录,迭代之  
			//如果不是,加入列表  
			if ((fileinfo.attrib &  _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					GetFiles(p.assign(path).append("\\").append(fileinfo.name), files);
			}
			else
			{
				files.push(path + "\\" + fileinfo.name);
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
	else
	{
		//尝试打开文件,能打开就直接加入队列
        ifstream iFile;
        iFile.open(path.c_str());
        if(iFile.is_open())
        {
            files.push(path);
        }
	}
}

// 辅助函数,去除空格,tab,换行符,续航符
void AdjustString(string &sStr)
{
	if (sStr.empty())
		return;
	int index = 0;
	while ((index = sStr.find(' ', index)) != string::npos)
	{
		sStr.erase(index, 1);
	}
	while ((index = sStr.find('\t', index)) != string::npos)
	{
		sStr.erase(index, 1);
	}
	while ((index = sStr.find('\n', index)) != string::npos)
	{
		sStr.erase(index, 1);
	}
	while ((index = sStr.find('\r', index)) != string::npos)
	{
		sStr.erase(index, 1);
	}
}

// 辅助函数,累加统计数据
void AddClocData(Cloc_Data data)
{
    //多线程操作,保证线程安全
    EnterCriticalSection(&g_Cloc_cs);

    g_cloc_data.m_TotalLine += data.m_TotalLine;
    g_cloc_data.m_BlankLine += data.m_BlankLine;
    g_cloc_data.m_NoteLine += data.m_NoteLine;
    g_cloc_data.m_CodeLine += data.m_CodeLine;

    LeaveCriticalSection(&g_Cloc_cs);
}

// 根据不同的文件类型来统计数据
void ClocFile(string sPath)
{
    if (!sPath.empty())
    {
        int index = sPath.find_last_of(".");
        if(index > 0)
        {
            string type = sPath.substr(index, sPath.length());
            if(0 == strcmp(".h",type.c_str()) || 0 == strcmp(".c",type.c_str()) || 0 == strcmp(".cpp",type.c_str()) || 0 == strcmp(".hpp",type.c_str()))
            {
                ClocCodeC(sPath);
            }
            if(0 == strcmp(".rb",type.c_str()))
            {
                ClocCodeRUBY(sPath);
            }
        }
    }
}

//统计c代码
void ClocCodeC(string sPath)
{
	ifstream iFile;
	iFile.open(sPath.c_str());
    Cloc_Data cloc_data = { 0 };
	if (!iFile.is_open())
	{
		cout << "opening file faild" << endl;
		return;
	}
	char szbuff[MAX_BUF_SZIE];
	bool flag = false;
	while (!iFile.eof() && iFile.getline(szbuff, MAX_BUF_SZIE - 1))
	{
		string sStr(szbuff);
		AdjustString(sStr);
		cloc_data.m_TotalLine++;
		if(sStr.empty() || 0 == sStr.length())
		{
			cloc_data.m_BlankLine++;
			continue;
		}
		//处多行注释状态
		if (flag)
		{
			int index_l = sStr.find("*/");
			//多行注释再此行处结束
			if (index_l != string::npos)
			{
				flag = false;
				if(index_l < sStr.length() - 2)
				{
					cloc_data.m_CodeLine++;
					continue;
				}
			}
			cloc_data.m_NoteLine++;
		}
		else
		{
			int index_s = sStr.find("//");
			//这里是单行注释
			if (0 == index_s)
			{
				cloc_data.m_NoteLine++;
				continue;
			}
			//先寻找是否开始多行注释
			int index_f = sStr.find("/*");
			int index_l = sStr.find("*/");
			if (string::npos != index_f)
			{
				// 多行注释不在此行结束
				if(string::npos == index_l)
				{
					if(0 == index_f)
					{
						cloc_data.m_NoteLine++;
					}
					else
					{
						cloc_data.m_CodeLine++;
					}
					flag = true;
					continue;
				}
				else
				{
					//此处相当于当行注释
					if(0 == index_f && index_l == sStr.length() - 2)
					{
						cloc_data.m_NoteLine++;
						continue;
					}
				}
			}
			cloc_data.m_CodeLine++;
		}
        memset(szbuff, 0, MAX_BUF_SZIE);
	}
    // 累加数据
    AddClocData(cloc_data);
}

//统计Ruby代码
void ClocCodeRUBY(string sPath)
{
	ifstream iFile;
	iFile.open(sPath.c_str());
    Cloc_Data cloc_data = { 0 };
	if (!iFile.is_open())
	{
		cout << "opening file faild" << endl;
		return;
	}
	char szbuff[MAX_BUF_SZIE];
	bool flag = false;
	while (!iFile.eof() && iFile.getline(szbuff, MAX_BUF_SZIE - 1))
	{
		string sStr(szbuff);
		AdjustString(sStr);
		cloc_data.m_TotalLine++;
		if(sStr.empty() || 0 == sStr.length())
		{
			cloc_data.m_BlankLine++;
			continue;
		}
		//处多行注释状态
		if (flag)
		{
			int index_l = sStr.find("=end");
			//多行注释再此行处结束
			if (index_l != string::npos)
			{
				flag = false;
				if(index_l < sStr.length() - 4)
				{
					cloc_data.m_CodeLine++;
					continue;
				}
			}
			cloc_data.m_NoteLine++;
		}
		else
		{
			int index_s = sStr.find("#");
			//这里是单行注释
			if (0 == index_s)
			{
				cloc_data.m_NoteLine++;
				continue;
			}
			//先寻找是否开始多行注释
			int index_f = sStr.find("=begin");
			int index_l = sStr.find("=end");
			if (string::npos != index_f)
			{
				// 多行注释不在此行结束
				if(string::npos == index_l)
				{
					if(0 == index_f)
					{
						cloc_data.m_NoteLine++;
					}
					else
					{
						cloc_data.m_CodeLine++;
					}
					flag = true;
					continue;
				}
				else
				{
					//此处相当于当行注释
					if(0 == index_f && index_l == sStr.length() - 4)
					{
						cloc_data.m_NoteLine++;
						continue;
					}
				}
			}
			cloc_data.m_CodeLine++;
		}
        memset(szbuff, 0, MAX_BUF_SZIE);
	}
    // 累加数据
    AddClocData(cloc_data);
}

//辅助函数,格式化输出结果
void PrintData()
{
    cout<<"--------------------------------------------------------------------------"<<endl;
    cout<<"Files            Lines           Codes           Comments            Blank"<<endl;
    cout<<"--------------------------------------------------------------------------"<<endl;
    cout.setf(ios::left);
    cout.width(16);
    cout<<g_cloc_data.m_Files;
    cout.setf(ios::left);
    cout.width(18);
    cout<<g_cloc_data.m_TotalLine;
    cout.setf(ios::left);
    cout.width(18);
    cout<<g_cloc_data.m_CodeLine;
    cout.setf(ios::left);
    cout.width(18);
    cout<<g_cloc_data.m_NoteLine;
    cout.setf(ios::left);
    cout.width(18);
    cout<<g_cloc_data.m_BlankLine<<endl;
    cout<<"--------------------------------------------------------------------------"<<endl;
}


int main(int argc, char* argv[])
{
    char **param = argv;
    ++param;
    if (NULL == *param)
    {
        cout<<" please input File Path "<<endl;
		system("pause");
        return 0;
    }
    string path(*param);
    // 将目标目录下的文件路径加入到文件队列
    GetFiles(path, g_FileQueue);

    //初始化临界区
	InitializeCriticalSection(&g_FileQueue_cs);
	InitializeCriticalSection(&g_Cloc_cs);
    InitializeCriticalSection(&g_Thread_cs);

    g_cloc_data.m_Files = g_FileQueue.size();
    int threads = g_cloc_data.m_Files / 50;
    if (threads > MAX_THREAD_NUM) 
    {
        threads = MAX_THREAD_NUM;
    }
	if (threads < MIN_THREAD_NUM) 
    {
        threads = MIN_THREAD_NUM;
    }
    HANDLE hWTHandle[MAX_THREAD_NUM];
    for (int i = 0; i < threads; i++)
	{
		int* nTemp = new int;
		*nTemp = i;
		hWTHandle[i] = CreateThread(NULL, 0, ClocThread, nTemp, 0, 0);
        g_thread_num++;
	}
    while (g_thread_num > 0){}

    // 删除临界区
    DeleteCriticalSection(&g_FileQueue_cs);
	DeleteCriticalSection(&g_Cloc_cs);
	DeleteCriticalSection(&g_Thread_cs);

    //输出统计结果
    PrintData();

    system("pause");
    return 0;
}