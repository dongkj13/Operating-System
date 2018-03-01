#include <iostream>
#include <fstream>
#include <windows.h>
#include <vector>
#include <time.h>

using namespace std;

const int maxCustomerNo = 20;					//假设所有顾客数的上限
const int ServerNo = 2;							//柜员数量
int Interval = 500;								//为写程序方便，对顾客到来时间统一乘以一个系数

vector<int> LeaveList;							//顾客离开的列表，当所有顾客离开时，程序终止
vector<int> WaitingList;						//顾客等待的列表，便于柜员依次叫号
int come_time[maxCustomerNo];					//顾客到来的时间
int wait_time[maxCustomerNo];					//顾客需要被服务的时间
int serve_time[maxCustomerNo];					//顾客开始被服务的时间
int leave_time[maxCustomerNo];					//顾客离开的时间
int server_id[maxCustomerNo];					//顾客被服务的柜员编号

HANDLE Server_mutex, Customer_mutex;			//柜员互斥锁，顾客互斥锁
HANDLE Server_Semaphore, Customer_Semaphore;	//柜员信号量，顾客信号量
HANDLE disp;									//用于控制屏幕打印的互斥锁

ofstream out("output.txt");

//Server线程的参数
struct ServerParameter
{
	int ID;
};

//Customer线程的参数
struct CustomerParameter{
	int ID;
	int come_time;
	int wait_time;
};

//柜员线程
DWORD WINAPI Server(LPVOID lpPara)
{
	ServerParameter *SP = (ServerParameter *)lpPara;
	while (true)
	{
		WaitForSingleObject(Customer_Semaphore, INFINITE); 
		WaitForSingleObject(Server_mutex, INFINITE);
		int ID = WaitingList[0];							//获取第一个等待的顾客编号
		serve_time[ID] = int(clock());						//当前时间即为开始服务时间	
		server_id[ID] = SP->ID;								//记录服务的柜员编号
		WaitingList.erase(WaitingList.begin());				//从等待列表中删除该顾客

		
		WaitForSingleObject(disp, INFINITE);
		out <<"Customer "<< ID << " is being servered by Server " << SP->ID<<'.'<< endl;
		cout <<"Customer "<< ID << " is being servered by Server " << SP->ID<<'.'<< endl;
		ReleaseMutex(disp);
		ReleaseMutex(Server_mutex); 
		
		Sleep(wait_time[ID]);								//模拟顾客正在被服务
		
		WaitForSingleObject(Server_mutex, INFINITE);
		WaitForSingleObject(disp, INFINITE);
		out <<"Customer "<< ID << " is serverd." << endl;
		cout <<"Customer "<< ID << " is serverd." << endl;
		ReleaseMutex(disp);
		
		leave_time[ID] = int(clock());						//当前时间即为顾客离开时间
		LeaveList.erase(LeaveList.begin());					//从离开列表中删除该顾客
		ReleaseMutex(Server_mutex); 
		ReleaseSemaphore(Server_Semaphore, 1, NULL); 
	}

	return 0;
}

//顾客线程
DWORD WINAPI Customer(LPVOID lpPara)
{
	CustomerParameter *CP = (CustomerParameter *)lpPara;
	//顾客到来
	WaitForSingleObject(disp, INFINITE);
	out << "Customer " << CP->ID << " is coming." << endl;
	cout << "Customer " << CP->ID << " is coming." << endl;
	ReleaseMutex(disp);
	//在等待列表和离开列表末尾添加该顾客编号
	WaitingList.push_back(CP->ID);
	LeaveList.push_back(CP->ID);
	//记录顾客到来时间
	int come_clock = int(clock());

	WaitForSingleObject(Server_Semaphore, INFINITE); 
	WaitForSingleObject(Customer_mutex, INFINITE);
	//记录顾客等待时间
	int wait_clock = floor((int(clock()) - come_clock) / Interval);
	WaitForSingleObject(disp, INFINITE);
	out << "After " << wait_clock << " seconds, " << "Customer " << CP->ID << " is called." << endl;
	cout << "After " << wait_clock << " seconds, " << "Customer " << CP->ID << " is called." << endl;
	ReleaseMutex(disp);
	ReleaseMutex(Customer_mutex);  
	ReleaseSemaphore(Customer_Semaphore, 1, NULL); 
	return 0;
}


int main()
{
	int init_time = int(clock());
	//柜员线程与顾客线程的线程ID
	DWORD ServerID[ServerNo];
	DWORD CustomerID[maxCustomerNo];
	//线程句柄
	HANDLE ServerHandle[ServerNo];
	HANDLE CustomerHandle[maxCustomerNo];

	disp = CreateMutex(NULL, FALSE, NULL);

	Server_mutex = CreateMutex(NULL, FALSE, NULL);
	Customer_mutex = CreateMutex(NULL, FALSE, NULL);
	Server_Semaphore = CreateSemaphore (NULL, ServerNo, ServerNo, NULL);
    Customer_Semaphore = CreateSemaphore (NULL, 0, ServerNo, NULL);
	//创建柜员线程
	ServerParameter SP[ServerNo];
	for (int i = 0; i < ServerNo; i++)
	{
		SP[i].ID = i;
		ServerHandle[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Server, &SP[i], 0, &ServerID[i]);
        if (ServerHandle[i]==NULL) return -1;
	}

	//读入顾客到来信息，并创建顾客线程
	int n;
	ifstream file("Customer.txt");
	double last_time = 0;
	CustomerParameter CP[maxCustomerNo];
	while (!file.eof())
	{
		file >> n;
		file >> come_time[n] >> wait_time[n];
		come_time[n] = come_time[n] * Interval;
		wait_time[n] = wait_time[n] * Interval;
		CP[n].ID = n;
		CP[n].come_time = come_time[n];
		CP[n].wait_time = wait_time[n];
		int t = (come_time[n] - last_time);
		//如果顾客同时到来，t可能为负值
		if (t < 0) t = 0;
		//模拟下一个顾客到来的时间
		Sleep(t);
		CustomerHandle[n] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Customer, &CP[n], 0, &CustomerID[n]);
		last_time = int(clock()) - init_time;
	}
	//等待直到顾客线程执行完成
	for (int i = 1; i <= n; i++)
		if (CustomerHandle[i] != NULL)
		{
			WaitForSingleObject(CustomerHandle[i], INFINITE);
			CloseHandle(CustomerHandle[1]);
		}

	//等待直到柜员线程执行完成
	while (LeaveList.size() != 0)
	{
		if (LeaveList.size() == 0)
			for (int i = 0; i < ServerNo; i++)
				CloseHandle(ServerHandle);
	}
	//输出结果
	for (int i = 1; i <= n; i++)
	{
		come_time[i] = floor(come_time[i] / Interval);
		serve_time[i] = floor(serve_time[i] / Interval);
		wait_time[i] = floor(wait_time[i] / Interval);
		leave_time[i] = floor(leave_time[i] / Interval);
		out<< i <<" & "<< come_time[i]<<" & "<<serve_time[i]<<" & "<<wait_time[i]<<" & "<<leave_time[i]<<" & "<<server_id[i] << endl;
		cout<< i <<' '<< come_time[i]<<' '<<serve_time[i]<<' '<<wait_time[i]<<' '<<leave_time[i]<<' '<<server_id[i] << endl;
	}

}