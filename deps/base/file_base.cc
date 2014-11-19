
#include "base/file_base.h"
#include "base/file_posix.h"
#include "base/yr.h"

namespace {

enum FSType {
  kPosix = 0,
  kHdfs = 1,
  kSocket = 2,
  kUnknown = 3,
};

FSType GetFileType(const string &path) {
  string::size_type pos = path.find("://");
  if (pos == string::npos)
    return kPosix;
  string scheme = path.substr(0, pos);
  if (scheme == "file")
    return kPosix;
  if (scheme == "hdfs")
    return kHdfs;
  if (scheme == "socket")
    return kSocket;
  return kUnknown;
}

}  // namespace

namespace base {

const string Status::ToString() const {
  if (ok()) return "OK";
  string message;
  switch (code_) {
  case kIOError:
    message = "IO error";
    break;
  case kNotFound:
    message = "Not Found";
    break;
  case kCorruption:
    message = "Corruption";
    break;
  case kUnsupported:
    message = "Not supported";
    break;
  default:
    message = StringPrintf("Unknown code(%d)", code_);
    break;
  }
  if (!msg_.empty())
    message += " : " + msg_;
  return message;
}

Status FileBase::Open(const std::string &path, Mode mode, FileBase **file_obj) {
  *file_obj = new FilePosix();
  if (*file_obj != NULL) {
    return (*file_obj)->OpenInternal(path, mode);
  }
  return Status::Unsupported(
      StringPrintf("not found implement for path: %s", path.c_str()));
}

FileBase *FileBase::Open(const string &path, FileBase::Mode mode) {
  FileBase *file_obj = NULL;
  if (Open(path, mode, &file_obj).ok())
    return file_obj;
  delete file_obj;
  return NULL;
}

FileBase *FileBase::OpenOrDie(const string &path, FileBase::Mode mode) {
  FileBase *file_obj = Open(path, mode);
  CHECK(file_obj);
  return file_obj;
}

bool FileBase::Exists(const string &path) {
  switch (GetFileType(path)) {
  case kPosix:
    return FilePosix::Exists(path);
  default:
    break;
  }
  return false;
}

bool FileBase::IsDir(const string &path) {
  switch (GetFileType(path)) {
  case kPosix:
    return FilePosix::IsDir(path);
  default:
    break;
  }
  return false;
}

Status FileBase::MoveFile(const std::string &old_path,
                          const std::string &new_path) {
  if (GetFileType(old_path) != GetFileType(new_path)) {
    return Status::Unsupported(
        "the type old path and new path should be the same");
  }
  switch (GetFileType(old_path)) {
  case kPosix:
    return FilePosix::MoveFile(old_path, new_path);
  default:
    break;
  }
  return Status::Unsupported("");
}

Status FileBase::CreateDir(const std::string& path) {
  switch (GetFileType(path)) {
  case kPosix:
    return FilePosix::CreateDir(path);
  default:
    break;
  }
  return Status::Unsupported("");
}

Status FileBase::GetDirsInDir(const std::string &dir,
                              std::vector<std::string> *dirs) {
  switch (GetFileType(dir)) {
  case kPosix:
    return FilePosix::GetDirsInDir(dir, dirs);
  default:
    break;
  }
  return Status::Unsupported("");
}

Status FileBase::GetFilesInDir(const std::string &dir,
                               std::vector<std::string> *files) {
  switch (GetFileType(dir)) {
  case kPosix:
    return FilePosix::GetFilesInDir(dir, files);
  default:
    break;
  }
  return Status::Unsupported("");
}

Status FileBase::DeleteRecursively(const std::string &dir) {
  switch (GetFileType(dir)) {
  case kPosix:
    return FilePosix::DeleteRecursively(dir);
  default:
    break;
  }
  return Status::Unsupported("");
}

}  // namespace base
