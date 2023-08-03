// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
//开机启动的时候，程序的权限是跟随启动用户的
//如果两者权限不一致，则会导致程序启动失败
//开机启动对环境变量有影响，如果依赖dll（动态库），则可能启动失败
// 解决方案：
//【复制这些dll到System32下面或者SysWOW64下面】
//system32下面，多是64位程序，sysWOW64下面，多是32位程序
//【使用静态库，而非动态库】
void ChooseAutoInvoke() {
    TCHAR wcsSystem[MAX_PATH] = _T("");
    CString strPath = CString(_T("C:\\Windows\\system32\\RemoteCtrl.exe"));
    if (PathFileExists(strPath)) {
        return;
    }
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strInfo = _T("该程序只允许用于合法的用途！！！\n"); 
    strInfo += _T("继续运行该程序，将使得这台机器处于被监控状态！\n");
    strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
    strInfo += _T("按下“否”按钮，程序只运行一次，不会在系统内留下任何东西！\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        char sPath[MAX_PATH] = "";
        char sSys[MAX_PATH] = "";
        std::string strExe = "\\RemoteCtrl.exe ";
        GetCurrentDirectoryA(MAX_PATH, sPath);
        GetSystemDirectoryA(sSys, sizeof(sSys));
        std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe; //cmd / K 能够让命令执行完不关闭窗口
        ret = system(strCmd.c_str());
        TRACE("ret=%d\r\n", ret);
        HKEY hKey = NULL;
        ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自动开机开启失败！是否权限不足？\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            ::exit(0);
        }
        ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自动开机开启失败！是否权限不足？\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            ::exit(0);
        }
        RegCloseKey(hKey);
    }
    else if (ret == IDCANCEL) {
        ::exit(0);
    }
    return;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            CCommand cmd;
            ChooseAutoInvoke();
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
                ::exit(0);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法正常接入用户，结束程序"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                ::exit(0);
                break;
            }          
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
