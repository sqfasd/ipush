#include "base/daemonizer.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#include "base/flags.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/file.h"

DEFINE_bool(daemon, false, "run as daemon");

namespace base {

static int lockfile(int fd) {
  struct flock fl;

  fl.l_type = F_WRLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;

  return fcntl(fd, F_SETLK, &fl);
}

int already_running(std::string pid_filename, pid_t pid) {
  int     fd;
  const int LOCKMODE = (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  fd = open(pid_filename.c_str(), O_RDWR|O_CREAT, LOCKMODE);

  CHECK_GE(fd, 0) <<  " can't open "
                  << pid_filename.c_str() << ":" << strerror(errno)
                  << ". Please check the path is writable";

  if (lockfile(fd) < 0) {
    if (errno == EACCES || errno == EAGAIN) {
      close(fd);
      return 1;
    }

    LOG(ERROR) <<  "can't lock "
               << pid_filename.c_str() << ":" << strerror(errno);
    exit(1);
  }

  CHECK(!ftruncate(fd, 0));

  std::string spid = StringPrintf("%ld", static_cast<int64>(pid));
  CHECK_GT(write(fd, spid.c_str(), spid.length() + 1), 0);
  return 0;
}

void daemonize() {
  if (FLAGS_daemon == false) {
    return;
  }

  const char *cmd = base::ProgramInvocationShortName();

  int i, fd0, fd1, fd2;
  pid_t pid;
  struct rlimit rl;
  struct sigaction sa;

  /*
   * Clear file creation mask.
   */
  umask(0);

  /*
   * Get maximum number of file descriptors.
   */
  CHECK_GE(getrlimit(RLIMIT_NOFILE, &rl), 0) << cmd << "can't get file limit";

  /*
   * Become a session leader to lose controlling TTY.
   */
  CHECK_GE((pid = fork()), 0) << cmd << ": can't fork";

  if (pid != 0) {
    /* parent */
    exit(0);
  }
  setsid();

  /*
   * Ensure future opens won't allocate controlling TTYs.
   */
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  CHECK_GE(sigaction(SIGHUP, &sa, NULL), 0) << cmd << ": can't ignore SIGHUP";

  CHECK_GE((pid = fork()), 0) << cmd << ": can't fork";
  if (pid != 0) {
    /* parent */
    exit(0);
  }

  pid = getpid();

  /*
   * Close all open file descriptors.
   */
  if (rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 1024;
  for (i = 3; i < rl.rlim_max; i++)
    close(i);

  std::string pidfilename =
    StringPrintf("./%s.pid",
        cmd);

  if (already_running(pidfilename, pid)) {
    LOG(ERROR) << cmd << " is already running, please stop it first";
    exit(1);
  }

  /*
   * Initialize the log file.
   */
  std::string logfilepath = "./logs";
  if (!base::File::Exists(logfilepath.c_str())) {
    base::File::CreateDir(logfilepath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
  }
  std::string logfilename =
    StringPrintf("%s/%s.log", logfilepath.c_str(), cmd);
  std::string stderrfilename =
    StringPrintf("%s/%s.error.log", logfilepath.c_str(), cmd);
  LOG(INFO) << cmd << " is running as daemon (pid is " << pid
            << "), logs are appending to " << logfilename
            << " , stderr logs are appending to " << stderrfilename;


  close(0);
  close(1);
  close(2);

  /*
   * Attach file descriptors 0, 1, and 2 to /dev/null.
   */
  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  /*
   * redirect stderr to log file, so the third party library (e.g. zookeeper)
   * can write logs into log file
   */
  fd2 = open(stderrfilename.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0666);

  logging::InitLogging(
      logfilename.c_str(),
      logging::LOG_ONLY_TO_FILE,
      logging::LOCK_LOG_FILE,
      logging::APPEND_TO_OLD_LOG_FILE);
  CHECK(fd0 == 0 && fd1 == 1 && fd2 == 2)
    << "unexpected file descriptors" << fd0 << ", " <<  fd1 << ", " <<  fd2;
}

}  // namespace base
