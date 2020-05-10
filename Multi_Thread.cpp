#include "Multi_Thread.h"
using namespace std;
//#include <Windows.h>
//#include<sysinfoapi.h>
//#pragma(lib,"Kernel32.lib")

int main()
{
	
	/*SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	ULONG  numOfThread = sysInfo.dwNumberOfProcessors;
	cout << "最佳的线程数目是：" << numOfThread << endl;*/

	threadPool();


	return 0;
}