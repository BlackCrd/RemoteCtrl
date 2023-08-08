// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>
#include "CBlackQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// 唯一的应用程序对象
//#pragma comment( linker, "/subsystem:windows /entry:WinMainCRTStartup" )
//#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
//#pragma comment( linker, "/subsystem:console /entry:mainCRTStartup" )
//#pragma comment( linker, "/subsystem:console /entry:WinMainCRTStartup" )
//#define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define INVOKE_PATH _T("C:\\Users\\BlackC\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

CWinApp theApp;

using namespace std;

//业务和通用
bool ChooseAutoInvoke(const CString& strPath) {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    if (PathFileExists(strPath)) {
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！！！\n"); 
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath);
        if (!CBlackTool::WriteStartupDir(strPath)) {
            MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}

#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2

enum {
    IocpListEmpty,
    IocpListPush,
    IocpListPop
};

typedef struct IocpParam {
    int nOperator;//操作
    std::string strData;//数据
    _beginthread_proc_type cbFunc;//回调
    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
        nOperator = op;
        strData = sData;
        cbFunc = cb;
    }
    IocpParam() {
        nOperator = -1;
    }
}IOCP_PARAM;

void threadmain(HANDLE hIOCP) {
    std::list<std::string>lstString;
    DWORD dwTransferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* pOverlapped = NULL;
    int count = 0, count0 = 0;
    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
        if ((dwTransferred == 0) || (CompletionKey == NULL)) {
            printf("Thread is prepare to exit!\r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOperator == IocpListPush) {
            lstString.push_back(pParam->strData);
            printf("push size: %d \r\n", lstString.size());
            count0++;
        }
        else if (pParam->nOperator == IocpListPop) {
            printf("%p size: %d \r\n", pParam->cbFunc, lstString.size());
            std::string str;
            if (lstString.size() > 0) {
                str = lstString.front();
                lstString.pop_front();
            }
            if (pParam->cbFunc) {
                pParam->cbFunc(&str);
            }
            count++;
        }
        else if (pParam->nOperator == IocpListEmpty) {
            lstString.clear();
        }
        delete pParam;
    }
    //lstString.clear();
    printf("Thread exit count %d count0 %d\r\n", count, count0);
}

void threadQueueEntry(HANDLE hIOCP) {
    threadmain(hIOCP);
    _endthread();//代码到此为止，会导致本地对象无法调用析构，从而使得内存发生泄漏
}

void func(void* arg) {
    std::string* pstr = (std::string*)arg;
    if (pstr != NULL) {
        printf("Pop from list:%s\r\n", pstr->c_str());
        //delete pstr;
    }
    else {
        printf("List is empty,no data!\r\n");
    }
}

int main()
{ 
    if (!CBlackTool::Init())return 1;
    printf("Press any key to exit...\r\n");
    CBlackQueue<std::string> lstStrings;
    ULONGLONG tick = GetTickCount64(), tick0 = GetTickCount64();
    while (_kbhit() == 0) {//完成端口 把请求与实现 分离
        if (GetTickCount64() - tick0 > 1300) {
            lstStrings.PushBack("Hello world!");
            tick0 = GetTickCount64();
        }
        if (GetTickCount64() - tick > 2000) {
            std::string str;
            lstStrings.PopFront(str);
            tick = GetTickCount64();
            printf("Pop from Queue:%s\r\n",str.c_str());
        }
        Sleep(1);
    }
    printf("exit done! size %d \r\n", lstStrings.Size());
    lstStrings.Clear();
    printf("exit done! size %d \r\n", lstStrings.Size());
    ::exit(0);
    /*
    if (CBlackTool::IsAdmin()) {
        if (!CBlackTool::Init())return 1;
        OutputDebugString(L"Current is run as administrator!\r\n");
        //MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
        if (ChooseAutoInvoke(INVOKE_PATH)) {
            CCommand cmd;
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                break;
            }
        }
    }
    else {
        OutputDebugString(L"Current is run as normal user!\r\n");
        if (CBlackTool::RunAsAdmin() == false) {
            CBlackTool::ShowError();
            return 1;
        }
        //MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
    }
    */
    return 0;
}
