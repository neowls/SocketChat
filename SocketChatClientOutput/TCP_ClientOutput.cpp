#include <iostream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cout << "����: ChatOutput.exe [����IP] [��ū] [�г���]" << std::endl;
		return 1;
	}
	std::string server_ip = argv[1];
	std::string token = argv[2];
	std::string nickname = argv[3];

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup ����");
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	{
		if (sock == INVALID_SOCKET)
		{
			WSACleanup();
			throw std::runtime_error("���� ���� ����");
		}
	}

	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8888);

	if (inet_pton(AF_INET, server_ip.c_str(), &(serv_addr.sin_addr)) != 1)
	{
		closesocket(sock);
		WSACleanup();
		throw std::runtime_error("IP ��ȯ ����");
	}

	if (connect(sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		throw std::runtime_error("���� ���� ����");
	}

	std::string login_msg = token + " OUTPUT " + nickname + "\n";
	send(sock, login_msg.c_str(), static_cast<int>(login_msg.size()), 0);

	std::cout << "[��� Ŭ���̾�Ʈ ����] \n";
	char buf[1024];
	while (true)
	{
		int n = recv(sock, buf, sizeof(buf) - 1, 0);
		if (n > 0)
		{
			buf[n] = 0;
			if (std::string(buf) == "__EXIT__")
			{
				std::cout << "[�Է�â ����]" << std::endl;
				break;
			}
			std::cout << buf << std::endl;
		}
		else if (n == 0)
		{
			std::cout << "[������ ������ ���������ϴ�.]" << std::endl;
			break;
		}
		else
		{
			std::cout << "[���� ����]" << std::endl;
			break;
		}
	}
	closesocket(sock);
	WSACleanup();
	std::cout << "��� Ŭ���̾�Ʈ ����" << std::endl;
	return 0;
}