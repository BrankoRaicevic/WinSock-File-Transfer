#undef UNICODE

#define WIN32_LEAN_AND_MEAN

//#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <fstream>
#include <iostream>
#include <string>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

unsigned __stdcall ClientSession(void* data);

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	fd_set readFDs = { 0 };
	TIMEVAL tv = { 0 };

	DWORD dwRecvTimeout = 30000, // Milliseconds
		dwSendTimeout = 30000;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	// No longer need server socket
	//closesocket(ListenSocket);
	while (1) {
		tv.tv_sec = 5;
		FD_ZERO(&readFDs);
		FD_SET(ListenSocket, &readFDs);
		int iSelectResult = select(0, &readFDs, NULL, NULL, &tv);
		if (iSelectResult) {
			if (FD_ISSET(ListenSocket, &readFDs)) {
				ClientSocket = accept(ListenSocket, (SOCKADDR*)&addr, &addrlen);
				if (ClientSocket == INVALID_SOCKET) {
					printf("accept failed with error: %d\n", WSAGetLastError());
					closesocket(ListenSocket);
					WSACleanup();
					return 1;
				}
				else
				{
					// Set Recv Timeout
					setsockopt(ClientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&dwRecvTimeout, sizeof(dwRecvTimeout));

					// Set Send Timeout                 
					setsockopt(ClientSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&dwSendTimeout, sizeof(dwSendTimeout));

					// Process Client Request(s)
					// HandleConnection( ClientSocket );
				}
				// Create a new thread for the accepted client (also pass the accepted client socket)
				char ipinput[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(addr.sin_addr), ipinput, INET_ADDRSTRLEN);
				printf("Accepted Connection from :  %s\n", ipinput);
				unsigned threadID;
				HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)ClientSocket, 0, &threadID);
			}
		}
	}
	return 0;
}

unsigned __stdcall ClientSession(void* data) {
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	char sendbuf[DEFAULT_BUFLEN];
	int sendbuflen = DEFAULT_BUFLEN;

#define FILE_TRANSFER_FLAG 0x01
#define FILE_ACK_FLAG 0x11

	//std::fstream fp("result.jpg", std::ios::out | std::ios::binary);
	std::string recieveFlag;
	//char* filename;
	//bool doTransfer = false;

	ClientSocket = (SOCKET)data;
	// Recieving flag

	//do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
		}
		else if (iResult == 0)
			printf("Going forward...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
		recieveFlag += recvbuf;
	//} while (iResult > 0);

	if (recieveFlag[0] == FILE_TRANSFER_FLAG) {
		recieveFlag = recieveFlag.substr(1);
		//doTransfer = true;
		//recieveFlag.copy(filename, 2);
	}
	else {
		printf("Error - invalid flag\n");
		closesocket(ClientSocket);
		WSACleanup();
	}

	// Sending ACK flag
	sendbuf[0] = FILE_ACK_FLAG;
	iResult = send(ClientSocket, sendbuf, 1, 0);

	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	std::fstream fp(recieveFlag.c_str(), std::ios::out | std::ios::binary);
	if (!fp) {
		std::cout << "Error - file couldn't be created" << std::endl;
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	do {

		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			for (int i = 0; i < iResult; i++) {
				fp.write(&recvbuf[i], 1);
			}
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);
	fp.close();
	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	//WSACleanup();

	return 0;
}