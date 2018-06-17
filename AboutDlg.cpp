#include "stdafx.h"
#include "resource.h"
#include "AboutDlg.h"

#include <array>
#include <bitset>
#include <sstream>
#include <string>
#include <vector>

namespace {

class InstructionSet {
  // forward declarations
  class InstructionSet_Internal;

public:
  // getters
  static std::string Vendor(void) { return CPU_Rep.vendor_; }
  static std::string Brand(void) { return CPU_Rep.brand_; }

  static bool SSE3(void) { return CPU_Rep.f_1_ECX_[0]; }
  static bool PCLMULQDQ(void) { return CPU_Rep.f_1_ECX_[1]; }
  static bool MONITOR(void) { return CPU_Rep.f_1_ECX_[3]; }
  static bool SSSE3(void) { return CPU_Rep.f_1_ECX_[9]; }
  static bool FMA(void) { return CPU_Rep.f_1_ECX_[12]; }
  static bool CMPXCHG16B(void) { return CPU_Rep.f_1_ECX_[13]; }
  static bool SSE41(void) { return CPU_Rep.f_1_ECX_[19]; }
  static bool SSE42(void) { return CPU_Rep.f_1_ECX_[20]; }
  static bool MOVBE(void) { return CPU_Rep.f_1_ECX_[22]; }
  static bool POPCNT(void) { return CPU_Rep.f_1_ECX_[23]; }
  static bool AES(void) { return CPU_Rep.f_1_ECX_[25]; }
  static bool XSAVE(void) { return CPU_Rep.f_1_ECX_[26]; }
  static bool OSXSAVE(void) { return CPU_Rep.f_1_ECX_[27]; }
  static bool AVX(void) { return CPU_Rep.f_1_ECX_[28]; }
  static bool F16C(void) { return CPU_Rep.f_1_ECX_[29]; }
  static bool RDRAND(void) { return CPU_Rep.f_1_ECX_[30]; }

  static bool MSR(void) { return CPU_Rep.f_1_EDX_[5]; }
  static bool CX8(void) { return CPU_Rep.f_1_EDX_[8]; }
  static bool SEP(void) { return CPU_Rep.f_1_EDX_[11]; }
  static bool CMOV(void) { return CPU_Rep.f_1_EDX_[15]; }
  static bool CLFSH(void) { return CPU_Rep.f_1_EDX_[19]; }
  static bool MMX(void) { return CPU_Rep.f_1_EDX_[23]; }
  static bool FXSR(void) { return CPU_Rep.f_1_EDX_[24]; }
  static bool SSE(void) { return CPU_Rep.f_1_EDX_[25]; }
  static bool SSE2(void) { return CPU_Rep.f_1_EDX_[26]; }

  static bool FSGSBASE(void) { return CPU_Rep.f_7_EBX_[0]; }
  static bool BMI1(void) { return CPU_Rep.f_7_EBX_[3]; }
  static bool HLE(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[4]; }
  static bool AVX2(void) { return CPU_Rep.f_7_EBX_[5]; }
  static bool BMI2(void) { return CPU_Rep.f_7_EBX_[8]; }
  static bool ERMS(void) { return CPU_Rep.f_7_EBX_[9]; }
  static bool INVPCID(void) { return CPU_Rep.f_7_EBX_[10]; }
  static bool RTM(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[11]; }
  static bool AVX512F(void) { return CPU_Rep.f_7_EBX_[16]; }
  static bool RDSEED(void) { return CPU_Rep.f_7_EBX_[18]; }
  static bool ADX(void) { return CPU_Rep.f_7_EBX_[19]; }
  static bool AVX512PF(void) { return CPU_Rep.f_7_EBX_[26]; }
  static bool AVX512ER(void) { return CPU_Rep.f_7_EBX_[27]; }
  static bool AVX512CD(void) { return CPU_Rep.f_7_EBX_[28]; }
  static bool SHA(void) { return CPU_Rep.f_7_EBX_[29]; }

  static bool PREFETCHWT1(void) { return CPU_Rep.f_7_ECX_[0]; }

  static bool LAHF(void) { return CPU_Rep.f_81_ECX_[0]; }
  static bool LZCNT(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_ECX_[5]; }
  static bool ABM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[5]; }
  static bool SSE4a(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[6]; }
  static bool XOP(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[11]; }
  static bool TBM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[21]; }

  static bool SYSCALL(void) {
    return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[11];
  }
  static bool MMXEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[22]; }
  static bool RDTSCP(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[27]; }
  static bool _3DNOWEXT(void) {
    return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[30];
  }
  static bool _3DNOW(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[31]; }

private:
  static const InstructionSet_Internal CPU_Rep;

  class InstructionSet_Internal {
  public:
    InstructionSet_Internal()
        : nIds_{0}, nExIds_{0}, isIntel_{false}, isAMD_{false}, f_1_ECX_{0},
          f_1_EDX_{0}, f_7_EBX_{0}, f_7_ECX_{0}, f_81_ECX_{0}, f_81_EDX_{0},
          data_{}, extdata_{} {
      // int cpuInfo[4] = {-1};
      std::array<int, 4> cpui;

      // Calling __cpuid with 0x0 as the function_id argument
      // gets the number of the highest valid function ID.
      __cpuid(cpui.data(), 0);
      nIds_ = cpui[0];

      for (int i = 0; i <= nIds_; ++i) {
        __cpuidex(cpui.data(), i, 0);
        data_.push_back(cpui);
      }

      // Capture vendor string
      char vendor[0x20];
      memset(vendor, 0, sizeof(vendor));
      *reinterpret_cast<int *>(vendor) = data_[0][1];
      *reinterpret_cast<int *>(vendor + 4) = data_[0][3];
      *reinterpret_cast<int *>(vendor + 8) = data_[0][2];
      vendor_ = vendor;
      if (vendor_ == "GenuineIntel") {
        isIntel_ = true;
      } else if (vendor_ == "AuthenticAMD") {
        isAMD_ = true;
      }

      // load bitset with flags for function 0x00000001
      if (nIds_ >= 1) {
        f_1_ECX_ = data_[1][2];
        f_1_EDX_ = data_[1][3];
      }

      // load bitset with flags for function 0x00000007
      if (nIds_ >= 7) {
        f_7_EBX_ = data_[7][1];
        f_7_ECX_ = data_[7][2];
      }

      // Calling __cpuid with 0x80000000 as the function_id argument
      // gets the number of the highest valid extended ID.
      __cpuid(cpui.data(), 0x80000000);
      nExIds_ = cpui[0];

      char brand[0x40];
      memset(brand, 0, sizeof(brand));

      for (int i = 0x80000000; i <= nExIds_; ++i) {
        __cpuidex(cpui.data(), i, 0);
        extdata_.push_back(cpui);
      }

      // load bitset with flags for function 0x80000001
      if (nExIds_ >= 0x80000001) {
        f_81_ECX_ = extdata_[1][2];
        f_81_EDX_ = extdata_[1][3];
      }

      // Interpret CPU brand string if reported
      if (nExIds_ >= 0x80000004) {
        memcpy(brand, extdata_[2].data(), sizeof(cpui));
        memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
        memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
        brand_ = brand;
      }
    };

    int nIds_;
    int nExIds_;
    std::string vendor_;
    std::string brand_;
    bool isIntel_;
    bool isAMD_;
    std::bitset<32> f_1_ECX_;
    std::bitset<32> f_1_EDX_;
    std::bitset<32> f_7_EBX_;
    std::bitset<32> f_7_ECX_;
    std::bitset<32> f_81_ECX_;
    std::bitset<32> f_81_EDX_;
    std::vector<std::array<int, 4>> data_;
    std::vector<std::array<int, 4>> extdata_;
  };
};

// Initialize static member data
const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;

std::wstring GetCpuTest() {
  std::wostringstream stream;
  auto &outstream = stream;

  auto support_message = [&outstream](std::wstring isa_feature,
                                      bool is_supported) {
    outstream << isa_feature
              << (is_supported ? L" supported" : L" not supported") << L"\r\n";
  };

  USES_CONVERSION;

  outstream << A2W(InstructionSet::Vendor().c_str()) << std::endl;
  outstream << A2W(InstructionSet::Brand().c_str()) << std::endl;

  support_message(L"3DNOW", InstructionSet::_3DNOW());
  support_message(L"3DNOWEXT", InstructionSet::_3DNOWEXT());
  support_message(L"ABM", InstructionSet::ABM());
  support_message(L"ADX", InstructionSet::ADX());
  support_message(L"AES", InstructionSet::AES());
  support_message(L"AVX", InstructionSet::AVX());
  support_message(L"AVX2", InstructionSet::AVX2());
  support_message(L"AVX512CD", InstructionSet::AVX512CD());
  support_message(L"AVX512ER", InstructionSet::AVX512ER());
  support_message(L"AVX512F", InstructionSet::AVX512F());
  support_message(L"AVX512PF", InstructionSet::AVX512PF());
  support_message(L"BMI1", InstructionSet::BMI1());
  support_message(L"BMI2", InstructionSet::BMI2());
  support_message(L"CLFSH", InstructionSet::CLFSH());
  support_message(L"CMPXCHG16B", InstructionSet::CMPXCHG16B());
  support_message(L"CX8", InstructionSet::CX8());
  support_message(L"ERMS", InstructionSet::ERMS());
  support_message(L"F16C", InstructionSet::F16C());
  support_message(L"FMA", InstructionSet::FMA());
  support_message(L"FSGSBASE", InstructionSet::FSGSBASE());
  support_message(L"FXSR", InstructionSet::FXSR());
  support_message(L"HLE", InstructionSet::HLE());
  support_message(L"INVPCID", InstructionSet::INVPCID());
  support_message(L"LAHF", InstructionSet::LAHF());
  support_message(L"LZCNT", InstructionSet::LZCNT());
  support_message(L"MMX", InstructionSet::MMX());
  support_message(L"MMXEXT", InstructionSet::MMXEXT());
  support_message(L"MONITOR", InstructionSet::MONITOR());
  support_message(L"MOVBE", InstructionSet::MOVBE());
  support_message(L"MSR", InstructionSet::MSR());
  support_message(L"OSXSAVE", InstructionSet::OSXSAVE());
  support_message(L"PCLMULQDQ", InstructionSet::PCLMULQDQ());
  support_message(L"POPCNT", InstructionSet::POPCNT());
  support_message(L"PREFETCHWT1", InstructionSet::PREFETCHWT1());
  support_message(L"RDRAND", InstructionSet::RDRAND());
  support_message(L"RDSEED", InstructionSet::RDSEED());
  support_message(L"RDTSCP", InstructionSet::RDTSCP());
  support_message(L"RTM", InstructionSet::RTM());
  support_message(L"SEP", InstructionSet::SEP());
  support_message(L"SHA", InstructionSet::SHA());
  support_message(L"SSE", InstructionSet::SSE());
  support_message(L"SSE2", InstructionSet::SSE2());
  support_message(L"SSE3", InstructionSet::SSE3());
  support_message(L"SSE4.1", InstructionSet::SSE41());
  support_message(L"SSE4.2", InstructionSet::SSE42());
  support_message(L"SSE4a", InstructionSet::SSE4a());
  support_message(L"SSSE3", InstructionSet::SSSE3());
  support_message(L"SYSCALL", InstructionSet::SYSCALL());
  support_message(L"TBM", InstructionSet::TBM());
  support_message(L"XOP", InstructionSet::XOP());
  support_message(L"XSAVE", InstructionSet::XSAVE());

  return stream.str();
}
} // namespace

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/,
                                LPARAM /*lParam*/, BOOL & /*bHandled*/) {
  CenterWindow(GetParent());
  c_cpu_.Attach(GetDlgItem(IDC_EDIT_CPU));
  PostMessage(WM_LOADCPUINFO);
  return TRUE;
}

LRESULT CAboutDlg::OnLoadcpuinfo(UINT /*uMsg*/, WPARAM /*wParam*/,
                                 LPARAM /*lParam*/, BOOL & /*bHandled*/) {
  std::wstring cpuinfo = GetCpuTest();
  c_cpu_.SetWindowTextW(cpuinfo.c_str());
  return 0;
}
