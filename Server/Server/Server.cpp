// HomeWork06.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mylibraly.h"

int main(int argc, char *argv[])
{
	DWORD		nEvents = 0;
	DWORD		index;
	SOCKET		socks[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT	events[WSA_MAXIMUM_WAIT_EVENTS];
	WSANETWORKEVENTS sockEvent;

	SOCKET connSock;
	//Step1: Inittiate WinSock
	InitWinSock();

	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in serverAddr;
	//Set sockaddr
	if (argc == 3) setSockAddrIn(&serverAddr, AF_INET, argv[1], ParseInt(argv[2], strlen(argv[2])));
	else if (argc == 2) setSockAddrIn(&serverAddr, AF_INET, SERVER_ADDR, ParseInt(argv[1], strlen(argv[1])));
	else setSockAddrIn(&serverAddr, AF_INET, SERVER_ADDR, SERVER_PORT);

	socks[0] = listenSock;
	events[0] = WSACreateEvent(); //create new events
	nEvents++;

	// Associate event types FD_ACCEPT and FD_CLOSE
	// with the listening socket and newEvent   
	WSAEventSelect(socks[0], events[0], FD_ACCEPT | FD_CLOSE);

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
	printf_s("Server [%s:%d] started.\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));

	//Step 5: Communicate with client

	file = fopen(ACCOUNT_FILE, "a+");

	if (file == NULL) {
		printf_s("Cann't open file.");
		//Step 5: Close socket
		closesocket(listenSock);
		//Step 6: Terminate Winsock
		WSACleanup();
		return 0;
	}

	listUsers = (Users**)malloc(MAX_USER * sizeof(Users*));
	for (int i = 0; i < MAX_USER; i++)
		listUsers[i] = NULL;
	nUsers = ReadUserFromFile(file, listUsers, MAX_USER);
	fclose(file);
	listStates = InitListPlayer(FD_SETSIZE);

	for (int i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
		socks[i] = 0;
	}
	
	int clientAddrLen = sizeof(clientAddr);
	//accept request

	while (true)
	{
		//wait for network events on all socket
		index = WSAWaitForMultipleEvents(nEvents, events, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("WSAWaitForMultipleEvents() failed with error %d\n", WSAGetLastError());
			break;
		}

		index = index - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(socks[index], events[index], &sockEvent);

		if (sockEvent.lNetworkEvents & FD_ACCEPT) {
			if (sockEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
				printf("FD_ACCEPT failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				break;
			}

			if ((connSock = accept(socks[index], (sockaddr *)&clientAddr, &clientAddrLen)) == SOCKET_ERROR) {
				printf("accept() failed with error %d\n", WSAGetLastError());
				break;
			}

			//Add new socket into socks array
			int i;
			if (nEvents == WSA_MAXIMUM_WAIT_EVENTS) {
				printf("\nToo many clients.");
				closesocket(connSock);
			}
			else
				for (i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++)
					if (socks[i] == 0) {
						socks[i] = connSock;
						listStates[i]->client = socks[i];
						listStates[i]->nError = 3;
						listStates[i]->status = STEP_LOGIN;
						events[i] = WSACreateEvent();
						WSAEventSelect(socks[i], events[i], FD_READ | FD_CLOSE);
						nEvents++;
						break;
					}

			//reset event
			WSAResetEvent(events[index]);
		}

		if (sockEvent.lNetworkEvents & FD_READ) {
			//Receive message from client
			if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
				printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				break;
			}

			int ret = Action(socks[index]);

			//Release socket and event if an error occurs
			if (ret == CLOSE_CLIENT) {
				closesocket(socks[index]);
				socks[index] = 0;
				WSACloseEvent(events[index]);
				DeleteSocketEvent(socks, events, nEvents, index);
				nEvents--;
				continue;
			}
			else {				
				//reset event
				WSAResetEvent(events[index]);
			}
		}

		if (sockEvent.lNetworkEvents & FD_CLOSE) {
			if (sockEvent.iErrorCode[FD_CLOSE_BIT] != 0) {
				printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_CLOSE_BIT]);
				continue;
			}
			//Release socket and event
			closesocket(socks[index]);
			socks[index] = 0;
			WSACloseEvent(events[index]);
			DeleteSocketEvent(socks, events, nEvents, index);
			nEvents--;
		}
	}

	//Step 5: Close socket
	closesocket(listenSock);

	_getch();
	//Step 6: Terminate Winsock
	WSACleanup();
	return 0;
}