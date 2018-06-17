#include "stdafx.h"

#include <atlframe.h>
#include "resource.h"
#include "MainDlg.h"
#include "AboutDlg.h"

#include <list>
#include <stdint.h>
#include <atldlgs.h>
#include <atlstr.h>
#include <algorithm>

#include "file_enumerator.h"
#include "png_decoder.h"


#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")

namespace {

  static const unsigned int kDisIntervalTimer = 0x701;
  static const unsigned int kDisIntervalTimeLong = 300;

  void AppendModeCharacter(wchar_t mode_char, std::wstring* mode) {
    size_t comma_pos = mode->find(L',');
    mode->insert(comma_pos == std::wstring::npos ? mode->length() : comma_pos,
      1, mode_char);
  }
  FILE* OpenFile(const std::wstring& filename, const char* mode) {
    // 'N' is unconditionally added below, so be sure there is not one already
    // present before a comma in |mode|.

    USES_CONVERSION;
    std::wstring w_mode = A2W(mode);
    AppendModeCharacter(L'N', &w_mode);
    return _wfsopen(filename.c_str(), w_mode.c_str(), _SH_DENYNO);
  }

  bool GetFileSize(const std::wstring& file_path, int64_t* file_size) {

    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesEx(file_path.c_str(),
      GetFileExInfoStandard, &attr)) {
      return false;
    }

    ULARGE_INTEGER size;
    size.HighPart = attr.nFileSizeHigh;
    size.LowPart = attr.nFileSizeLow;
    *file_size = size.QuadPart;

    return true;
  }

  bool CloseFile(FILE* file) {
    if (file == nullptr)
      return true;
    return fclose(file) == 0;
  }

  bool ReadFileToStringWithMaxSize(const std::wstring& path,
    std::string* contents,
    size_t max_size) {
    if (contents)
      contents->clear();
    FILE* file = OpenFile(path, "rb");
    if (!file) {
      return false;
    }

    // Many files supplied in |path| have incorrect size (proc files etc).
    // Hence, the file is read sequentially as opposed to a one-shot read, using
    // file size as a hint for chunk size if available.
    constexpr int64_t kDefaultChunkSize = 1 << 16;
    int64_t chunk_size;
#if !defined(OS_NACL_NONSFI)
    if (!GetFileSize(path, &chunk_size) || chunk_size <= 0)
      chunk_size = kDefaultChunkSize - 1;
    // We need to attempt to read at EOF for feof flag to be set so here we
    // use |chunk_size| + 1.
    chunk_size = std::min<uint64_t>(chunk_size, max_size) + 1;
#else
    chunk_size = kDefaultChunkSize;
#endif  // !defined(OS_NACL_NONSFI)
    size_t bytes_read_this_pass;
    size_t bytes_read_so_far = 0;
    bool read_status = true;
    std::string local_contents;
    local_contents.resize(chunk_size);

    while ((bytes_read_this_pass = fread(&local_contents[bytes_read_so_far], 1,
      chunk_size, file)) > 0) {
      if ((max_size - bytes_read_so_far) < bytes_read_this_pass) {
        // Read more than max_size bytes, bail out.
        bytes_read_so_far = max_size;
        read_status = false;
        break;
      }
      // In case EOF was not reached, iterate again but revert to the default
      // chunk size.
      if (bytes_read_so_far == 0)
        chunk_size = kDefaultChunkSize;

      bytes_read_so_far += bytes_read_this_pass;
      // Last fread syscall (after EOF) can be avoided via feof, which is just a
      // flag check.
      if (feof(file))
        break;
      local_contents.resize(bytes_read_so_far + chunk_size);
    }
    read_status = read_status && !ferror(file);
    CloseFile(file);
    if (contents) {
      contents->swap(local_contents);
      contents->resize(bytes_read_so_far);
    }

    return read_status;
  }
  bool ReadFileToString(const std::wstring& file, std::string* out) {
    return ReadFileToStringWithMaxSize(file, out,
      std::numeric_limits<size_t>::max());
  }

  void WalkDirs(ATL::CString csPath, std::list<std::wstring>& out_set) {
    std::wstring input_path = csPath.GetString();
    base::FileEnumerator iter(input_path, true,
      base::FileEnumerator::FILES,
      FILE_PATH_LITERAL("*.png"), base::FileEnumerator::FolderSearchPolicy::ALL);
    for (std::wstring file = iter.Next(); !file.empty(); file = iter.Next())
      if (!file.empty())
        out_set.push_back(file);
  }

  //Aero效果是否已启用

  BOOL IsCompositionEnabled()
  {
    BOOL bEnabled, bResult;
    bResult = (SUCCEEDED(DwmIsCompositionEnabled(&bEnabled)) && bEnabled);
    return bResult;
  }

  //对已分层的窗口启动玻璃效果
  HRESULT EnableBlurBehindWindow(HWND hWnd, //窗口句柄
    BOOL bEnable = TRUE, //启用或禁用
    HRGN hRgn = 0, //模糊窗体中某个区域
    BOOL bTransitionOnMaximized = FALSE) //最大化时是否启用
  {
    DWM_BLURBEHIND blurBehind = { 0 };
    blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_TRANSITIONONMAXIMIZED;
    blurBehind.fEnable = bEnable;
    blurBehind.fTransitionOnMaximized = bTransitionOnMaximized;
    if (bEnable && hRgn != NULL)
    {
      blurBehind.dwFlags |= DWM_BB_BLURREGION;
      blurBehind.hRgnBlur = hRgn;
    }
    return DwmEnableBlurBehindWindow(hWnd, &blurBehind);
  }
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  // unregister message filtering and idle updates
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  ATLASSERT(pLoop != NULL);
  pLoop->RemoveMessageFilter(this);
  pLoop->RemoveIdleHandler(this);

  return 0;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  // center the dialog on the screen
  CenterWindow();

  // set icons
  HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
  SetIcon(hIcon, TRUE);
  HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
  SetIcon(hIconSmall, FALSE);


  c_file_.Attach(GetDlgItem(IDC_EDIT_FILE));

  c_file_.Attach(GetDlgItem(IDC_EDIT_FILE));
  c_pic_.Attach(GetDlgItem(IDC_STATIC_PIC));
  c_log_.Attach(GetDlgItem(IDC_EDIT_LOG));
  c_info_.Attach(GetDlgItem(IDC_EDIT_INFO));

  //if (IsCompositionEnabled() == TRUE)
  //{
  //  EnableBlurBehindWindow(m_hWnd, TRUE);
  //}

  // register object for message filtering and idle updates
  CMessageLoop* pLoop = _Module.GetMessageLoop();
  ATLASSERT(pLoop != NULL);
  pLoop->AddMessageFilter(this);
  pLoop->AddIdleHandler(this);

  UIAddChildWindowContainer(m_hWnd);

  return TRUE;
}

LRESULT CMainDlg::OnBnClickedButtonFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: Add your control notification handler code here
  KillTimer(kDisIntervalTimer);

  ATL::CString strFile = _T("");

  CFileDialog    dlgFile(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("Describe Files (*.png)|*.png|All Files (*.*)|*.*||"), NULL);

  if (dlgFile.DoModal() == IDOK) {
    strFile = dlgFile.m_szFileName;
  }

  if (!strFile.IsEmpty()) {
    c_file_.SetWindowTextW(strFile);
  }
  return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CAboutDlg dlg;
  dlg.DoModal();
  return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: Add validation code 
  CloseDialog(wID);
  return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CloseDialog(wID);
  return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
  DestroyWindow();
  ::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnBnClickedButtonDisp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: Add your control notification handler code here
  KillTimer(kDisIntervalTimer);
  log_.clear();
  c_log_.SetWindowTextW(L"");
  BuildFileList();
  CheckAndDispImageList();
  // TODO: Add your control notification handler code here
  if (file_list_.size() >= 1) {
    SetTimer(kDisIntervalTimer, kDisIntervalTimeLong, NULL);
  }
  return 0;
}


LRESULT CMainDlg::OnBnClickedButtonDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: Add your control notification handler code here

  KillTimer(kDisIntervalTimer);

  // TODO: Add your control notification handler code here
  TCHAR           szFolderPath[MAX_PATH] = { 0 };
  ATL::CString         strFolderPath = TEXT("");

  BROWSEINFO      sInfo;
  ::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
  sInfo.pidlRoot = 0;
  sInfo.lpszTitle = _T("请选择需要遍历PNG图片的文件夹");
  sInfo.ulFlags = BIF_NONEWFOLDERBUTTON | BIF_USENEWUI | BIF_EDITBOX;
  sInfo.lpfn = NULL;

  // 显示文件夹选择对话框  
  LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);
  if (lpidlBrowse != NULL)
  {
    // 取得文件夹名  
    if (::SHGetPathFromIDList(lpidlBrowse, szFolderPath))
    {
      strFolderPath = szFolderPath;
    }
  }
  if (lpidlBrowse != NULL)
  {
    ::CoTaskMemFree(lpidlBrowse);
  }

  if (!strFolderPath.IsEmpty()) {
    c_file_.SetWindowTextW(strFolderPath);
  }
  return 0;
}

void CMainDlg::BuildFileList()
{
  ATL::CString file_or_path;
  c_file_.GetWindowTextW(file_or_path);

  if (!PathFileExists(file_or_path))
    return;

  file_list_.clear();

  if (GetFileAttributes(file_or_path) & FILE_ATTRIBUTE_DIRECTORY) {
    if (file_or_path.Right(2) != "\\") {
      file_or_path += "\\";
    }
    WalkDirs(file_or_path, file_list_);
  }
  else {
    // single file
    file_list_.push_back(std::wstring(file_or_path));
  }

  all_ = file_list_.size();
  ok_ = 0;
  err_ = 0;
}

LRESULT CMainDlg::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  if (wParam == kDisIntervalTimer) {
    CheckAndDispImageList();

    if (file_list_.size() == 0) {
      KillTimer(kDisIntervalTimer);
    }
  }
  return 0;
}


void CMainDlg::SetLineColor(int line, COLORREF color) {
  CRichEditCtrl m_cRichEdit;
  int lineStart, lineEnd;
  CHARFORMAT cFmt;
  cFmt.cbSize = sizeof(CHARFORMAT);
  cFmt.crTextColor = color;
  cFmt.dwEffects = 0;
  cFmt.dwMask = CFM_COLOR;

  lineStart = c_log_.LineIndex(line);//取第一行的第一个字符的索引
  int nLineLength = c_log_.LineLength(line);
  lineEnd = lineStart + nLineLength;
  c_log_.SetSel(lineStart, lineEnd);//选取第一行字符
  c_log_.SetSelectionCharFormat(cFmt);//设置颜色
}

void CMainDlg::CheckAndDispImageList() {
  if (file_list_.size() >= 1) {
    std::wstring file = file_list_.front();
    file_list_.pop_front();
    ATL::CString cstr_file(file.c_str());

#if 1//USE_LIBPNG
    std::string data;
    if (ReadFileToString(cstr_file.GetString(), &data) && data.size() != 0) {
      std::vector<unsigned char> bits;
      int w, h;
      bool ok = PngDecoder::Decode((unsigned char*)data.data(), data.size(), PngDecoder::ColorFormat::FORMAT_SkBitmap, &bits, &w, &h);
      if (!ok) {
        err_++;
        DispLog(cstr_file.GetString(), false);
        c_pic_.SetBitmap(errbitmap_);
        MessageBox(cstr_file + L" 解码失败", L"Error", MB_OK);
        return;
      }

      HBITMAP bt = CreateBitmap(w, h, 32, 1, bits.data());

      c_pic_.SetBitmap(bt);
    }

    else {
      err_++;
      DispLog(cstr_file.GetString(), false);
      MessageBox(L"读取png文件失败！", L"Error", MB_OK);
      return;
    }
#else
    image_.Destroy();
    image_.Load(cstr_file);
    bitmap_ = image_.Detach();
    c_pic_.SetBitmap(bitmap_);
#endif
    ok_++;
    DispLog(cstr_file.GetString(), true);
  }
}

void CMainDlg::DispLog(std::wstring file, bool ok) {

  LogInfo info;
  info.ok = ok;
  info.logline = L"image:" + file + L" OK";
  log_.push_back(info);

  std::wstring log_str;
  for (auto& item : log_) {
    log_str += item.logline + L"\n";
  }

  c_log_.SetWindowTextW(log_str.c_str());

  for (size_t i = 0; i < log_.size(); i++) {
    if (!log_[i].ok)
      SetLineColor(i, RGB(255, 0, 0));
  }

  c_log_.LineScroll(c_log_.GetLineCount());

  std::wstring info_sum = L"All:" + std::to_wstring(all_) + L" Ok:" + std::to_wstring(ok_) + L" Err:" + std::to_wstring(err_);
  c_info_.SetWindowTextW(info_sum.c_str());
}

LRESULT CMainDlg::OnBnClickedButtonClear(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: Add your control notification handler code here

  KillTimer(kDisIntervalTimer);
  log_.clear();
  c_log_.SetWindowTextW(L"");
  c_pic_.SetBitmap(NULL);
  c_file_.SetWindowTextW(L"");
  file_list_.clear();
  all_ = 0;
  ok_ = 0;
  err_ = 0;
  c_info_.SetWindowTextW(L"");
  return 0;
}


LRESULT CMainDlg::OnBnClickedCpuCheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: Add your control notification handler code here
  CAboutDlg dlg;
  dlg.DoModal();
  return 0;
}
