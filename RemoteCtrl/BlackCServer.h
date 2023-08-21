#pragma once
#include <MSWSock.h>
#include "BlackCThread.h"
#include "CBlackQueue.h"
#include <map>

enum BlackCOperator{
    ENone,
    EAccept,
    ERecv,
    ESend,
    EError
};

class BlackCServer;
class BlackCClient;
typedef std::shared_ptr<BlackCClient> PCLIENT;

class BlackCOverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;//操作 参见BlackCOperator
    std::vector<char> m_buffer;//缓冲区
    ThreadWorker m_worker;//处理函数
    BlackCServer* m_server;//服务器对象
    BlackCClient* m_client;//对应的客户端
    WSABUF m_wsabuffer;
    virtual ~BlackCOverlapped() {
        m_client = NULL;
    }
};

template<BlackCOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<BlackCOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<BlackCOperator>class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class BlackCClient:public ThreadFuncBase {
public:
    BlackCClient();
    ~BlackCClient() {
        m_buffer.clear();
        closesocket(m_sock);
        m_recv.reset();
        m_send.reset();
        m_overlapped.reset();
        m_vecSend.Clear();
    }

    void SetOverlapped(BlackCClient* ptr);

    operator SOCKET() {
        return m_sock;
    }
    operator PVOID() {
        return &m_buffer[0];
    }
    operator LPOVERLAPPED();

    operator LPDWORD() {
        return &m_received;
    }
    LPWSABUF RecvWSABuffer();
    LPOVERLAPPED RecvOverlapped();
    LPWSABUF SendWSABuffer();
    LPOVERLAPPED SendOverlapped();
    DWORD& flags() { return m_flags; }
    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr; }
    size_t GetBufferSize()const { return m_buffer.size(); }
    int Recv();
    int Send(void* buffer, size_t nSize);
    int SendData(std::vector<char>& data);
private:
    SOCKET m_sock;
    DWORD m_received;
    DWORD m_flags;
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED> m_recv;
    std::shared_ptr<SENDOVERLAPPED> m_send;
    std::vector<char> m_buffer;
    size_t m_used;//已经使用的缓冲区大小
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isbusy;
    BlackCSendQueue<std::vector<char>> m_vecSend;//发送数据队列
};

template<BlackCOperator>
class AcceptOverlapped :public BlackCOverlapped,ThreadFuncBase
{
public:
    AcceptOverlapped();
    int AcceptWorker();
};


template<BlackCOperator>
class RecvOverlapped :public BlackCOverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();
    int RecvWorker() {
        int ret = m_client->Recv();
        return ret;
    }
};

template<BlackCOperator>
class SendOverlapped :public BlackCOverlapped, ThreadFuncBase
{
public:
    SendOverlapped();
    int SendWorker() {
        //TODO:
        /*
        * 1 Send可能不会立即完成
        */
        return -1;
    }
};
typedef SendOverlapped<ESend> SENDOVERLAPPED;

template<BlackCOperator>
class ErrorOverlapped :public BlackCOverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024 * 256);
    }
    int ErrorWorker() {
        //TODO:
        return -1;
    }
};
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class BlackCServer :
    public ThreadFuncBase
{
public:
    BlackCServer(const std::string& ip = "0.0.0.0", short port = 9527) : m_pool(10) {
        m_hIOCP = INVALID_HANDLE_VALUE;
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());       
    }
    ~BlackCServer();
    bool StartService();
    bool NewAccept() {
        //PCLIENT pClient(new BlackCClient());
        BlackCClient* pClient = new BlackCClient();
        pClient->SetOverlapped(pClient);
        m_client.insert(std::pair<SOCKET, BlackCClient*>(*pClient, pClient));
        if (!AcceptEx(m_sock,
            *pClient,
            *pClient,
            0,
            sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            *pClient, *pClient)) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        return true;
    }
    void BindNewSocket(SOCKET s, ULONG_PTR nKey);
private:
    void CreateSocket() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            return;
        }
        m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    }
    int threadIocp();
private:
    BlackCThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, BlackCClient*> m_client;
};
