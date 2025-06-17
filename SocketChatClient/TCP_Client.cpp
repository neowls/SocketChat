#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> running(true);

void ReceiveThread(SOCKET sock)
{
	char buf[1024];
	while (running)
	{
		int n = recv(sock, buf, sizeof(buf), 0);
		if (n > 0)
		{
			// �� ����
			if (n >= 0 && n < sizeof(buf)) buf[n] = 0;
			else buf[sizeof(buf) - 1] = 0;
			std::cout << buf << std::endl;
		}
		else if (n == 0)
		{
			std::cout << "[���� ���� ����]" << std::endl;
			running = false;
			break;
		}
		else
		{
			std::cout << "[���� ����]" << std::endl;
			running = false;
			break;
		}
	}
}

int main()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup ����" << std::endl;
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		std::cout << "���� ���� ����" << std::endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8888);

	std::string server_ip;
	std::cout << "���� IP�� �Է��ϼ��� : ";
	std::getline(std::cin, server_ip);

	if (inet_pton(AF_INET, server_ip.c_str(), &(serv_addr.sin_addr)) != 1)
	{
		std::cout << "IP ��ȯ ����" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	if (connect(sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		std::cout << "���� ���� ����" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::string nickname;
	std::cout << "�г����� �Է��ϼ��� : ";
	std::getline(std::cin, nickname);
	send(sock, nickname.c_str(), static_cast<int>(nickname.size()), 0);

	std::cout << "[ä���� �����մϴ�. �����Ϸ��� /exit �� �Է��ϼ���.]\n";

	std::thread t(ReceiveThread, sock);
	while (running)
	{
		std::cout << nickname << " : ";
		std::string msg;
		std::getline(std::cin, msg);

		if (msg == "/exit")
		{
			running = false;
			break;
		}

		if (msg.empty()) continue;

		send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
	}

	closesocket(sock);
	WSACleanup();

	if (t.joinable()) t.join();

	std::cout << "Ŭ���̾�Ʈ ����" << std::endl;

	return 0;
}