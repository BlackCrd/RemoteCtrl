#pragma once

template<class T>
class CBlackQueue
{//线程安全的队列（利用IOCP实现）
public:
	CBlackQueue();
	~CBlackQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hTread;
public:
	typedef struct IocpParam {
		int nOperator;//操作
		T strData;//数据
		_beginthread_proc_type cbFunc;//回调
		HANDLE hEvent;//pop操作需要的
		IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
			cbFunc = cb;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM;//Post Parameter 用于投递信息的结构体
	enum {
		BQPush,
		BQPop,
		BQSize,
		BQClear
	};
};