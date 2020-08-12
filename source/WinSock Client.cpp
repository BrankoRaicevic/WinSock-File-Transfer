#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define FILE_TRANSFER_FLAG 0x01
#define FILE_ACK_FLAG 0x11

int __cdecl main(int argc, char** argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL,
		* ptr = NULL,
		hints;
	char sendbuf[DEFAULT_BUFLEN];
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	std::fstream fp;

	// Validate the parameters
	if (argc != 4) {
		printf("usage: %s server-name/address type argument\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// Send an initial buffer
	char byte;
	int check = 0;
	bool doTransfer = false;
	// -f for file
	if (!strcmp(argv[2], "-f")) {
		// Telling the server i want to send a file
		sendbuf[0] = FILE_TRANSFER_FLAG;
		sendbuf[1] = 0;
		strcat_s(sendbuf, argv[3]);
		sendbuf[strlen(sendbuf) + 1] = 0;
		iResult = send(ConnectSocket, sendbuf, strlen(sendbuf) + 1, 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
		// Zeroing buffer for my own personal safety
		recvbuf[0] = 0;
		recvbuf[1] = 0;
		recvbuf[2] = 0;
		while (1) {
			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				printf("Bytes received: %d\n", iResult);
				int answer = (int)recvbuf[0];
				if (answer == FILE_ACK_FLAG) {
					doTransfer = true;
				}
				else {
					std::cout << "Error - wrong answer flag" << std::endl;
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}
				break;
			}
			else if (iResult == 0) {
				printf("No bytes recieved, continuing\n");
			}
			else {
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}
		}
		// Sending the file

		if (doTransfer) {
			fp.open(argv[3], std::ios::in | std::ios::binary);
			if (!fp) {
				std::cout << ">> " << argv[3] << " <- file doesn't exist" << std::endl;
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}
			while (!fp.eof()) {
				fp.read(&byte, 1);
				sendbuf[check] = byte;
				check++;
				if (check >= 500) {
					iResult = send(ConnectSocket, sendbuf, check, 0);
					check = 0;
					if (iResult == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ConnectSocket);
						WSACleanup();
						return 1;
					}

					printf("Bytes Sent: %ld\n", iResult);


					//iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
					//if (iResult > 0)
					//	printf("Bytes received: %d\n", iResult);
					//else if (iResult == 0)
					//	printf("No bytes recieved, continuing\n");
					//else
					//	printf("recv failed with error: %d\n", WSAGetLastError());
				}
			}
			if (check != 0) {
				iResult = send(ConnectSocket, sendbuf, check - 1, 0);
				if (iResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(ConnectSocket);
					WSACleanup();
					return 1;
				}

				printf("Bytes Sent: %ld\n", iResult);
			}


			printf("Bytes Sent: %ld\n", iResult);
			fp.close();
		}
	}
	else {
		std::cout << ">> " << argv[2] <<  " <- Invalid argument" << std::endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// Receive until the peer closes the connection
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}