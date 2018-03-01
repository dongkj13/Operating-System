# include <stdio.h>
# include <stdlib.h>
# include <windows.h>
# include <iostream>
# include <fstream>
# include <string>
using namespace std;

HANDLE AllocatorSemaphore, TrackSemaphore;
LPVOID start;
ofstream out("output.txt");

void printSystemInfo()
{
	SYSTEM_INFO info;  //系统消息
    GetSystemInfo (&info);
	out<<"系统消息"<<endl;
	//指定页面的大小和页面保护和委托的颗粒。这是被 VirtualAlloc 函数使用的页大小。
    out<<"dwPageSize"<<"\t"<<info.dwPageSize<<endl;
	//指向应用程序和动态链接库(DLL)可以访问的最低内存地址。
	out<<"lpMinimumapplicationAddress"<<"\t"<<info.lpMinimumApplicationAddress<<endl;
	//指向应用程序和动态链接库(DLL)可以访问的最高内存地址。
    out<<"lpMaximumApplicationAddress"<<"\t"<<info.lpMaximumApplicationAddress<<endl;
	//指定一个用来代表这个系统中装配了的中央处理器的掩码。二进制0位是处理器0；31位是处理器31。
    out<<"dwActiveProcessorMask"<<"\t"<<info.dwActiveProcessorMask<<endl;
	//指定系统中的处理器的数目。
    out<<"dwNumberOfProcessors"<<"\t"<<info.dwNumberOfProcessors<<endl;    
	//指定系统中中央处理器的类型。
	out<<"dwProcessorType"<<"\t"<<info.dwProcessorType<<endl;
	//指定已经被分配的虚拟内存空间的粒度。
	out<<"dwAllocationGranularity"<<"\t"<<info.dwAllocationGranularity<<endl;
	//指定系统体系结构依赖的处理器级别。
	out<<"wProcessorLevel"<<"\t"<<info.wProcessorLevel<<endl;
	//指定系统体系结构依赖的处理器修订版本号。下表显示了对于每一种处理器体系，处理器的修订版本号是如何构成的。
	out<<"wProcessorRevision"<<"\t"<<info.wProcessorRevision<<endl;
	out<<"**************************************************"<<endl;
}

void printMemoryStatus()
{
	MEMORYSTATUS status; //内存状态
	GlobalMemoryStatus (&status);
	out<<"内存状态"<<endl;
	////结构体的长度
	//out<<"dwLength"<<"\t"<<status.dwLength<<endl;
	////物理内存的使用情况，0-100
	//out<<"dwMemoryLoad"<<"\t"<<status.dwMemoryLoad<<endl;
	////物理内存的大小
	//out<<"dwTotalPhy"<<"\t"<<status.dwTotalPhys<<endl;
	//当前可用的物理内存大小
	out<<"dwAvailPhys"<<"\t"<<status.dwAvailPhys / 1024<<"KB"<<endl;
	////可调拨的物理存储器的大小：物理内存+页交换文件
	//out<<"dwTotalPageFile"<<"\t"<<status.dwTotalPageFile<<endl;
	//当前可调拨的物理存储器的大小
	out<<"dwAvailPageFile"<<"\t"<<status.dwAvailPageFile / 1024<<"KB"<<endl;
	////进程虚拟地址空间中用户模式分区的大小
	//out<<"dwTotalVirtual"<<"\t"<<status.dwTotalVirtual<<endl;
	//进程虚拟地址空间中用户模式分区可用的大小
	out<<"dwAvailVirtual"<<"\t"<<status.dwAvailVirtual / 1024<<"KB"<<endl;
	//out<<"**************************************************"<<endl;
}

string getProtect(DWORD dwProtect)
{
	switch (dwProtect)
	{
	case PAGE_NOACCESS : return "PAGE_NOACCESS";
	case PAGE_READONLY : return "PAGE_READONLY";
	case PAGE_READWRITE: return "PAGE_READWRITE";
	case PAGE_EXECUTE : return "PAGE_EXECUTE";
	case PAGE_EXECUTE_READ: return "PAGE_EXECUTE_READ";
	case PAGE_EXECUTE_READWRITE : return "PAGE_EXECUTE_READWRITE";
	}
	return "does not have access";
}

string getState(DWORD dwState)
{
	switch (dwState)
	{
	case MEM_COMMIT: return "MEM_COMMIT";
	case MEM_FREE: return "MEM_FREE";
	case MEM_RESERVE: return "MEM_RESERVE";
	}
}

void printMemoryInfo(LPVOID Address)
{
	SYSTEM_INFO info;  //系统消息
    GetSystemInfo (&info);
	MEMORY_BASIC_INFORMATION mem; //内存基本信息
	VirtualQuery (Address, &mem, sizeof(MEMORY_BASIC_INFORMATION) );
	out<<"内存基本信息"<<endl;
	// //查询内存块所占的第一个页面基地址
	//out<<"BaseAddress"<<"\t"<<mem.BaseAddress<<endl;
	////内存块所占的第一块区域基地址，小于等于BaseAddress，也就是说BaseAddress一定包含在AllocationBase分配的范围内
	//out<<"AllocationBase"<<"\t"<<mem.AllocationBase<<endl;
	//区域被初次保留时赋予的保护属性
	out<<"AllocationProtect"<<"\t"<<getProtect(mem.AllocationProtect)<<endl;
	//从BaseAddress开始，具有相同属性的页面的大小
	out<<"RegionSize"<<"\t"<<mem.RegionSize / 1024 <<"KB"<<endl;
	//页面的状态，有三种可能值：MEM_COMMIT、MEM_FREE和MEM_RESERVE
	out<<"State"<<"\t"<<getState(mem.State)<<endl;
	//页面的属性，其可能的取值与AllocationProtect相同
	out<<"Protect"<<"\t"<<getProtect(mem.Protect)<<endl;
	////该内存块的类型，有三种可能值：MEM_IMAGE、MEM_MAPPED、MEM_PRIVATE
	//out<<"Type"<<"\t"<<mem.Type<<endl;
	out<<"**************************************************"<<endl;
}

DWORD WINAPI Tracker(LPVOID param)
{
	for (int i = 0;i < 30;i++)
	{
		WaitForSingleObject(TrackSemaphore, INFINITE);	//请求trac信号量
		printMemoryStatus();	//打印内存状态信息
		printMemoryInfo(start);	//打印内存基本信息
		ReleaseSemaphore(AllocatorSemaphore, 1, NULL);	//释放allo信号量
	}
	return 0;
}

DWORD WINAPI Allocator(LPVOID param)
{
	SYSTEM_INFO info;
	GetSystemInfo (&info);
	DWORD Protect[5] = {PAGE_READONLY, PAGE_READWRITE, PAGE_EXECUTE, PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE};
	string protect[5] = {"PAGE_READONLY", "PAGE_READWRITE", "PAGE_EXECUTE", "PAGE_EXECUTE_READ", "PAGE_EXECUTE_READWRITE"};
	long size;
	out<<"模拟内存分配开始："<<endl;
	printMemoryStatus();
	out<<"**************************************************"<<endl;
	for (int j = 0; j<30; j++)
	{
		//请求allo信号量
		WaitForSingleObject(AllocatorSemaphore, INFINITE); 
		int i = j % 6;
		int Pages = j / 6 + 1;
		if (i == 0)
		{
			out<<"保留内存"<<endl;
			start = VirtualAlloc(NULL, Pages * info.dwPageSize, MEM_RESERVE, PAGE_NOACCESS);
			size = Pages * info.dwPageSize;
			out<<"起始地址："<<start<<"\t"<<"内存大小："<<size / 1024<<"KB"<<endl;
		}
		if (i == 1)
		{
			out<<"提交内存"<<endl;
			start = VirtualAlloc(start, size, MEM_COMMIT, Protect[int(j / 6)]);
			out<<"起始地址："<<start<<"\t"<<"内存大小："<<size / 1024<<"KB"<<endl;
			out<<"保护级别："<<protect[j / 6]<<endl;
		}
		if (i == 2)
		{
			out<<"锁定内存"<<endl;
			out<<"起始地址："<<start<<"\t"<<"内存大小："<<size / 1024<<"KB"<<endl;
			if (!VirtualLock(start, size))
				out<<GetLastError()<<endl;
		}
		if (i == 3)
		{
			out<<"解锁内存"<<endl;
			out<<"起始地址："<<start<<"\t"<<"内存大小："<<size / 1024<<"KB"<<endl;
			if (!VirtualUnlock(start, size))
				out<<GetLastError()<<endl;	 
		}
		if (i == 4)
		{
			out<<"回收内存"<<endl;
			out<<"起始地址："<<start<<"\t"<<"内存大小："<<size / 1024<<"KB"<<endl;
			if (!VirtualFree(start, size, MEM_DECOMMIT))
				out<<GetLastError()<<endl;	 
		}
		if (i == 5)
		{
			out<<"释放内存"<<endl;
			out<<"起始地址："<<start<<"\t"<<"内存大小："<<size / 1024<<"KB"<<endl;
			if (!VirtualFree(start, 0, MEM_RELEASE))
				out<<GetLastError()<<endl;	
		}
		//释放trac信号量
		ReleaseSemaphore(TrackSemaphore, 1, NULL);
	}
	
	Sleep(1000);
	return 0;

}

int main()
{
	out<<"程序开始："<<endl;
	//printSystemInfo();
	//printMemoryInfo();
	//printMemoryStatus();

	AllocatorSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
	TrackSemaphore = CreateSemaphore(NULL, 0, 1, NULL);

	DWORD TrackID, AllocatorID;
	HANDLE TrackerHandle = CreateThread(NULL, 0, Tracker, NULL, 0, &TrackID);
	HANDLE AllocatorHandle = CreateThread(NULL, 0, Allocator, NULL, 0, &AllocatorID);

	if (TrackerHandle != NULL)
	{
		WaitForSingleObject(TrackerHandle, INFINITE);
		CloseHandle(TrackerHandle);
	}		
	if (AllocatorHandle != NULL)
	{
		WaitForSingleObject(AllocatorHandle, INFINITE);
		CloseHandle(AllocatorHandle);
	}
	out<<"程序结束："<<endl;
	printMemoryStatus();
}