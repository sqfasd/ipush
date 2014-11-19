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
// emulates google3/file/base/file.h
//

#ifndef BASE_FILE_H__
#define BASE_FILE_H__

#include <string>
#include <vector>

#include "base/basictypes.h"

namespace base {

// Protocol buffer code only uses a couple static methods of File, and only
// in tests.
class File {
 public:
  // Check if the file exists.
  static bool Exists(const std::string &name);

  // Check if the path is a dir.
  static bool IsDir(const std::string &path);

  // Read an entire file to a string.  Return true if successful, false
  // otherwise.
  static bool ReadFileToString(const std::string& name, std::string* output);

  // Same as above, but crash on failure.
  static void ReadFileToStringOrDie(const std::string& name,
                                    std::string* output);

  // Create a file and write a string to it.
  static bool WriteStringToFile(const std::string& contents,
                                const std::string& name);

  // Same as above, but crash on failure.
  static void WriteStringToFileOrDie(const std::string& contents,
                                     const std::string& name);

  // Create a file and write a string to it.
  // If the file exists, append data to it, not over-write on it.
  static bool AppendStringToFile(const std::string& contents,
                                const std::string& name);

  // Same as above, but crash on failure.
  static void AppendStringToFileOrDie(const std::string& contents,
                                 const std::string& name);

  // Create a directory.
  static bool CreateDir(const std::string& name, int mode);

  // Create a directory and all parent directories if necessary.
  static bool RecursivelyCreateDir(const std::string& path, int mode);

  // If "name" is a file, we delete it.  If it is a directory, we
  // call DeleteRecursively() for each file or directory (other than
  // dot and double-dot) within it, and then delete the directory itself.
  // The "dummy" parameters have a meaning in the original version of this
  // method but they are not used anywhere in protocol buffers.
  static void DeleteRecursively(const std::string& name);

  // Join the path.
  static std::string JoinPath(const std::string &dirname,
                              const std::string &basename);

  // Get all file names under the directory, the sub directories are ignored.
  static bool GetFilesInDir(const std::string &dir,
                            std::vector<std::string> *files);

  // Get all file names under the directory
  static void GetFilesInDirRecursively(const std::string &dir,
                            std::vector<std::string> *files);

  // Get all dir names under the directory, the sub directories are ignored.
  static bool GetDirsInDir(const std::string &dir,
                           std::vector<std::string> *dirs);

  // Same as above, but crash on failure.
  static void GetFilesInDirOrDie(const std::string &dir,
                                 std::vector<std::string> *files);

  static const std::string MakeTempFile(const std::string &basename);

  static const std::string BaseName(const std::string &path);

  static const std::string DirName(const std::string &path);

  static bool FileSize(const std::string& name, size_t* result);

  static std::string GetExtension(const std::string &path);

 private:
  DISALLOW_COPY_AND_ASSIGN(File);
};

}  // namespace base

#endif  // BASE_FILE_H__
