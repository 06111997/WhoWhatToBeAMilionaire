// HomeWork06.cpp : Defines the entry point for the console application.
//

#include "thread.h"
#include "stdafx.h"


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
		playersPlaying[i] = NULL;
	}

	for (int i = 0; i < MAX_CLIENT_WAITTING; i++) {
		playersWaitting[i] = NULL;
	}

	for (int i = 0; i < MAX_THREAD; i++) {
		threadWaittingHandle[i] = 0;
	}
	nThreadWaitting = 0;
	threadPlayingHandle = 0;

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
			EnterCriticalSection(&waitCrt);
			for (index = 0; index < MAX_CLIENT_WAITTING; index++)
				if (playersWaitting[index] == NULL) break;
			LeaveCriticalSection(&waitCrt);

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

				playersWaitting[index] = InitPlayer(connSock);
				eventsPlaying[index] = WSACreateEvent();

				LeaveCriticalSection(&waitCrt);

				//Post Receive request
				WSARecv(playersWaitting[index]->client, &playersWaitting[index]->dataBuff, 1, &playersWaitting[index]->recvByte, &flags, &playersWaitting[index]->overlap, NULL);

				//Check if client have no thread to manage
				if (threadWaittingHandle[indexOfThread] == 0) {
					EnterCriticalSection(&threadCrt);
					//Create new thread
					InitializeCriticalSection(&threadWaiting[indexOfThread].criticle);
					threadWaiting[indexOfThread].nEvent = 1;
					threadWaiting[indexOfThread].tEvents = &eventsWaitting[indexOfThread * WSA_MAXIMUM_WAIT_EVENTS];
					threadWaiting[indexOfThread].indexOfThread = indexOfThread;
					threadWaiting[indexOfThread].arrCrt = &waitCrt;
					threadWaiting[indexOfThread].listPlayers = (Players **) &(playersWaitting[indexOfThread * WSA_MAXIMUM_WAIT_EVENTS]);
					threadWaittingHandle[indexOfThread] = (HANDLE)_beginthreadex(0, 0, WaittingThread, (LPVOID)&threadWaiting[indexOfThread], 0, 0);
					nThreadWaitting++;
					LeaveCriticalSection(&threadCrt);

				}
				else {
					//Thread manage client index is already exist
					//EnterCricticleSection
					//Must fix it
					EnterCriticalSection(&threadWaiting[indexOfThread].criticle);
					threadWaiting[indexOfThread].nEvent++;
					LeaveCriticalSection(&threadWaiting[indexOfThread].criticle);
				}

			}
		}
	}

	//Wait for all thread done
	WaitForMultipleObjects(nThreadWaitting, threadWaittingHandle, TRUE, INFINITE);
	WSACleanup();
	return 0;
}