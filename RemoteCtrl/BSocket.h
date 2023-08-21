#pragma once
#include <WinSock2.h>
#include <memory>

enum class BTYPE{
	BTypeTCP = 1,
	BTypeUDP
};

class BSockaddrIn {
public:
	BSockaddrIn() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}
	BSockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	BSockaddrIn(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = nPort;
	}
	BSockaddrIn(const std::string& strIP, short nPort) {
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	}
	BSockaddrIn(const BSockaddrIn& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_ip = addr.m_ip;
		m_port = addr.m_port;
	}
	BSockaddrIn& operator=(const BSockaddrIn& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_ip = addr.m_ip;
			m_port = addr.m_port;
		}
		return *this;
	}
	operator sockaddr* ()const {
		return (sockaddr*)&m_addr;
	}
	operator void* ()const {
		return (void*)&m_addr;
	}
	void update() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	std::string GetIP()const {
		return m_ip;
	}
	short GetPort()const { return m_port; }
	inline int size()const { return sizeof(sockaddr_in); }
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class BBuffer :public std::string 
{
public:
	BBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	BBuffer(size_t size = 0) :std::string() {
		if (size > 0) { 
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	BBuffer(void* buffer, size_t size) :std::string(){
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~BBuffer() {
		std::string::~basic_string();
	}
	operator char* () { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }
	void Update(void* buffer, size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
};

class BSocket
{
public:
	BSocket(BTYPE nType = BTYPE::BTypeTCP,int nProtocol=0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_protocol = nProtocol;
	}

	BSocket(const BSocket& sock) {
		m_socket = socket(PF_INET, (int)m_type, m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}

	~BSocket() {
		close();
	}

	BSocket& operator=(const BSocket& sock) {
		if (this != &sock) {
			m_socket = socket(PF_INET, (int)m_type, m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
			m_addr = sock.m_addr;
		}
		return *this;
	}

	operator SOCKET()const { return m_socket; }
	operator SOCKET() { return m_socket; }
	bool operator==(SOCKET sock) const {
		return m_socket == sock;
	}
	int listen(int backlog = 5) {
		if (m_type != BTYPE::BTypeTCP) return -1;
		return ::listen(m_socket, backlog);
	}
	int bind(const std::string& ip, short port) {
		m_addr = BSockaddrIn(ip, port);
		return ::bind(m_socket, m_addr, m_addr.size());
	}
	int accept(){}
	int connect(const std::string& ip, short port) {}
	int send(const BBuffer& buffer){
		return ::send(m_socket, buffer, buffer.size(), 0);
	}
	int recv(BBuffer& buffer){
		return ::recv(m_socket, buffer, buffer.size(), 0);
	}
	int sendto(const BBuffer& buffer, const BSockaddrIn& to) {
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.size());
	}
	int recvfrom(BBuffer& buffer, BSockaddrIn& from) {
		int len = from.size();
		int ret = ::recvfrom(m_socket, buffer, buffer.size(), 0, from, &len);
		if (ret > 0) {
			from.update();
		}
		return ret;
	}
	void close() {
		if (m_socket != INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
private:
	SOCKET m_socket;
	BTYPE m_type;
	int m_protocol;
	BSockaddrIn m_addr;
};

typedef std::shared_ptr<BSocket> BSOCKET;

