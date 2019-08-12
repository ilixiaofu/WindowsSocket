#include "SocketClient.h"
#include <assert.h>

SocketClient::SocketClient()
{
	this->init();
}

SocketClient::~SocketClient()
{
	this->Close();
	// ж��DLL
	int iret = WSACleanup();
	if (iret == SOCKET_ERROR) {
	}
}

void SocketClient::init()
{
	this->hSocket = this->createSocket();
}

SOCKET SocketClient::createSocket()
{
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	int iret = WSAStartup(wVersion, &wsaData);
	assert(iret == 0);
	SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert(hSocket != INVALID_SOCKET);
	return hSocket;
}

/*
const char * inet_ntop(int family, const void *addrptr, char *strptr, size_t len);     //����ֵ��ʽת��Ϊ���ʮ���Ƶ�ip��ַ��ʽ
*/
bool SocketClient::connectServer(const char* ip, unsigned short port, unsigned long long timeout)
{
	assert(this->hSocket != INVALID_SOCKET);
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET; // ʹ��IPv4
	sockaddr.sin_port = htons(port); // �˿�
	int iret = inet_pton(AF_INET, ip, &(sockaddr.sin_addr)); // ����ֵ�����ɹ���Ϊ1�������벻����Ч�ı��ʽ��Ϊ0����������Ϊ-1  
	assert(iret == 1);
	unsigned long long connect_timeout = 0;
	do {
		int icret = connect(this->hSocket, (struct sockaddr*)(&sockaddr), sizeof(sockaddr));
		if (icret == 0) {
			this->_sockaddr_in = sockaddr;
			break;
		} else if (icret == SOCKET_ERROR) {
			int ierr = WSAGetLastError();
			switch (ierr)
			{
			case WSANOTINITIALISED: // �ɹ��ص���WSAStartup()
				break;
			case WSAENETDOWN: // WINDOWS�׽ӿ�ʵ�ּ�⵽������ϵͳʧЧ
				break;
			case WSAENOTSOCK: // �����ֲ���һ���׽ӿڡ�
				break;
			case WSAEINPROGRESS: // һ��������WINDOWS�׽ӿڵ������������С�
				break;
			case WSAEINTR: // ͨ��һ��WSACancelBlockingCall()��ȡ��һ���������ģ����á�
				break;
			case WSAEWOULDBLOCK: // ���׽ӿ�����Ϊ��������ʽ��SO_LINGER����Ϊ���㳬ʱ�����
				break;
			default:
				//this->hSocket = INVALID_SOCKET;
				break;
			}
			Sleep(1 * 1000);
			connect_timeout++;
			if (connect_timeout > timeout) {
				connect_timeout = 0;
				return false;
			}
		}
	} while (true);
	return true;
}

void SocketClient::Shutdown(int how)
{
	assert(hSocket != INVALID_SOCKET);
	int iret = shutdown(hSocket, how);
	assert(iret == 0);
}

void SocketClient::Close()
{
	//assert(hSocket != INVALID_SOCKET);
	int iret = closesocket(hSocket);
	if (iret == 0) {
		this->hSocket = INVALID_SOCKET;
		return;
	}
	if (iret == SOCKET_ERROR) {
		iret = WSAGetLastError();
		switch (iret)
		{
		case WSANOTINITIALISED: // �ɹ��ص���WSAStartup()
			break;
		case WSAENETDOWN: // WINDOWS�׽ӿ�ʵ�ּ�⵽������ϵͳʧЧ
			break;
		case WSAENOTSOCK: // �����ֲ���һ���׽ӿڡ�
			break;
		case WSAEINPROGRESS: // һ��������WINDOWS�׽ӿڵ������������С�
			break;
		case WSAEINTR: // ͨ��һ��WSACancelBlockingCall()��ȡ��һ���������ģ����á�
			break;
		case WSAEWOULDBLOCK: // ���׽ӿ�����Ϊ��������ʽ��SO_LINGER����Ϊ���㳬ʱ�����
			break;
		default:
			break;
		}
		this->hSocket = INVALID_SOCKET;
	}
}

bool SocketClient::invalid()
{
	if (this->hSocket == INVALID_SOCKET) {
		return true;
	}
	else {
		return false;
	}
}

bool SocketClient::setnBlock(bool nblock)
{
	assert(hSocket != INVALID_SOCKET);
	u_long mode = nblock ? 1 : 0;
	int iret = ioctlsocket(this->hSocket, FIONBIO, &mode);
	assert(iret == 0);
	return true;
}

SOCKET SocketClient::getSocket()
{
	return this->hSocket;
}

void SocketClient::setSocket(SOCKET hSocket)
{
	this->hSocket = hSocket;
}

struct sockaddr_in& SocketClient::getSockaddr()
{
	return this->_sockaddr_in;
}

int SocketClient::recvMsg(unsigned char* byteBuf, int nsize)
{
	assert(byteBuf);
	assert(nsize > 0);
	char* buffer = (char*)byteBuf;
	int nread = recv(this->hSocket, buffer, nsize, 0);
	if (nread == SOCKET_ERROR) {
		int ierr = WSAGetLastError();
		switch (ierr)
		{
		case WSAEWOULDBLOCK: // ���ܻ�������������A non-blocking socket operation could not be completed immediately.
			nread = 0;
			break;
		case WSAEBADF: // The file handle supplied is not valid.
		case WSAETIMEDOUT: // ���ӳ�ʱ
		case WSAENETDOWN: // �����ж�
		default:
			this->Close();
			break;
		}
	}
	return nread;
}

bool SocketClient::sendMsg(const unsigned char* byteBuf, int nsize)
{
	if (!(byteBuf && nsize > 0)) {
		return false;
	}
	else {
		const char* buffer = (const char*)byteBuf;
		int limit = 1024 * 4;
		int nsend = 0;
		while (nsend < nsize) {
			limit = (nsize - nsend) < limit ? (nsize - nsend) : limit;
			int nwrite = send(this->hSocket, (buffer + nsend), limit, 0);
			if (nwrite > 0) {
				nsend += nwrite;
			}
			else if (nwrite == SOCKET_ERROR) {
				int ierr = WSAGetLastError();
				switch (ierr)
				{
				case WSAEBADF: // The file handle supplied is not valid.
				default:
					this->Close();
					return false;
				}
			}
			else {
				return false;
			}
		}
		return true;
	}
}