#include <iostream>
#include <string>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cout << "사용법: ChatOutput.exe [서버IP] [토큰] [닉네임]" << std::endl;
		return 1;
	}
	std::string server_ip = argv[1];
	std::string token = argv[2];
	std::string nickname = argv[3];

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup 실패");
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	{
		if (sock == INVALID_SOCKET)
		{
			WSACleanup();
			throw std::runtime_error("소켓 생성 실패");
		}
	}

	sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8888);

	if (inet_pton(AF_INET, server_ip.c_str(), &(serv_addr.sin_addr)) != 1)
	{
		closesocket(sock);
		WSACleanup();
		throw std::runtime_error("IP 변환 실패");
	}

	if (connect(sock, (SOCKADDR*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		throw std::runtime_error("서버 연결 실패");
	}

	std::string login_msg = token + " OUTPUT " + nickname + "\n";
	send(sock, login_msg.c_str(), static_cast<int>(login_msg.size()), 0);

	std::cout << "[출력 클라이언트 시작] \n";
	char buf[1024];
	while (true)
	{
		int n = recv(sock, buf, sizeof(buf) - 1, 0);
		if (n > 0)
		{
			buf[n] = 0;
			if (std::string(buf) == "__EXIT__")
			{
				std::cout << "[입력창 종료]" << std::endl;
				break;
			}
			std::cout << buf << std::endl;
		}
		else if (n == 0)
		{
			std::cout << "[서버와 연결이 끊어졌습니다.]" << std::endl;
			break;
		}
		else
		{
			std::cout << "[수신 오류]" << std::endl;
			break;
		}
	}
	closesocket(sock);
	WSACleanup();
	std::cout << "출력 클라이언트 종료" << std::endl;
	return 0;
}