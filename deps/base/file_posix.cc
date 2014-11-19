
#include <errno.h>

#include "base/file_posix.h"
#include "base/scoped_ptr.h"
#include "base/logging.h"

using std::string;
using std::vector;

namespace base {

FilePosix::FilePosix(): FileBase(), file_(NULL), pos_(0) {}

FilePosix::~FilePosix() {
  if (file_ != NULL) {
    fclose(file_);
  }
}

// TODO(someone): set proper error code
Status FilePosix::OpenInternal(const string &path, Mode mode) {
  CHECK(file_ == NULL) << "the file is already opened.";
  string mode_str;
  switch (mode) {
    case kAppend:
      mode_str = "a+";
      break;
    case kWrite:
      mode_str = "w+";
      break;
    case kRead:
      mode_str = "r";
      break;
    default:
      DCHECK(false) << "invalid mode type: " << mode;
  }
  file_ = fopen(path.c_str(), mode_str.c_str());
  return file_ ? Status::OK() : Status::IOError(StringPrintf(
        "path %s with mode %s", path.c_str(), mode_str.c_str()));
}

bool FilePosix::IsSeekable() const {
  return true;
}

Status FilePosix::Seek(size_type pos, SeekPos seek_pos) {
  if (0 != fseek(file_, pos, seek_pos)) {
    const char* str = std::strerror(errno);
    std::string err = StringPrintf("file seek failed: %s(%d)", str, errno);
    LOG(ERROR) << err;
    return Status::Unsupported(err);
  }
  return Status::OK();
}

FileBase::size_type FilePosix::Position() const {
  return ftell(file_);
}

bool FilePosix::Eof() const {
  return feof(file_);
}

Status FilePosix::Write(const void *buffer, size_type length) {
  size_type size = fwrite(buffer, 1, length, file_);
  if (size == length)  {
    return Status::OK();
  }

  int err_no = ferror(file_);
  const char* str = std::strerror(err_no);
  std::string err = StringPrintf("write failed: %s(%d)", str, err_no);
  LOG(ERROR) << err;
  return Status::IOError(err);
}

Status FilePosix::Read(size_type length, std::string *result) {
  result->clear();
  scoped_array<char> buffer(new char[length + 1]);
  size_type size = fread(buffer.get(), 1, length, file_);
  result->assign(buffer.get(), size);
  if (size == length || feof(file_)) {
    return Status::OK();
  }
  int err_no = ferror(file_);
  const char* str = std::strerror(err_no);
  std::string err = StringPrintf("read failed: %s(%d)", str, err_no);
  LOG(ERROR) << err;
  if (err_no == ENXIO || err_no == ENOMEM) {
    return Status::Corruption(err);
  } else {
    return Status::IOError(err);
  }
}

Status FilePosix::ReadAt(size_type length,
                        size_type offset,
                        std::string *result) {
  result->clear();
  scoped_array<char> buffer(new char[length + 1]);
  ssize_t size = pread(fileno(file_), buffer.get(), length, offset);
  if (size == length || feof(file_)) {
    result->assign(buffer.get(), size);
    return Status::OK();
  }
  int err_no = ferror(file_);
  const char* str = std::strerror(err_no);
  std::string err = StringPrintf("read failed: %s(%d)", str, err_no);
  LOG(ERROR) << err;
  if (err_no == ENXIO || err_no == ENOMEM) {
    return Status::Corruption(err);
  } else {
    return Status::IOError(err);
  }
}

Status FilePosix::Flush() {
  return fflush(file_) == 0 ? Status::OK() : Status::IOError("flush fail");
}

bool FilePosix::Exists(const string& path) {
  return access(path.c_str(), F_OK) == 0;
}

bool FilePosix::IsDir(const std::string& path) {
  if (!Exists(path))
    return false;
  struct stat statbuf;
  lstat(path.c_str(), &statbuf);
  return S_ISDIR(statbuf.st_mode);
}

Status FilePosix::MoveFile(const std::string &old_path,
                           const std::string &new_path) {
  if (rename(old_path.c_str(), new_path.c_str())) {
    string msg = strerror(errno);
    CHECK(errno != EXDEV) << "Invalid cross-device link";
    return Status::IOError(msg);
  }
  return Status::OK();
}

Status FilePosix::CreateDir(const string& path) {
  return mkdir(path.c_str(), 0755) == 0 ? Status::OK() : Status::IOError(path);
}

Status FilePosix::GetFilesInDir(const string& dir, vector<string>* files) {
  DIR *dp = NULL;
  struct dirent *entry = NULL;
  vector<string> result;

  if ((dp = opendir(dir.c_str())) == NULL) {
    return Status::IOError("cannot open directory: " + dir);
  }

  while ((entry = readdir(dp)) != NULL) {
    const string path = JoinPath(dir, entry->d_name);
    if (IsDir(path))
      continue;
    result.push_back(path);
  }
  closedir(dp);
  files->swap(result);
  return Status::OK();
}

Status FilePosix::GetDirsInDir(const std::string& dir,
                               vector<std::string>* dirs) {
  DIR *dp = NULL;
  struct dirent *entry = NULL;
  vector<string> result;

  if ((dp = opendir(dir.c_str())) == NULL) {
    return Status::IOError("cannot open directory: " + dir);
  }

  while ((entry = readdir(dp)) != NULL) {
    if (0 == strcmp(".", entry->d_name) ||
        0 == strcmp("..", entry->d_name)) {
      continue;
    }
    const string path = JoinPath(dir, entry->d_name);
    if (!IsDir(path))
      continue;
    result.push_back(path);
  }
  closedir(dp);
  dirs->swap(result);
  return Status::OK();
}

// TODO(someone): set proper error code
Status FilePosix::DeleteRecursively(const string& name) {
  // We don't care too much about error checking here since this is only used
  // in tests to delete temporary directories that are under /tmp anyway.
  struct stat stats;
  if (lstat(name.c_str(), &stats) != 0)
    return Status::IOError(name);

  Status result;
  if (S_ISDIR(stats.st_mode)) {
    DIR* dir = opendir(name.c_str());
    if (dir != NULL) {
      while (true) {
        struct dirent* entry = readdir(dir);
        if (entry == NULL) break;
        string entry_name = entry->d_name;
        if (entry_name != "." && entry_name != "..") {
          Status s = DeleteRecursively(name + "/" + entry_name);
          if (!s.ok())
            result = s;
        }
      }
    }
    closedir(dir);
    rmdir(name.c_str());
  } else if (S_ISREG(stats.st_mode)) {
    remove(name.c_str());
  }
  return result;
}

string FilePosix::JoinPath(const string& dirname, const string& basename) {
  if (StartsWithASCII(basename, "/", false))
    return basename;
  string result = dirname;
  if (!EndsWith(dirname, "/", false))
    result += "/";
  return result + basename;
}

}
