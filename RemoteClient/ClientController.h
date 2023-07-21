#pragma once
#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include <map>
#include "BlackTool.h"

#define WM_SEND_PACK (WM_USER+1)//���Ͱ�����
#define WM_SEND_DATA (WM_USER+2)//��������
#define WM_SHOW_STATUS (WM_USER+3)//չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4)//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000)//�Զ�����Ϣ����

class CClientController
{
public:
	//��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitController();
	//����
	int Invoke(CWnd* pMainWnd);
	//������Ϣ
	LRESULT SendMessage(MSG msg);
	//��������������ĵ�ַ
	void UpdataAddress(int nIP, int nPort) {
		CClientController::getInstance()->UpdataAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientController::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientController::getInstance()->CloseSocket();
	}
	bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(pack);
		//1 �鿴���̷���
		//2 �鿴ָ��Ŀ¼�µ��ļ�
		//3 ���ļ�
		//4 �����ļ�
		//5 ������
		//6 ������Ļ����
		//7 ����
		//8 ����
		//9 ɾ���ļ�
		//1981 ��������
		//����ֵ��������ţ����С��0���Ǵ���
	}
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0) 
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false)return false;
		pClient->Send(CPacket(nCmd, pData, nLength));
		int cmd = DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (bAutoClose)
			CloseSocket();
		return cmd;
	}
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CBlackTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownloadFile(CString strPath) {
		CFileDialog dlg(FALSE, NULL,
			strPath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
			NULL, &m_remoteDlg);
		if (dlg.DoModal() == IDOK) {
			m_strRemote = strPath;
			m_strLocal = dlg.GetPathName();
			m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
			if (WaitForSingleObject(m_hThreadDownload, 0) == WAIT_TIMEOUT) {
				return -1;
			}
			m_statusDlg.BeginWaitCursor();
			m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));
			m_statusDlg.ShowWindow(SW_SHOW);
			m_statusDlg.CenterWindow(&m_remoteDlg);
			m_statusDlg.SetActiveWindow();
		}
		return 0;
	}

	void StartWatchScreen();
protected:
	void threadWatchScreen();
	static void threadWatchScreen(void* arg);
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	CClientController() :
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg)
	{
		m_isClosed = true;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != NULL) {
			delete m_instance;
			m_instance = NULL;
		}
	}
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam,LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam,LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam,LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam,LPARAM lParam);
private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	} MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam,
		LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	bool m_isClosed;//�����Ƿ�ر�
	//�����ļ���Զ��·��
	CString m_strRemote;
	//�����ļ��ı��ر���·��
	CString m_strLocal;
	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

