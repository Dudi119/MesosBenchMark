#include "TcpSocket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include "Assert.h"
#include "Logger.h"

using namespace std;

namespace core
{


	TCPSocket::TCPSocket(SocketRole role)
	:m_role(role), m_fd(Invalid_Socket), m_hostAddress(""), m_port(-1)
	{
		m_connected = false;	
	}

	TCPSocket::TCPSocket(SocketRole role, int fd)
		:m_role(role), m_fd(fd), m_hostAddress(""), m_port(-1)
	{
		if(m_fd != Invalid_Socket)
			m_connected = true;
		else
			m_connected = false;
	}

	TCPSocket::TCPSocket(TCPSocket&& obj)
	:m_fd(Invalid_Socket)
	{
		m_connected = false;
		swap(m_fd, obj.m_fd);
		m_role = obj.m_role;
		bool temp = obj.m_connected;
		m_connected = temp;
		m_hostAddress = obj.m_hostAddress;
		m_port = obj.m_port;
	}

	void TCPSocket::operator=(TCPSocket&& obj)
	{
		if(m_connected)
			Close();
		
		swap(m_fd, obj.m_fd);
		m_role = obj.m_role;
		bool temp = obj.m_connected;
		m_connected = temp;
		m_hostAddress = obj.m_hostAddress;
		m_port = obj.m_port;
	}

	TCPSocket::~TCPSocket()
	{
		if(m_connected)
			Close();
	}

	void TCPSocket::Connect(const std::string& host, uint16_t port)
	{
		ASSERT(m_role == SocketRole::Client);
		VERIFY(m_connected == false, "Socket is already bound");
		TRACE_INFO("Attempting to open a socket");
		//Establish a socket
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		LINUX_VERIFY(m_fd != -1);
		//Prase received address
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET;

		struct addrinfo* result = nullptr;
		LINUX_VERIFY(::getaddrinfo(host.c_str(), nullptr, &hints, &result) == 0);
		//Build host address
		struct sockaddr_in serverAddress;
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
		serverAddress.sin_port   = htons(port);
		serverAddress.sin_family = AF_INET;
		//Free the addresses linked list
		freeaddrinfo(result);
		TRACE_INFO("Attempting to connect to host - %s:%u", host.c_str(), port);
		LINUX_VERIFY(::connect(m_fd, (const struct sockaddr*)&serverAddress, sizeof(serverAddress)) != -1);
		m_hostAddress = host;
		m_port = (int)port;
	}
	
	void TCPSocket::Close()
	{
		TRACE_INFO("Attempting to close the socket");
		LINUX_VERIFY(::close(m_fd) != -1);
		m_connected = false;
		m_fd = Invalid_Socket;
	}

	int TCPSocket::Send(char* buf, size_t bufSize)
	{
		ASSERT(m_role == SocketRole::Client);
		VERIFY(m_connected == true, "Socket is not connected");
		ssize_t bytesCount;
		LINUX_VERIFY(bytesCount = ::send(m_fd, (const void*)buf, bufSize, MSG_NOSIGNAL) != -1);
		return (int)bytesCount;	
	}

	pair<char*, ssize_t> TCPSocket::Receive(size_t sizeToRead)
	{
		ASSERT(m_role == SocketRole::Client);
		VERIFY(m_connected == true, "Socket is not connected");
		ssize_t bytesCount;
		char* buf = (char*)malloc(sizeof(char) * sizeToRead);
		LINUX_VERIFY(bytesCount = ::recv(m_fd, (void*)buf, sizeToRead, 0) != -1); 
		return {buf, bytesCount};
	}

	void TCPSocket::Bind(const string& host, int port)
	{
		ASSERT(m_role == SocketRole::Server);
		VERIFY(m_connected == false, "Socket is already bound");
		TRACE_INFO("Attempting to open a socket");
		//Establish a socket
		m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		LINUX_VERIFY(m_fd != -1);
		//Prase received address
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET;

		struct addrinfo* result = nullptr;
		LINUX_VERIFY(::getaddrinfo(host.c_str(), nullptr, &hints, &result) == 0);
		//Build host address
		struct sockaddr_in serverAddress;
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
		serverAddress.sin_port   = htons((uint16_t)port);
		serverAddress.sin_family = AF_INET;
		//Free the addresses linked list
		freeaddrinfo(result);
		TRACE_INFO("Attempting to bind host address - %s:%d", host.c_str(), port);
		LINUX_VERIFY(::bind(m_fd, (const struct sockaddr*)&serverAddress, 
					sizeof(serverAddress)) != -1);
		m_hostAddress = host;
		if(port) //Port is known
			m_port = port;
		else
		{
			socklen_t addressSize = sizeof(serverAddress);
			memset(&serverAddress, 0, sizeof(serverAddress));
			LINUX_VERIFY(::getsockname(m_fd, (struct sockaddr*)&serverAddress, 
						&addressSize) != -1);
			m_port = (int)ntohs(serverAddress.sin_port);
		}
	}

	void TCPSocket::Listen()
	{
		ASSERT(m_role == SocketRole::Server);
		VERIFY(m_connected, "Socket is not bound to any host");
		TRACE_INFO("Attempting to listen on a socket %s:%d", m_hostAddress.c_str(), m_port);
		LINUX_VERIFY(::listen(m_fd, Max_Incoming_connections) != -1);
	}

	TCPSocket TCPSocket::Accept()
	{
		ASSERT(m_role == SocketRole::Server);
		VERIFY(m_connected, "Socket is not bound to any host");
		sockaddr_in serverAddress;
		memset(&serverAddress, 0, sizeof(serverAddress));
		int fd;
		socklen_t addressSize = sizeof(serverAddress);
		VERIFY(fd = ::accept(m_fd, (sockaddr*)&serverAddress, &addressSize)
				!= Invalid_Socket, "Invalid socket was accepted");
		return {SocketRole::Client, fd};

	}
}
