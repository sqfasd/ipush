// This file define an abstract file class, provide some basic file operations
// and smart factory methods.

#ifndef BASE_FILE_BASE_H_
#define BASE_FILE_BASE_H_

#include <string>
#include <vector>

#include "base/basictypes.h"

namespace base {

class Status {
 public:
  // Create a success status.
  Status() : code_(kOk) {}
  ~Status() {}

  // Return a success status.
  static Status OK() { return Status(); }

  // Return error status of an appropriate type.
  static Status IOError(const std::string& msg) {
    return Status(kIOError, msg);
  }
  static Status NotFound(const std::string& msg) {
    return Status(kNotFound, msg);
  }
  static Status Corruption(const std::string& msg) {
    return Status(kCorruption, msg);
  }
  static Status Unsupported(const std::string& msg) {
    return Status(kUnsupported, msg);
  }

  // Returns true iff the status indicates success.
  bool ok() const { return code_ == kOk; }
  // Returns true iff the status indicates a NotFound error.
  bool IsNotFound() const { return code_ == kNotFound; }
  // Returns true iff the status indicates an IO error.
  bool IsIOError() const { return code_ == kIOError; }
  // Returns true if the status indicates an Corruption error
  bool IsCorruption() const { return code_ == kCorruption; }
  // Returns true iff the status indicates an unsupported functionality
  bool IsUnsupported() const { return code_ == kUnsupported; }

  // Return a string representation of this status suitable for printing.
  // Returns the string "OK" for success.
  const std::string ToString() const;

  bool operator==(const Status &another) const {
    return code_ == another.code_;
  }

 private:
  // Add more ...
  enum Code {
    kOk = 0,
    kIOError = 1,
    kNotFound = 2,
    kCorruption = 3,
    kUnsupported = 4,
  };

  Status(Code code, const std::string& msg) : code_(code), msg_(msg) {}

  int code_;
  std::string msg_;
};

// Abstract object of file class.
class FileBase {
 public:
  typedef int64 size_type;

  enum Mode {
    kAppend = 0,
    kWrite = 1,
    kRead = 2,
  };

  enum SeekPos {
    kSeekSet = 0,
    kSeekCur = 1,
    kSeekEnd = 2,
  };

  virtual ~FileBase() {}

  // Open a file by given the path and openning mode.
  static Status Open(const std::string &path, Mode mode, FileBase **file_obj);
  static FileBase *Open(const std::string &path, Mode mode);
  static FileBase *OpenOrDie(const std::string &path, Mode mode);

  // Write data to the file at current position of the file cursor.
  virtual Status Write(const void *buffer, size_type length) = 0;

  // Read data from the file at current position of the file cursor.
  // Will set eof if current position great or equal than file length.
  virtual Status Read(size_type length, std::string *result) = 0;

  // Read data from the file at a specific offset. Depend on underlying
  // filesystem, the position may changed.
  virtual Status ReadAt(size_type length,
                        size_type offset,
                        std::string *result) = 0;

  // Put cursor to the position. This will set eof to false, as posix.
  // Will always return false if this file is not seekable.
  virtual Status Seek(size_type pos, SeekPos seek_pos) = 0;

  // Flush buffer.
  virtual Status Flush() = 0;

  // Return current position of the file cursor.
  virtual size_type Position() const = 0;

  // If it reaches the end of the file.
  virtual bool Eof() const = 0;

  // Check if the file exists.
  static bool Exists(const std::string &path);

  // Check if the path is a dir.
  static bool IsDir(const std::string &path);

  // Move the file from old path to new path
  static Status MoveFile(const std::string &old_path,
                         const std::string &new_path);

  // Create a directory.
  static Status CreateDir(const std::string& path);

  // Get all dir names under the directory, the sub directories are ignored.
  static Status GetDirsInDir(const std::string &dir,
                             std::vector<std::string> *dirs);

  // Get all file names under the directory, the sub directories are ignored.
  static Status GetFilesInDir(const std::string &dir,
                              std::vector<std::string> *files);

  static Status DeleteRecursively(const std::string& name);

 protected:
  // Open the file.
  virtual Status OpenInternal(const std::string &path, Mode mode) = 0;

  // Whether the file is seekable, that is, if Seek could be done
  virtual bool IsSeekable() const = 0;
};

}  // namespace base

#endif  // BASE_FILE_BASE_H_
