#pragma once
#include "BSocket.h"
#include "BlackCThread.h"
/*
*1�����Ĺ��ܵ�����ʲô��
*2��ҵ���߼���ʲô��
*/
class BNetwork
{

};

typedef int (*AcceptFunc)(void* arg, BSOCKET& client);
typedef int (*RecvFunc)(void* arg, const BBuffer& buffer);
typedef int (*SendFunc)(void* arg, BSOCKET& client, int ret);
typedef int (*RecvFromFunc)(void* arg, const BBuffer& buffer, BSockaddrIn& addr);
typedef int (*SendToFunc)(void* arg, const BSockaddrIn& addr, int ret);

class BServerParameter
{
public:
	BServerParameter(
		const std::string& ip = "0.0.0.0",
		short port = 9527, BTYPE type = BTYPE::BTypeTCP,
		AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL,
		SendFunc sendf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	);
	//����
	BServerParameter& operator<<(AcceptFunc func);
	BServerParameter& operator<<(RecvFunc func);
	BServerParameter& operator<<(SendFunc func);
	BServerParameter& operator<<(RecvFromFunc func);
	BServerParameter& operator<<(SendToFunc func);
	BServerParameter& operator<<(const std::string& ip);
	BServerParameter& operator<<(short port);
	BServerParameter& operator<<(BTYPE type);
	//���
	BServerParameter& operator>>(AcceptFunc& func);
	BServerParameter& operator>>(RecvFunc& func);
	BServerParameter& operator>>(SendFunc& func);
	BServerParameter& operator>>(RecvFromFunc& func);
	BServerParameter& operator>>(SendToFunc& func);
	BServerParameter& operator>>(std::string& ip);
	BServerParameter& operator>>(short& port);
	BServerParameter& operator>>(BTYPE& type);
	//���ƹ��캯�������ں����� ����ͬ���͵ĸ�ֵ
	BServerParameter(const BServerParameter& param);
	BServerParameter& operator=(const BServerParameter& param);
	std::string m_ip;
	short m_port;
	BTYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;
};

class BServer :public ThreadFuncBase {
public:
	BServer(const BServerParameter& param);//��ʱ���ùؼ�����������Ҫ���ݸ��˿��������ʵ������ȥ����
	~BServer();
	int Invoke(void* arg);
	int Send(BSOCKET& client, const BBuffer& buffer);
	int Sendto(BSockaddrIn& addr, const BBuffer& buffer);
	int Stop();
private:
	int threadFunc();
	int threadUDPFunc();
	int threadTCPFunc();
private:
	BServerParameter m_params;
	void* m_args;
	BlackCThread m_thread;
	BSOCKET m_sock;
	std::atomic<bool> m_stop;
};