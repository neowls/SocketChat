#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <cstring>


#pragma comment(lib, "ws2_32.lib")

std::map<std::string, SOCKET> clients;
std::mutex clients_mutex;

void ClientThread(SOCKET client_sock)
{
	char namebuf[32] = { 0 };
	int nameLen = recv(client_sock, namebuf, sizeof(namebuf) - 1, 0);
	std::string nickname = (nameLen > 0) ? std::string(namebuf, nameLen) : "Unknown";

	// 닉네임 중복 체크
	std::string base_nick = nickname;
	int idx = 1;
	char tmp[16];
	{
		std::lock_guard<std::mutex> lock(clients_mutex);

		while (clients.find(nickname) != clients.end()) 
		{
			sprintf_s(tmp, "%d", idx++);
			nickname = base_nick + tmp;
		}
		clients.insert({ nickname, client_sock });
	}

	std::cout << "[" << nickname << "] 님이 접속하셨습니다." << std::endl;

	std::string joinMsg = nickname + "님이 입장하셨습니다.\n";
	{
		std::lock_guard<std::mutex> lock(clients_mutex);
		for (const auto& c : clients)
		{
			if (c.first != nickname)
			{
				send(c.second, joinMsg.c_str(), static_cast<int>(joinMsg.size()), 0);
			}
		}
	}

	// 메세지 수신 루프
	while (true)
	{
		char buf[1024] = { 0 };
		int n = recv(client_sock, buf, sizeof(buf), 0);
		if (n <= 0)
		{
			std::string exitMessage = nickname + "님이 나갔습니다.\n";
			std::cout << "[" << nickname << "] 님이 퇴장하셨습니다." << std::endl;
			{
				std::lock_guard<std::mutex> lock(clients_mutex);
				for (auto& c : clients)
				{
					if (c.first != nickname)
					{
						send(c.second, exitMessage.c_str(), static_cast<int>(exitMessage.size()), 0);
					}
				}
				clients.erase(nickname);
			}
			break;
		}

		// 널 종료
		if (n >= 0 && n < sizeof(buf)) buf[n] = 0;
		else buf[sizeof(buf) - 1] = 0;

		std::string chat = nickname + " : " + std::string(buf, n);
		std::cout << chat << std::endl;

		std::lock_guard<std::mutex> lock(clients_mutex);
		for (auto& c : clients)
		{
			if (c.first == nickname) continue;
			send(c.second, chat.c_str(), static_cast<int>(chat.size()), 0);
		}
	}

	closesocket(client_sock);
	
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