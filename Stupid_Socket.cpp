#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#include <string>
#include <iostream>
using namespace std;

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "4444"
#define DEFAULT_BUFLEN 512

class stupid_tcp_socket {
private:
	WSADATA wsaData;
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	SOCKET ConnectSocket = INVALID_SOCKET;

	string ip_addr = DEFAULT_IP;
	string port = DEFAULT_PORT;

	// 0 = unitialized, 1 = initalized, 2 = successfully connected
	int socket_state = 0;
public:
	stupid_tcp_socket() {
		WSADATA wsaData;

		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
		}
	}
	
	// AF_INET = 2, SOCK_STREAM = 1, IPPROTO_TCP = 6, IP_ADDR, PORT
	int socket_settings(int ai_family, int ai_socktype, int ai_protocol, const char * ip_addr, const char * port) {
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = ai_family;
		hints.ai_socktype = ai_socktype;
		hints.ai_protocol = ai_protocol;

		int iResult = getaddrinfo(ip_addr, port, &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed: %d\n", iResult);
			WSACleanup();
			return 1;
		}
		this->ip_addr = ip_addr;
		this->port = port;
		return 0;
	}

	int socket_initialize() {
		ptr = result;

		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);

		if (ConnectSocket == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}
		socket_state = 1;
		return 0;
	}

	int start_tcp_connect() {
		if (socket_state != 1) {
			cout << "Socket has not been initialized, connect aborted\nSocket State: " << socket_state << endl;
			return 2;
		}
		int iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
		}

		// Should really try the next address returned by getaddrinfo
		// if the connect call failed
		// But for this simple example we just free the resources
		// returned by getaddrinfo and print an error message

		freeaddrinfo(result);

		if (ConnectSocket == INVALID_SOCKET) {
			printf("Unable to connect to server!\n");
			WSACleanup();
			return 1;
		}
		socket_state = 2;
		return 0;
	}

	int send_tcp_string(char* sendbuf) {
		if (socket_state != 2) {
			cout << "Socket has not been connected, send_tcp_string aborted\nSocket State: " << socket_state << endl;
			return 2;
		}
		// Send an initial buffer
		int iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			return 1;
		}

		printf("Bytes Sent: %ld\n", iResult);
		return 0;
	}

	int close_send_socket() {
		int iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("send socket shutdown failed: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
		return 0;
	}

	int receive_tcp_string() {
		int recvbuflen = DEFAULT_BUFLEN;
		char recvbuf[DEFAULT_BUFLEN];
		// Receive data until the server closes the connection
		
		int iResult;
		do {
			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0)
				printf("Bytes received: %d\n", iResult);
			else if (iResult == 0)
				printf("Connection closed\n");
			else
				printf("recv failed: %d\n", WSAGetLastError());
		} while (iResult > 0);
		return 0;
	}

	void close_socket() {
		closesocket(ConnectSocket);
		socket_state = 1;
	}

	~stupid_tcp_socket() {
		WSACleanup();
	}
};

int main() {
	stupid_tcp_socket mysock;
	string ipaddr = "10.10.10.10";
	string port = "4444";
	mysock.socket_settings(AF_INET, SOCK_STREAM, IPPROTO_TCP, ipaddr.data(), port.data());
	mysock.socket_initialize();
	mysock.start_tcp_connect();
	cout << "test";
	char sengmsg = char("test hello world");
	mysock.send_tcp_string(&sengmsg);
	mysock.close_socket();
	return 0;
}
