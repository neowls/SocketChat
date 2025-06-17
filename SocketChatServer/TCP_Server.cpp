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

	// �г��� �ߺ� üũ
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

	std::cout << "[" << nickname << "] ���� �����ϼ̽��ϴ�." << std::endl;

	std::string joinMsg = nickname + "���� �����ϼ̽��ϴ�.\n";
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

	// �޼��� ���� ����
	while (true)
	{
		char buf[1024] = { 0 };
		int n = recv(client_sock, buf, sizeof(buf), 0);
		if (n <= 0)
		{
			std::string exitMessage = nickname + "���� �������ϴ�.\n";
			std::cout << "[" << nickname << "] ���� �����ϼ̽��ϴ�." << std::endl;
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

		// �� ����
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
	// 1. Winsock �ʱ�ȭ
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup ����" << std::endl;
		return 1;
	}

	// 2. ���� ����
	SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == INVALID_SOCKET)
	{
		std::cout << "���� ���� ����" << std::endl;
		WSACleanup();
		return 1;
	}

	// 3. ����ü �غ� �� ���ε�
	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(8888);


	if (bind(server_sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		std::cout << "���ε� ����" << std::endl;
		closesocket(server_sock);
		WSACleanup();
		return 1;
	}

	// 4. ����
	if (listen(server_sock, 5) == SOCKET_ERROR)
	{
		std::cout << "���� ����" << std::endl;
		closesocket(server_sock);
		WSACleanup();
		return 1;
	}

	std::cout << "ä�� ���� ����. ��Ʈ 8888���� ��� ��..." << std::endl;

	// 5. ���� ���� �� ������ ó��
	while (true)
	{
		bool exitCall = false;
		SOCKET client_sock = accept(server_sock, NULL, NULL);
		if (client_sock == INVALID_SOCKET)
		{
			std::cout << "accept ����" << std::endl;
			continue;
		}

		std::thread(ClientThread, client_sock).detach();
	}

	closesocket(server_sock);
	WSACleanup();

	return 0;
}