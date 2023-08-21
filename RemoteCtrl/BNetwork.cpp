#include "pch.h"
#include "BNetwork.h"

BServer::BServer(const BServerParameter& param) :m_stop(false), m_sock(NULL)
{
	m_params = param;
	m_thread.UpdateWorker(ThreadWorker(this, (FUNCTYPE)&BServer::threadFunc));
}

BServer::~BServer()
{
	Stop();
}

int BServer::Invoke(void* arg)
{
    m_sock.reset(new BSocket(m_params.m_type));
    if (*m_sock == INVALID_SOCKET) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return -1;
    }
    if (m_params.m_type == BTYPE::BTypeTCP) {
        if (m_sock->listen() == -1) {
            return -2;
        }
    }
    BSockaddrIn client;
    if (-1 == m_sock->bind(m_params.m_ip, m_params.m_port)) {
        printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
        return -3;
    }
    if (m_thread.Start() == false)return -4;
    m_args = arg;
	return 0;
}

int BServer::Send(BSOCKET& client, const BBuffer& buffer)
{
    int ret = m_sock->send(buffer);//TODO:待优化，发送虽然成功，但是不完整！！！
    if (m_params.m_send)m_params.m_send(m_args, client, ret);
    return ret;
}

int BServer::Sendto(BSockaddrIn& addr, const BBuffer& buffer)
{
    int ret = m_sock->sendto(buffer, addr);//TODO:待优化，发送虽然成功，但是不完整！！！
    if (m_params.m_sendto)m_params.m_sendto(m_args, addr, ret);
    return ret;
}

int BServer::Stop()
{
    if (m_stop == false) {
        m_sock->close();
        m_stop = true;
        m_thread.Stop();
    }
    return 0;
}

int BServer::threadFunc()
{
    if (m_params.m_type == BTYPE::BTypeTCP) {
        return threadTCPFunc();
    }
    else {
        return threadUDPFunc();
    }
}

int BServer::threadUDPFunc()
{

    BBuffer buf(1024 * 256);
    BSockaddrIn client;
    int ret = 0;
    while (!m_stop) {
        ret = m_sock->recvfrom(buf, client);
        if (ret > 0) {
            client.update();
            if (m_params.m_recvfrom != NULL) {//
                m_params.m_recvfrom(m_args, buf, client);
            }
        }
        else {//
            printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
            break;
        }
        /*
        if (lstclients.size() <= 0) {
                lstclients.push_back(client);
                printf("%s(%d):%s ip %s port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.GetIP().c_str(), client.GetPort());
                ret = sock->sendto(buf, client);
                printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
            }
            else {
                buf.Update((void*)&lstclients.front(), lstclients.front().size());
                ret = sock->sendto(buf, client);
                printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
            }
            //CBlackTool::Dump((BYTE*)buf.c_str(), ret);            
        */
    }
    if (m_stop == false)m_stop = true;
    m_sock->close();
    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    return 0;
}

int BServer::threadTCPFunc()
{
    return 0;
}

BServerParameter::BServerParameter(
    const std::string& ip, short port, BTYPE type, 
    AcceptFunc acceptf, RecvFunc recvf, SendFunc sendf, 
    RecvFromFunc recvfromf, SendToFunc sendtof)
{
    m_ip = ip;
    m_port = port;
    m_type = type;
    m_accept = acceptf;
    m_recv = recvf;
    m_send = sendf;
    m_recvfrom = recvfromf;
    m_sendto = sendtof;
}

BServerParameter& BServerParameter::operator<<(AcceptFunc func)
{
    m_accept = func;
    return *this;
}

BServerParameter& BServerParameter::operator<<(RecvFunc func)
{
    m_recv = func;
    return *this;
}

BServerParameter& BServerParameter::operator<<(SendFunc func)
{
    m_send = func;
    return *this;
}

BServerParameter& BServerParameter::operator<<(RecvFromFunc func)
{
    m_recvfrom = func;
    return *this;
}

BServerParameter& BServerParameter::operator<<(SendToFunc func)
{
    m_sendto = func;
    return *this;
}

BServerParameter& BServerParameter::operator<<(const std::string& ip)
{
    m_ip = ip;
    return *this;
}

BServerParameter& BServerParameter::operator<<(short port)
{
    m_port = port;
    return *this;
}

BServerParameter& BServerParameter::operator<<(BTYPE type)
{
    m_type = type;
    return *this;
}

BServerParameter& BServerParameter::operator>>(AcceptFunc& func)
{
    func = m_accept;
    return *this;
}

BServerParameter& BServerParameter::operator>>(RecvFunc& func)
{
    func = m_recv;
    return *this;
}

BServerParameter& BServerParameter::operator>>(SendFunc& func)
{
    func = m_send;
    return *this;
}

BServerParameter& BServerParameter::operator>>(RecvFromFunc& func)
{
    func = m_recvfrom;
    return *this;
}

BServerParameter& BServerParameter::operator>>(SendToFunc& func)
{
    func = m_sendto;
    return *this;
}

BServerParameter& BServerParameter::operator>>(std::string& ip)
{
    ip = m_ip;
    return *this;
}

BServerParameter& BServerParameter::operator>>(short& port)
{
    port = m_port;
    return *this;
}

BServerParameter& BServerParameter::operator>>(BTYPE& type)
{
    type = m_type;
    return *this;
}

BServerParameter::BServerParameter(const BServerParameter& param)
{
    m_ip = param.m_ip;
    m_port = param.m_port;
    m_type = param.m_type;
    m_accept = param.m_accept;
    m_recv = param.m_recv;
    m_send = param.m_send;
    m_recvfrom = param.m_recvfrom;
    m_sendto = param.m_sendto;
}

BServerParameter& BServerParameter::operator=(const BServerParameter& param)
{
    if (this != &param) {
        m_ip = param.m_ip;
        m_port = param.m_port;
        m_type = param.m_type;
        m_accept = param.m_accept;
        m_recv = param.m_recv;
        m_send = param.m_send;
        m_recvfrom = param.m_recvfrom;
        m_sendto = param.m_sendto;
    }
    return *this;
}
