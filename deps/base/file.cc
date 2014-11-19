// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
// emulates google3/file/base/file.cc
// TODO(unknown): add unittests

#include "base/file.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <unistd.h>
#include <errno.h>

#include "base/logging.h"
#include "base/string_util.h"
#include "base/time.h"
#include "base/yr.h"
#include "base/file_base.h"

namespace {
const char kCurrentDirectory[] = ".";
const char kParentDirectory[] = "..";
const char kExtensionSeparator = '.';
const char kSeparators[] = "/";
const char* kCommonDoubleExtensions[] = { "gz", "z", "bz2" };
}

namespace base {

bool File::Exists(const string& name) {
  return FileBase::Exists(name);
}

bool File::IsDir(const std::string &path) {
  return FileBase::IsDir(path);
}

bool File::ReadFileToString(const string& name, string* output) {
  size_t file_size;
  if (FileSize(name, &file_size)) {
    output->reserve(file_size);
  }
  FileBase* file = NULL;
  Status st = FileBase::Open(name, FileBase::kRead, &file);
  if (file == NULL || !st.ok()) {
    LOG(ERROR) << "FileBase::Open(\"" << name << "\", \"FileBase::kRead\"): "
               << st.ToString();
  } else {
    while (st.ok() && !file->Eof()) {
      string buf;
      st = file->Read(64 * 1024, &buf);
      output->append(buf);
    }
  }
  delete file;
  return st.ok();
}

void File::ReadFileToStringOrDie(const string& name, string* output) {
  CHECK(ReadFileToString(name, output)) << "Could not read: " << name;
}

bool File::WriteStringToFile(const string& contents, const string& name) {
  FileBase* file = NULL;
  Status st = FileBase::Open(name, FileBase::kWrite, &file);
  if (file == NULL || !st.ok()) {
    LOG(ERROR) << "FileBase::Open(\"" << name << "\", \"FileBase::kWrite\"): "
               << st.ToString();
  } else {
    st = file->Write(contents.data(), contents.size());
    if (!st.ok()) {
      LOG(ERROR) << "FileBase::Write(\"" << name << "\"): " << st.ToString();
    }
  }
  delete file;
  return st.ok();
}

bool File::AppendStringToFile(const string& contents, const string& name) {
  FileBase* file = NULL;
  Status st = FileBase::Open(name, FileBase::kAppend, &file);
  if (file == NULL || !st.ok()) {
    LOG(ERROR) << "FileBase::Open(\"" << name << "\", \"FileBase::kAppend\"): "
               << st.ToString();
  } else {
    st = file->Write(contents.data(), contents.size());
    if (!st.ok()) {
      LOG(ERROR) << "FileBase::Write(\"" << name << "\"): " << st.ToString();
    }
  }
  delete file;
  return st.ok();
}

void File::AppendStringToFileOrDie(const string& contents, const string& name) {
  CHECK(AppendStringToFile(contents, name));
}

void File::WriteStringToFileOrDie(const string& contents, const string& name) {
  CHECK(WriteStringToFile(contents, name));
}

bool File::CreateDir(const string& name, int mode) {
  return mkdir(name.c_str(), mode) == 0;
}

bool File::RecursivelyCreateDir(const string& path, int mode) {
  if (CreateDir(path, mode)) return true;

  if (Exists(path)) return false;

  // Try creating the parent.
  string::size_type slashpos = path.find_last_of('/');
  if (slashpos == string::npos) {
    // No parent given.
    return false;
  }

  return RecursivelyCreateDir(path.substr(0, slashpos), mode) &&
         CreateDir(path, mode);
}

void File::DeleteRecursively(const string& name) {
  FileBase::DeleteRecursively(name);
}

string File::JoinPath(const string &dirname, const string &basename) {
  if (StartsWithASCII(basename, "/", false))
    return basename;
  string result = dirname;
  if (!EndsWith(dirname, "/", false))
    result += "/";
  return result + basename;
}

bool File::GetDirsInDir(const std::string &dir,
                         std::vector<std::string> *dirs) {
  return FileBase::GetDirsInDir(dir, dirs).ok();
}

void File::GetFilesInDirRecursively(const std::string &dir,
                                    std::vector<std::string> *paths) {
  CHECK(File::Exists(dir));
  vector<string> files;
  File::GetFilesInDir(dir, &files);
  for (int i = 0; i < files.size(); ++i) {
    paths->push_back(files[i]);
  }
  File::GetDirsInDir(dir, &files);
  for (int i = 0; i < files.size(); ++i) {
    GetFilesInDirRecursively(files[i], paths);
  }
}

bool File::GetFilesInDir(const string &dir, vector<string> *files) {
  return FileBase::GetFilesInDir(dir, files).ok();
}

void File::GetFilesInDirOrDie(const string &dir, vector<string> *files) {
  CHECK(GetFilesInDir(dir, files));
}

const string File::MakeTempFile(const std::string &basename) {
  string name;
  int result = 0;
  int try_time = 0;
  do {
    Time::Exploded now;
    Time::Now().UTCExplode(&now);

    name = StringPrintf("%s_%04d%02d%02d%02d%02d%02d%03d%05d", basename.c_str(),
                        now.year,
                        now.month,
                        now.day_of_month,
                        now.hour,
                        now.minute,
                        now.second,
                        now.millisecond,
                        getpid());
    struct stat st;
    result = stat(name.c_str(), &st);
    CHECK_LT(++try_time, 10);
  } while (0 == result);
  return name;
}

static void StripTrailingSeparatorsInternal(string* path) {
  // If there is no drive letter, start will be 1, which will prevent stripping
  // the leading separator if there is only one separator.  If there is a drive
  // letter, start will be set appropriately to prevent stripping the first
  // separator following the drive letter, if a separator immediately follows
  // the drive letter.
  //  string::size_type start = FindDriveLetter(path_) + 2;
  string::size_type start = 1;

  string::size_type last_stripped = string::npos;
  for (string::size_type pos = path->length();
       pos > start && ((*path)[pos - 1] == '/');
       --pos) {
    // If the string only has two separators and they're at the beginning,
    // don't strip them, unless the string began with more than two separators.
    if (pos != start + 1 || last_stripped == start + 2 ||
        ((*path)[start - 1]) != '/') {
      path->resize(pos - 1);
      last_stripped = pos;
    }
  }
}

const string File::BaseName(const std::string &path) {
  string new_path(path);
  StripTrailingSeparatorsInternal(&new_path);

  // Keep everything after the final separator, but if the pathname is only
  // one character and it's a separator, leave it alone.
  string::size_type last_separator =
      new_path.find_last_of(kSeparators, string::npos,
                                  arraysize(kSeparators) - 1);
  if (last_separator != string::npos &&
      last_separator < new_path.length() - 1) {
    new_path.erase(0, last_separator + 1);
  }

  return new_path;
}

const string File::DirName(const std::string &path) {
  string::size_type pos = path.rfind("/");
  if (pos == path.npos) return "";
  return path.substr(0, pos);
}

bool File::FileSize(const string& name, size_t* result) {
  if (!base::File::Exists(name) ||
      base::File::IsDir(name)) {
    return false;
  }
  struct stat st;
  if (stat(name.c_str(), &st)) {
    LOG(ERROR) << "cannot stat file: " << name;
    return false;
  } else {
    *result = st.st_size;
    return true;
  }
}

// Find the position of the '.' that separates the extension from the rest
// of the file name. The position is relative to BaseName(), not value().
// This allows a second extension component of up to 4 characters when the
// rightmost extension component is a common double extension (gz, bz2, Z).
// For example, foo.tar.gz or foo.tar.Z would have extension components of
// '.tar.gz' and '.tar.Z' respectively. Returns npos if it can't find an
// extension.
static string::size_type ExtensionSeparatorPosition(const string& path) {
  // Special case "." and ".."
  if (path == kCurrentDirectory || path == kParentDirectory)
    return string::npos;

  const string::size_type last_dot = path.rfind(kExtensionSeparator);

  // No extension, or the extension is the whole filename.
  if (last_dot == string::npos || last_dot == 0U)
    return last_dot;

  // Special case .<extension1>.<extension2>, but only if the final extension
  // is one of a few common double extensions.
  string extension(path, last_dot + 1);
  bool is_common_double_extension = false;
  for (size_t i = 0; i < arraysize(kCommonDoubleExtensions); ++i) {
    if (LowerCaseEqualsASCII(extension, kCommonDoubleExtensions[i]))
      is_common_double_extension = true;
  }
  if (!is_common_double_extension)
    return last_dot;

  // Check that <extension1> is 1-4 characters, otherwise fall back to
  // <extension2>.
  const string::size_type penultimate_dot =
      path.rfind(kExtensionSeparator, last_dot - 1);
  const string::size_type last_separator =
      path.find_last_of(kSeparators, last_dot - 1,
                        arraysize(kSeparators) - 1);
  if (penultimate_dot != string::npos &&
      (last_separator == string::npos ||
      penultimate_dot > last_separator) &&
      last_dot - penultimate_dot <= 5U &&
      last_dot - penultimate_dot > 1U) {
    return penultimate_dot;
  }

  return last_dot;
}

string File::GetExtension(const string& path) {
  string base = File::BaseName(path);
  const string::size_type dot = ExtensionSeparatorPosition(base);
  if (dot == string::npos)
    return string();

  return base.substr(dot);
}
}  // namespace base
