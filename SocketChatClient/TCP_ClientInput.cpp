#include <iostream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <random>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> running(true);

std::string generate_token()
{
	std::stringstream ss;
	std::random_device rd;
	for (int i = 0; i < 8; ++i)
	{
		ss << std::hex << ((rd() % 16));
	}
	return ss.str();
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

	std::string nickname;
	std::cout << "�г����� �Է��ϼ��� : ";
	std::getline(std::cin, nickname);

	// ��ū ����
	std::string token = generate_token();
	

	if (inet_pton(AF_INET, server_ip.c_str(), &(serv_addr.sin_addr)) != 1)
	{
		std::cout << "IP ��ȯ ����" << std::endl;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	if (connect(sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		throw std::runtime_error("���� ���� ����");
		return 1;
	}

	std::string login_msg = token + " INPUT " + nickname + "\n";
	send(sock, login_msg.c_str(), static_cast<int>(login_msg.size()), 0);

	std::string cmd = "SocketChatClientOutput.exe " + server_ip + " " + token + " " + nickname;
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);

	if (CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else
	{
		closesocket(sock);
		WSACleanup();
		throw std::runtime_error("��¿� exe ���� ����");
	}

	std::cout << "[ä���� �����մϴ�. �����Ϸ��� /exit �� �Է��ϼ���.]\n";

	std::string msg;
	char buf[1024];
	while (true)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds);
		timeval tv = { 0, 0 };

		int rv = select(0, &read_fds, NULL, NULL, &tv);
		if (rv > 0 && FD_ISSET(sock, &read_fds))
		{
			int n = recv(sock, buf, sizeof(buf) - 1, 0);
			if (n > 0)
			{
				buf[n] = 0;
				if (std::string(buf) == "__EXIT__")
				{
					std::cout << "���� ����\n";
					break;
				}
				else
				{
					std::cout << "������ ���� ����\n";
					break;
				}
			}

		}

		if (!std::getline(std::cin, msg)) break;
		if (msg == "/exit")
		{
			send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
			break;
		}
		if (msg.empty()) continue;
		send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
	}

	closesocket(sock);
	WSACleanup();

	std::cout << "Ŭ���̾�Ʈ ����" << std::endl;

	return 0;
}