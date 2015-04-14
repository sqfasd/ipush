#include "deps/base/logging.h"
#include "deps/base/daemonizer.h"
#include "deps/base/file.h"
#include "deps/base/at_exit.h"
#include "deps/base/flags.h"
#include "src/session_server.h"

const char* const SERVER_PID_FILE = "xcomet_server.pid";

static void WritePidFile() {
  base::File::WriteStringToFileOrDie(std::to_string(::getpid()),
      SERVER_PID_FILE);
}

int main(int argc, char* argv[]) {
  base::AtExitManager at_exit;
  base::ParseCommandLineFlags(&argc, &argv, false);
  base::daemonize();
  if (FLAGS_flagfile.empty()) {
    LOG(WARNING) << "not using --flagfile option !";
  }
  LOG(INFO) << "command line options\n" << base::CommandlineFlagsIntoString();
  WritePidFile();
  {
    xcomet::SessionServer server;
    server.Start();
    LOG(INFO) << "main loop break";
  }
  base::File::DeleteRecursively(SERVER_PID_FILE);
}
