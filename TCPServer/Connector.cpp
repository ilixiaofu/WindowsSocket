#include "Connector.h"
#include <assert.h>


Connector::Connector(SOCKET hSocket, struct sockaddr_in sockaddr) :hSocket(hSocket), _sockaddr_in(sockaddr)
{
}

Connector::~Connector()
{
}

SOCKET Connector::getSocket()
{
	return this->hSocket;
}

void Connector::setSocket(SOCKET hSocket)
{
	this->hSocket = hSocket;
}

struct sockaddr_in& Connector::getSockaddr()
{
	return this->_sockaddr_in;
}

void Connector::setSockaddr(struct sockaddr_in& sockaddr)
{
	this->_sockaddr_in = sockaddr;
}

bool Connector::setnBlock(bool nblock)
{
	assert(hSocket != INVALID_SOCKET);
	u_long mode = nblock ? 1 : 0;
	int iret = ioctlsocket(this->hSocket, FIONBIO, &mode);
	assert(iret == 0);
	return true;
}

//  setsockopt(), WSAAsyncSelect().
int Connector::recvMsg(unsigned char* byteBuf, int nsize)
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

bool Connector::sendMsg(const unsigned char* byteBuf, int nsize)
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
					this->Close();
					return false;
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

/*
�ú������ڹر�TCP���ӣ��������ر�socket�����
howֵ��SD_RECEIVE��SD_SEND��SD_BOTH��
SD_RECEIVE�����رս���ͨ�����ڸ�socket�ϲ����ٽ������ݣ�
�����ǰ���ջ���������δȡ�����ݻ����Ժ��������ݵ��
��TCP�����Ͷ˷���RST�������������á�

SD_SEND�����رշ���ͨ����TCP�Ὣ���ͻ����е����ݶ�������ϲ����յ��������ݵ�ACK����Զ˷���FIN����
��������û�и������ݷ��͡������һ�����Źرչ��̡�

SD_BOTH���ʾͬʱ�رս���ͨ���ͷ���ͨ����
*/
void Connector::Shutdown(int how)
{
	assert(hSocket != INVALID_SOCKET);
	int iret = shutdown(hSocket, how);
	assert(iret == 0);
}


void Connector::Close()
{
	assert(hSocket != INVALID_SOCKET);
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

bool Connector::invalid()
{
	if (this->hSocket == INVALID_SOCKET) {
		return true;
	}
	else {
		return false;
	}
}