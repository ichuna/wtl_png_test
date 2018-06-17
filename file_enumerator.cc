// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "file_enumerator.h"

namespace base {
template <typename T, size_t N> char (&ArraySizeHelper(T (&array)[N]))[N];
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

const wchar_t kSeparators[] = FILE_PATH_LITERAL("\\/");

const size_t kSeparatorsLength = arraysize(kSeparators);

const wchar_t kCurrentDirectory[] = FILE_PATH_LITERAL(".");
const wchar_t kParentDirectory[] = FILE_PATH_LITERAL("..");

const wchar_t kExtensionSeparator = FILE_PATH_LITERAL('.');

bool IsSeparator(wchar_t character) {
  for (size_t i = 0; i < kSeparatorsLength - 1; ++i) {
    if (character == kSeparators[i]) {
      return true;
    }
  }

  return false;
}

std::wstring::size_type FindDriveLetter(std::wstring path) {
  // This is dependent on an ASCII-based character set, but that's a
  // reasonable assumption.  iswalpha can be too inclusive here.
  if (path.length() >= 2 && path[1] == L':' &&
      ((path[0] >= L'A' && path[0] <= L'Z') ||
       (path[0] >= L'a' && path[0] <= L'z'))) {
    return 1;
  }
  return std::wstring::npos;
}

std::wstring StripTrailingSeparatorsInternal(std::wstring path) {
  // If there is no drive letter, start will be 1, which will prevent stripping
  // the leading separator if there is only one separator.  If there is a drive
  // letter, start will be set appropriately to prevent stripping the first
  // separator following the drive letter, if a separator immediately follows
  // the drive letter.
  std::wstring new_path(path);
  std::wstring::size_type start = FindDriveLetter(new_path) + 2;

  std::wstring::size_type last_stripped = std::wstring::npos;
  for (std::wstring::size_type pos = new_path.length();
       pos > start && IsSeparator(new_path[pos - 1]); --pos) {
    // If the string only has two separators and they're at the beginning,
    // don't strip them, unless the string began with more than two separators.
    if (pos != start + 1 || last_stripped == start + 2 ||
        !IsSeparator(new_path[start - 1])) {
      new_path.resize(pos - 1);
      last_stripped = pos;
    }
  }

  return new_path;
}

std::wstring BaseName(const std::wstring &path) {
  std::wstring new_path(path);
  new_path = StripTrailingSeparatorsInternal(new_path);

  // The drive letter, if any, is always stripped.
  std::wstring::size_type letter = FindDriveLetter(new_path);
  if (letter != std::wstring::npos) {
    new_path.erase(0, letter + 1);
  }

  // Keep everything after the final separator, but if the pathname is only
  // one character and it's a separator, leave it alone.
  std::wstring::size_type last_separator = new_path.find_last_of(
      kSeparators, std::wstring::npos, kSeparatorsLength - 1);
  if (last_separator != std::wstring::npos &&
      last_separator < new_path.length() - 1) {
    new_path.erase(0, last_separator + 1);
  }

  return new_path;
}

FileEnumerator::FileInfo::~FileInfo() = default;

bool FileEnumerator::ShouldSkip(const std::wstring &path) {

  std::wstring basename = BaseName(path);
  return basename == FILE_PATH_LITERAL(".") ||
         (basename == FILE_PATH_LITERAL("..") &&
          !(INCLUDE_DOT_DOT & file_type_));
}

bool FileEnumerator::IsTypeMatched(bool is_dir) const {
  return (file_type_ &
          (is_dir ? FileEnumerator::DIRECTORIES : FileEnumerator::FILES)) != 0;
}

std::wstring FileEnumerator::Append(std::wstring input,
                                    std::wstring component) {
  std::wstring appended = component;
  std::wstring without_nuls;

  const wchar_t kStringTerminator = FILE_PATH_LITERAL('\0');
  std::wstring::size_type nul_pos = component.find(kStringTerminator);
  if (nul_pos != std::wstring::npos) {
    without_nuls = component.substr(0, nul_pos);
    appended = std::wstring(without_nuls);
  }

  if (input.compare(kCurrentDirectory) == 0 && !appended.empty()) {
    // Append normally doesn't do any normalization, but as a special case,
    // when appending to kCurrentDirectory, just return a new path for the
    // component argument.  Appending component to kCurrentDirectory would
    // serve no purpose other than needlessly lengthening the path, and
    // it's likely in practice to wind up with FilePath objects containing
    // only kCurrentDirectory when calling DirName on a single relative path
    // component.
    return std::wstring(appended);
  }

  std::wstring new_path(input);
  new_path = StripTrailingSeparatorsInternal(new_path);

  // Don't append a separator if the path is empty (indicating the current
  // directory) or if the path component is empty (indicating nothing to
  // append).
  if (!appended.empty() && !new_path.empty()) {
    // Don't append a separator if the path still ends with a trailing
    // separator after stripping (indicating the root directory).
    if (!IsSeparator(new_path.back())) {
      // Don't append a separator if the path is just a drive letter.
      if (FindDriveLetter(new_path) + 1 != new_path.length()) {
        new_path.append(1, kSeparators[0]);
      }
    }
  }

  new_path += appended;
  return new_path;
}

} // namespace base
