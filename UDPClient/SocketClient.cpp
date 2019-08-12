#include "SocketClient.h"
#include <assert.h>

SocketClient::SocketClient(const char* ip, unsigned short port)
{
	this->init(ip, port);
}

SocketClient::~SocketClient()
{
	this->Close();
	// ж��DLL
	int iret = WSACleanup();
	if (iret == SOCKET_ERROR) {
	}
}

void SocketClient::init(const char* ip, unsigned short port)
{
	this->hSocket = this->createSocket();
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET; // ʹ��IPv4
	sockaddr.sin_port = htons(port); // �˿�
	int iret = inet_pton(AF_INET, ip, &(sockaddr.sin_addr)); // ����ֵ�����ɹ���Ϊ1�������벻����Ч�ı��ʽ��Ϊ0����������Ϊ-1  
	assert(iret == 1);
	this->_sockaddr_in = sockaddr;
}

SOCKET SocketClient::createSocket()
{
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	int iret = WSAStartup(wVersion, &wsaData);
	assert(iret == 0);
	SOCKET hSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert(hSocket != INVALID_SOCKET);
	return hSocket;
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

int SocketClient::recvMsg(sockaddr* from_sockaddr, int* fromlen, unsigned char* byteBuf, int nsize)
{
	assert(byteBuf);
	assert(nsize > 0);
	char* buffer = (char*)byteBuf;
	int nread = recvfrom(this->hSocket, buffer, nsize, 0, from_sockaddr, fromlen);
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
			int nwrite = sendto(this->hSocket, (buffer + nsend), limit, 0, (struct sockaddr*)&(this->_sockaddr_in), sizeof(this->_sockaddr_in));
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