#ifndef MAIN_DLG_H_
#define MAIN_DLG_H_

#include <atlctrls.h>
#include <string>
#ifndef min
#define min
#endif
#ifndef max
#define max
#endif
#include <atlimage.h>
#undef min
#undef max

#include <vector>
#include <list>

class CMainDlg : public CDialogImpl<CMainDlg>, public CUpdateUI<CMainDlg>,
		public CMessageFilter, public CIdleHandler
{
public:
	enum { IDD = IDD_MAINDLG };

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		UIUpdateChildWindows();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainDlg)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER(IDC_BUTTON_FILE, BN_CLICKED, OnBnClickedButtonFile)
		COMMAND_HANDLER(IDC_BUTTON_DISP, BN_CLICKED, OnBnClickedButtonDisp)
		COMMAND_HANDLER(IDC_BUTTON_DIR, BN_CLICKED, OnBnClickedButtonDir)
		COMMAND_HANDLER(IDC_BUTTON_CLEAR, BN_CLICKED, OnBnClickedButtonClear)
		COMMAND_HANDLER(ID_CPU_CHECK, BN_CLICKED, OnBnClickedCpuCheck)
	END_MSG_MAP()

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void CloseDialog(int nVal);
	LRESULT OnBnClickedButtonFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

protected:
	void BuildFileList();
	void SetLineColor(int line, COLORREF color);
	void DispLog(std::wstring file, bool ok);

private:
	HICON m_hIcon;
	CEdit c_file_;
	CRichEditCtrl c_log_;
	CEdit c_info_;
	CStatic c_pic_;
	int all_ = 0;
	int ok_ = 0;
	int err_ = 0;

	void CheckAndDispImageList();

	std::list<std::wstring> file_list_;

	CImage image_;
	HBITMAP bitmap_;
	CImage errimage_;
	HBITMAP errbitmap_;

	struct LogInfo {
		std::wstring logline;
		bool ok = true;
	};

	std::vector<LogInfo> log_;

public:
	LRESULT OnBnClickedButtonDisp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedButtonDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedButtonClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedCpuCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // MAIN_DLG_H_