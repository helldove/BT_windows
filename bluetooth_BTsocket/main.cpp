/*********************************************************
/ Bluetooth test program
/ using Microsoft Bluetooth SDK (BTsocket)
/ install this library :https://msdn.microsoft.com/en-us/library/windows/desktop/aa363058(v=vs.85).aspx
/ Author : SangWook Ha
*********************************************************/

#include <iostream>  
#include <stdlib.h>  
//#include <windows.h>  don't use!!!!
#include <winsock2.h>    //crash with Windows.h  
#include <ws2bth.h>  
#include <atlstr.h>     
#include <process.h>   
#define RCBUFFSIZE	128

//#pragma comment(lib,"ws2_32.lib") //linker library
using namespace std;  



class BTclass {
private:
	WSADATA wsaData;
	WSAQUERYSET wsaQuery;
	LPWSAQUERYSET pwsaResults;
	SOCKET AccSocket;
	int err;

public:
	SOCKADDR_BTH *TargetBT = new SOCKADDR_BTH[10];
	SOCKET LocalSocket;
	HANDLE hThreadReceive;
	BTclass(BOOL chk) {
		err = WSAStartup(MAKEWORD(2, 2), &wsaData);
		LocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
		if (err != 0)
			cout << "WSAStartup failed with error" << err << "\ncheck bluetooth state\n" << endl;
		else {
			cout << "windock 2.2 dll was found okay\n" << endl;
		}
	}
	~BTclass() {
		CloseHandle(hThreadReceive);
		closesocket(LocalSocket);
		delete(TargetBT);
		WSACleanup();
	}
	static UINT WINAPI ReceiveData(LPVOID target) {
		int DataLen = 0;
		char ReceiveBuff[RCBUFFSIZE + 1] = { 0, };
		SOCKET temp = (SOCKET)target;
		int i = 0;
		//cout << "thead start\n" << endl;
		while (true) {
			DataLen = recv(temp, ReceiveBuff, RCBUFFSIZE, 0);
			if (DataLen == SOCKET_ERROR) {
				cout << "data receive failed\n" << endl;
			}
			else if (DataLen != 0) {
				cout << "receive data : " << ReceiveBuff << "\n" << "data length : " << DataLen << "\n" << endl;
			}
			DataLen = 0;
			for (i = 0; i < RCBUFFSIZE + 1; i++) {
				ReceiveBuff[i] = 0;
			}
		}
		return 0;
	}
	void DeviceListScan(void) {
		HANDLE hLookup;
		BOOL bHaveName;
		DWORD dwSize;
		delete(TargetBT);
		TargetBT = new SOCKADDR_BTH[10];
		union { CHAR buf[5000];	SOCKADDR_BTH _Unused_; } butuh;
		dwSize = sizeof(butuh.buf);
		pwsaResults = (LPWSAQUERYSET)butuh.buf;
		dwSize = sizeof(butuh.buf);
		ZeroMemory(&wsaQuery, sizeof(wsaQuery));
		wsaQuery.dwSize = sizeof(wsaQuery);
		wsaQuery.dwNameSpace = NS_BTH;
		wsaQuery.lpcsaBuffer = NULL;
		if (WSALookupServiceBegin(&wsaQuery, LUP_CONTAINERS | LUP_FLUSHCACHE, &hLookup) == SOCKET_ERROR) {
			cout << "WSALookupServiceBegin failed\n" << endl;
			return;
		}
		else {
			ZeroMemory(pwsaResults, sizeof(WSAQUERYSET));
			pwsaResults->dwSize = sizeof(WSAQUERYSET);
			pwsaResults->dwNameSpace = NS_BTH;
			pwsaResults->lpBlob = NULL;
			int i = 0;
			while (WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &dwSize, pwsaResults) == 0) {
				if (i >= 10)
					break;
				CopyMemory((PSOCKADDR_BTH)&TargetBT[i], (PSOCKADDR_BTH)pwsaResults->lpcsaBuffer->RemoteAddr.lpSockaddr, sizeof(TargetBT[i]));
				//TargetBT[i].btAddr = ((SOCKADDR_BTH *)pwsaResults->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr;
				bHaveName = (pwsaResults->lpszServiceInstanceName) && *(pwsaResults->lpszServiceInstanceName);
				cout << i + 1 << ". device name : " << CW2A(pwsaResults->lpszServiceInstanceName) << "\n" << endl;
				i++;
			}
			if (WSALookupServiceEnd(hLookup) != 0)
				cout << "WSALookupServiceEnd failed\n" << endl;
		}
	}
	void DeviceConnect(int deviceNum) {
		TargetBT[deviceNum].addressFamily = AF_BTH;
		TargetBT[deviceNum].serviceClassId = SerialPortServiceClass_UUID; //RFCOMM_PROTOCOL_UUID 
		TargetBT[deviceNum].port = 0;//BT_PORT_ANY;
		if (connect(LocalSocket, (struct sockaddr *)&TargetBT[deviceNum], sizeof(SOCKADDR_BTH)) == SOCKET_ERROR){
			cout << "Device connect error : " << WSAGetLastError() << "\n" << endl;
		}
		hThreadReceive = (HANDLE)_beginthreadex(NULL, 0, ReceiveData, (LPVOID)LocalSocket, 0, NULL);
		if (hThreadReceive == 0)
			cout << "Receive thread error\n" << endl;
		//_beginthreadex_proc_type
	}
	void SendData(char * data) {
		if (send(LocalSocket, data, strlen(data), 0) == SOCKET_ERROR || send(LocalSocket, "\n", strlen("\n"), 0) == SOCKET_ERROR) {
			cout << "\"" << data << "\" send error\n" << endl;
		}
	}
};


int main(int argc, char **argv) {
	bool loopchk = true;
	int menuchk = 0;
	int deviceNum;
	char SendBuff[30];
	cout << "Bluetooth using socket test program \n" << endl;
	BTclass BTtest(true);
	while (loopchk) {
		cout << "\t1. scan device\n\t2. connect device\n\t3. send text\n\t4. exit\n" << endl;
		cin >> menuchk;
		switch (menuchk){
		case 1:
			BTtest.DeviceListScan();
			break;
		case 2:
			cout << "which device? (input device number)\n" << endl;
			cin >> deviceNum;
			BTtest.DeviceConnect(deviceNum - 1);
			//_beginthread((BTtest.ReceiveData), 0, (void *)BTtest.LocalSocket);
			BTtest.SendData("test");
			break;
		case 3:
			cin.ignore(INT_MAX, '\n');
			cout << "input text" << endl;
			cin.getline(SendBuff, sizeof(SendBuff));
			BTtest.SendData((char *)SendBuff);
			break;
		case 4:
			cout << "program exit\n" << endl;
			loopchk = false;
			break;
		default:
			cout << "input right number\n" << endl;
			break;
		}
	}
	return 1;
}