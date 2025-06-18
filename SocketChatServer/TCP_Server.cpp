#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <map>
#include <sstream>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

struct ClientPair
{
	SOCKET input = INVALID_SOCKET;
	SOCKET output = INVALID_SOCKET;
	std::string nickname;
	std::mutex mutex;
	ClientPair() : input(INVALID_SOCKET), output(INVALID_SOCKET), nickname() {}
};

std::map<std::string, ClientPair> tokenToClient;
std::mutex global_mutex;

const char* EXIT_SIGNAL = "__EXIT__";

void handle_input(const std::string& token)
{
	SOCKET input_sock, output_sock;
	std::string nickname;
	{
		std::lock_guard<std::mutex> lock(global_mutex);
		input_sock = tokenToClient[token].input;
		output_sock = tokenToClient[token].output;
		nickname = tokenToClient[token].nickname;
	}

	char buf[1024];
	while (true)
	{
		int n = recv(input_sock, buf, sizeof(buf) - 1, 0);
		if (n > 0)
		{
			buf[n] = 0;
			std::string msg = nickname + " : " + std::string(buf, n);

			if (std::string(buf, n) == "/exit")
			{
				if (output_sock != INVALID_SOCKET)
				{
					send(output_sock, EXIT_SIGNAL, static_cast<int>(strlen(EXIT_SIGNAL)), 0);
					closesocket(output_sock);
				}
				break;
			}

			if (output_sock != INVALID_SOCKET)
			{
				send(output_sock, msg.c_str(), static_cast<int>(msg.size()), 0);
			}
		}
		else
		{
			if (output_sock != INVALID_SOCKET)
			{
				send(output_sock, EXIT_SIGNAL, static_cast<int>(strlen(EXIT_SIGNAL)), 0);
				closesocket(output_sock);
			}
			break;
		}
	}
	closesocket(input_sock);

	std::lock_guard<std::mutex> lock(global_mutex);
	tokenToClient.erase(token);
}

void handle_output(const std::string& token)
{
	SOCKET input_sock, output_sock;
	std::string nickname;
	{
		std::lock_guard<std::mutex> lock(global_mutex);
		input_sock = tokenToClient[token].input;
		output_sock = tokenToClient[token].output;
	}

	char buf[1024];
	while (true)
	{
		int n = recv(output_sock, buf, sizeof(buf) - 1, 0);
		if (n > 0)
		{
			buf[n] = 0;
			if (std::string(buf) == EXIT_SIGNAL)
			{
				if (input_sock != INVALID_SOCKET)
				{
					send(input_sock, EXIT_SIGNAL, static_cast<int>(strlen(EXIT_SIGNAL)), 0);
					closesocket(input_sock);
				}
				break;
			}
		}
		else
		{
			if (input_sock != INVALID_SOCKET)
			{
				send(input_sock, EXIT_SIGNAL, static_cast<int>(strlen(EXIT_SIGNAL)), 0);
				closesocket(input_sock);
			}
			break;
		}
	}
	closesocket(output_sock);

	std::lock_guard<std::mutex> lock(global_mutex);
	tokenToClient.erase(token);
}

void ClientThread(SOCKET client_sock)
{
	char firstline[256] = { 0 };
	int n = recv(client_sock, firstline, sizeof(firstline) - 1, 0);

	if (n <= 0)
	{
		closesocket(client_sock);
		return;
	}

	firstline[n] = 0;

	// [토큰] [INPUT/OUTPUT] [닉네임]
	std::istringstream iss(firstline);
	std::string token, type, nickname;
	iss >> token >> type >> nickname;

	if (token.empty() || type.empty() || nickname.empty())
	{
		closesocket(client_sock);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(global_mutex);
		if (tokenToClient.find(token) == tokenToClient.end())
		{
			tokenToClient[token];
		}
		if (type == "INPUT")
		{
			tokenToClient[token].input = client_sock;
			tokenToClient[token].nickname = nickname;
		}
		else if (type == "OUTPUT")
		{
			tokenToClient[token].output = client_sock;
			tokenToClient[token].nickname = nickname;
		}
		else
		{
			closesocket(client_sock);
			return;
		}
	}

	// 스레드 분기
	
	if (type == "INPUT")
	{
		std::thread(handle_input, token).detach();
	}
	else if (type == "OUTPUT")
	{
		std::thread(handle_output, token).detach();
	}
}

int main()
{
	// 1. Winsock 초기화
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup 실패" << std::endl;
		return 1;
	}

	// 2. 소켓 생성
	SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == INVALID_SOCKET)
	{
		std::cout << "소켓 생성 실패" << std::endl;
		WSACleanup();
		return 1;
	}

	// 3. 구조체 준비 및 바인딩
	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(8888);


	if (bind(server_sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		std::cout << "바인드 실패" << std::endl;
		closesocket(server_sock);
		WSACleanup();
		return 1;
	}

	// 4. 리슨
	if (listen(server_sock, 5) == SOCKET_ERROR)
	{
		std::cout << "리슨 실패" << std::endl;
		closesocket(server_sock);
		WSACleanup();
		return 1;
	}

	std::cout << "채팅 서버 시작. 포트 8888에서 대기 중..." << std::endl;

	// 5. 연결 수락 및 데이터 처리
	while (true)
	{
		bool exitCall = false;
		SOCKET client_sock = accept(server_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET)
		{
			std::cout << "accept 실패" << std::endl;
			continue;
		}

		std::thread(ClientThread, client_sock).detach();
	}

	closesocket(server_sock);
	WSACleanup();

	return 0;
}