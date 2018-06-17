#ifndef ABOUT_DLG_H_
#define ABOUT_DLG_H_

#include <atlctrls.h>

#define WM_LOADCPUINFO WM_USER + 706

class CAboutDlg : public CDialogImpl<CAboutDlg> {
public:
  enum { IDD = IDD_ABOUTBOX };

  BEGIN_MSG_MAP(CAboutDlg)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_LOADCPUINFO, OnLoadcpuinfo)
  COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
  COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/,
                       BOOL & /*bHandled*/);
  LRESULT OnLoadcpuinfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/,
                        BOOL & /*bHandled*/);

  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/,
                     BOOL & /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

private:
  CEdit c_cpu_;
};

#endif // ABOUT_DLG_H_
