// HomeWork06.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "thread.h"

int main(int argc, char *argv[])
{
	SOCKET connSock;
	//Step1: Inittiate WinSock
	InitWinSock();

	SOCKET listenSock;
	listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSock == INVALID_SOCKET) {
		printf("Can not init socket.\n");
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddr;
	//Set sockaddr
	if (argc == 3) setSockAddrIn(&serverAddr, AF_INET, argv[1], ParseInt(argv[2], strlen(argv[2])));
	else if (argc == 2) setSockAddrIn(&serverAddr, AF_INET, SERVER_ADDR, ParseInt(argv[1], strlen(argv[1])));
	else setSockAddrIn(&serverAddr, AF_INET, SERVER_ADDR, SERVER_PORT);


	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(sockaddr_in)))
	{
		printf_s("Error! Cannot bind this address.\n");
		_getch();
		return 0;
	}

	//Step4: Listen request from client
	if (listen(listenSock, 10))
	{
		printf_s("Error! Cannot listen.");
		_getch();
		return 0;
	}

	//Create event to listenSocket
	WSAEVENT listenEvent;
	if ((listenEvent = WSACreateEvent()) == WSA_INVALID_EVENT) {
		//Show error and exit
		printf("WSACreateEvent() failed with error: %d\n", WSAGetLastError());
		closesocket(listenSock);
		WSACleanup();
		return 1;
	}

	printf_s("Server [%s:%d] started.\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));

	//Step 5: Communicate with client


	//Initialze cricticle sections
	InitializeCriticalSection(&userCrt);
	InitializeCriticalSection(&threadCrt);
	InitializeCriticalSection(&playCrt);
	InitializeCriticalSection(&waitCrt);

	for (int i = 0; i < MAX_CLIENT_PLAYING; i++) {
		clientPlaying[i] = NULL;
	}

	for (int i = 0; i < MAX_CLIENT_WAITTING; i++) {
		clientWaitting[i] = NULL;
	}

	for (int i = 0; i < MAX_THREAD; i++) {
		threadHandle[i] = 0;
	}
	nThread = 0;

	int index;
	SOCKET connSock;
	DWORD flags = 0;
	
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (TRUE) {
		//Accept a new client
		

		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
		if (connSock == SOCKET_ERROR) {
			//Accept() failed
			printf("accept() failed with error: %d\n", WSAGetLastError());
			closesocket(listenSock);
			break;
		}
		else {
			//Accepted a new client
			//Find space of new section
			
			index = nWaitting;

			if (index == MAX_CLIENT_WAITTING) {
				//To many client connect to sever
				printf("Too many client\n");
				continue;
			}
			else {
				int indexOfThread = index / WSA_MAXIMUM_WAIT_EVENTS;
				printf("Connect to client with socket: %d\n", connSock);

				//Create section and event to new client
				EnterCriticalSection(&waitCrt);

				clientWaitting[index] = InitPlayer(connSock);
				eventsPlaying[index] = WSACreateEvent();
				nWaitting++;

				LeaveCriticalSection(&waitCrt);

				//Post Receive request
				WSARecv(clientWaitting[index]->client, &clientWaitting[index]->dataBuff, 1, &clientWaitting[index]->recvByte, &flags, &clientWaitting[index]->overlap, NULL);

				//Check if client have no thread to manage
				if (threadHandle[indexOfThread] == 0) {
					EnterCriticalSection(&threadCrt);
					//Create new thread
					InitializeCriticalSection(&threads[indexOfThread].criticle);
					threads[indexOfThread].nEvent = 1;
					threads[indexOfThread].tEvents = &eventsWaitting[indexOfThread * WSA_MAXIMUM_WAIT_EVENTS];
					threads[indexOfThread].indexOfThread = indexOfThread;
					threadHandle[indexOfThread] = (HANDLE)_beginthreadex(0, 0, workerThread, (LPVOID)&threads[indexOfThread], 0, 0);
					nThread++;
					LeaveCriticalSection(&threadCrt);

				}
				else {
					//Thread manage client index is already exist
					//EnterCricticleSection
					//Must fix it
					EnterCriticalSection(&threads[indexOfThread].criticle);
					threads[indexOfThread].nEvent++;
					LeaveCriticalSection(&threads[indexOfThread].criticle);
				}

			}
		}
	}

	//Wait for all thread done
	WaitForMultipleObjects(nThread, threadHandle, TRUE, INFINITE);
	WSACleanup();
	return 0;
}