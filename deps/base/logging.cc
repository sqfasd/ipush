// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#define MAX_PATH PATH_MAX
typedef FILE* FileHandle;
typedef pthread_mutex_t* MutexHandle;

#include <ctime>
#include <iomanip>
#include <cstring>
#include <algorithm>

#include "base/debug_util.h"
#include "base/eintr_wrapper.h"
#include "base/mutex.h"
#include "base/safe_strerror_posix.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"

DEFINE_int32(v, -1, "LOG verbose level.");
DEFINE_bool(enable_addition_info_business_id, true,
    "whether enable add business id in log");

namespace logging {

bool g_enable_dcheck = false;

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "ERROR_REPORT", "FATAL" };

int min_log_level = 0;
LogLockingState lock_log_file = LOCK_LOG_FILE;

// The default set here for logging_destination will only be used if
// InitLogging is not called.  On Windows, use a file next to the exe;
// on POSIX platforms, where it may not even be possible to locate the
// executable on disk, use stderr.
LoggingDestination logging_destination = LOG_ONLY_TO_SYSTEM_DEBUG_LOG;

const int kMaxFilteredLogLevel = LOG_WARNING;
std::string* log_filter_prefix;

// For LOG_ERROR and above, always print to stderr.
const int kAlwaysPrintErrorLevel = LOG_ERROR;

// Which log file to use? This is initialized by InitLogging or
// will be lazily initialized to the default value when it is
// first needed.
typedef char PathChar;
typedef std::string PathString;
PathString* log_file_name = NULL;

// this file is lazily opened and the handle may be NULL
FileHandle log_file = NULL;

// what should be prepended to each message?
bool log_process_id = false;
bool log_thread_id  = true;
bool log_date       = true;
bool log_timestamp  = true;
bool log_tickcount  = false;

// An assert handler override specified by the client to be called instead of
// the debug message dialog and process termination.
LogAssertHandlerFunction log_assert_handler = NULL;
// An report handler override specified by the client to be called instead of
// the debug message dialog.
LogReportHandlerFunction log_report_handler = NULL;
// A log message handler that gets notified of every log message we process.
LogMessageHandlerFunction log_message_handler = NULL;

// The lock is used if log file locking is false. It helps us avoid problems
// with multiple threads writing to the log file at the same time.
static base::Mutex* log_lock = NULL;

// When we don't use a lock, we are using a global mutex. We need to do this
// because LockFileEx is not thread safe.
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Helper functions to wrap platform differences.

int32 CurrentProcessId() {
  return getpid();
}

int32 CurrentThreadId() {
  return syscall(__NR_gettid);
}

uint64 TickCount() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64 absolute_micro = static_cast<int64>(ts.tv_nsec) / 1000;

  return absolute_micro;
}

void CloseFile(FileHandle log) {
  fclose(log);
}

void DeleteFilePath(const PathString& log_name) {
  unlink(log_name.c_str());
}

// Called by logging functions to ensure that debug_file is initialized
// and can be used for writing. Returns false if the file could not be
// initialized. debug_file will be NULL in this case.
bool InitializeLogFileHandle() {
  if (log_file)
    return true;

  if (!log_file_name) {
    // Nobody has called InitLogging to specify a debug log file, so here we
    // initialize the log file name to a default.
    // On other platforms we just use the current directory.
    log_file_name = new std::string("debug.log");
  }

  if (logging_destination == LOG_ONLY_TO_FILE ||
      logging_destination == LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG) {
    log_file = fopen(log_file_name->c_str(), "a");
    if (log_file == NULL)
      return false;
  }

  return true;
}

void InitLogMutex() {
  // statically initialized
}

void InitLogging(const PathChar* new_log_file, LoggingDestination logging_dest,
                 LogLockingState lock_log, OldFileDeletionState delete_old) {
#if defined(NDEBUG)
  g_enable_dcheck = false;
#else
  g_enable_dcheck = true;
#endif
  // TODO(quj): fix it
  //      CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableDCHECK);

  if (log_file) {
    // calling InitLogging twice or after some log call has already opened the
    // default log file will re-initialize to the new options
    CloseFile(log_file);
    log_file = NULL;
  }

  lock_log_file = lock_log;
  logging_destination = logging_dest;

  // ignore file options if logging is disabled or only to system
  if (logging_destination == LOG_NONE ||
      logging_destination == LOG_ONLY_TO_SYSTEM_DEBUG_LOG)
    return;

  if (!log_file_name)
    log_file_name = new PathString();
  *log_file_name = new_log_file;
  if (delete_old == DELETE_OLD_LOG_FILE)
    DeleteFilePath(*log_file_name);

  if (lock_log_file == LOCK_LOG_FILE) {
    InitLogMutex();
  } else if (!log_lock) {
    log_lock = new base::Mutex();
  }

  InitializeLogFileHandle();
}

void SetMinLogLevel(int level) {
  min_log_level = level;
}

int GetMinLogLevel() {
  return min_log_level;
}

void SetLogFilterPrefix(const char* filter)  {
  if (log_filter_prefix) {
    delete log_filter_prefix;
    log_filter_prefix = NULL;
  }

  if (filter)
    log_filter_prefix = new std::string(filter);
}

void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_date, bool enable_timestamp,
                 bool enable_tickcount) {
  log_process_id = enable_process_id;
  log_thread_id = enable_thread_id;
  log_date = enable_date;
  log_timestamp = enable_timestamp;
  log_tickcount = enable_tickcount;
}

void SetLogAssertHandler(LogAssertHandlerFunction handler) {
  log_assert_handler = handler;
}

void SetLogReportHandler(LogReportHandlerFunction handler) {
  log_report_handler = handler;
}

void SetLogMessageHandler(LogMessageHandlerFunction handler) {
  log_message_handler = handler;
}


// Displays a message box to the user with the error message in it.
// Used for fatal messages, where we close the app simultaneously.
void DisplayDebugMessageInDialog(const std::string& str) {
  if (str.empty())
    return;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       int ctr)
    : severity_(severity) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, const CheckOpString& result)
    : severity_(LOG_FATAL) {
  Init(file, line);
  stream_ << "Check failed: " << (*result.str_);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       const CheckOpString& result)
    : severity_(severity) {
  Init(file, line);
  stream_ << "Check failed: " << (*result.str_);
}

LogMessage::LogMessage(const char* file, int line)
     : severity_(LOG_INFO) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity) {
  Init(file, line);
}

// writes the common header info to the stream
void LogMessage::Init(const char* file, int line) {
  // log only the filename
  const char* last_slash = strrchr(file, '\\');
  if (last_slash)
    file = last_slash + 1;

  // TODO(darin): It might be nice if the columns were fixed width.

  stream_ <<  '[';
  if (log_process_id)
    stream_ << CurrentProcessId() << ':';
  if (log_thread_id)
    stream_ << CurrentThreadId() << ':';
  if (log_date || log_timestamp) {
    time_t t = time(NULL);
    struct tm local_time = {0};
    localtime_r(&t, &local_time);
    struct tm* tm_time = &local_time;
    if (log_date)
      stream_ << std::setfill('0')
              << std::setw(2) << 1 + tm_time->tm_mon
              << std::setw(2) << tm_time->tm_mday;
    if (log_date && log_timestamp)
      stream_ << '/';
    if (log_timestamp)
      stream_ << std::setfill('0')
              << std::setw(2) << tm_time->tm_hour
              << std::setw(2) << tm_time->tm_min
              << std::setw(2) << tm_time->tm_sec
              << ':';
  }
  if (log_tickcount)
    stream_ << std::setfill('0') << std::setw(6) << TickCount() << ':';
  stream_ << log_severity_names[severity_] << ":" << file <<
             "(" << line << ")] ";

  if (FLAGS_enable_addition_info_business_id) {
    stream_ << *(LogAdditionInfo::GetInstance());
  }

  message_start_ = stream_.tellp();
}

LogMessage::~LogMessage() {
  // TODO(brettw) modify the macros so that nothing is executed when the log
  // level is too high.
  if (severity_ < min_log_level)
    return;

  if (severity_ == LOG_FATAL) {
    // Include a stack trace on a fatal.
    StackTrace trace;
    stream_ << std::endl;  // Newline to separate from log message.
    trace.OutputToStream(&stream_);
  }
  stream_ << std::endl;
  std::string str_newline(stream_.str());

  // Give any log message handler first dibs on the message.
  if (log_message_handler && log_message_handler(severity_, str_newline))
    return;

  if (log_filter_prefix && severity_ <= kMaxFilteredLogLevel &&
      str_newline.compare(message_start_, log_filter_prefix->size(),
                          log_filter_prefix->data()) != 0) {
    return;
  }

  if (logging_destination == LOG_ONLY_TO_SYSTEM_DEBUG_LOG ||
      logging_destination == LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG) {
    {
      // TODO(erikkay): this interferes with the layout tests since it grabs
      // stderr and stdout and diffs them against known data. Our info and warn
      // logs add noise to that.  Ideally, the layout tests would set the log
      // level to ignore anything below error.  When that happens, we should
      // take this fprintf out of the #else so that Windows users can benefit
      // from the output when running tests from the command-line.  In the
      // meantime, we leave this in for Mac and Linux, but until this is fixed
      // they won't be able to pass any layout tests that have info or warn
      // logs.  See http://b/1343647
      fprintf(stderr, "%s", str_newline.c_str());
      fflush(stderr);
    }
  } else if (severity_ >= kAlwaysPrintErrorLevel) {
    // When we're only outputting to a log file, above a certain log level, we
    // should still output to stderr so that we can better detect and diagnose
    // problems with unit tests, especially on the buildbots.
    fprintf(stderr, "%s", str_newline.c_str());
    fflush(stderr);
  }

  // write to log file
  if (logging_destination != LOG_NONE &&
      logging_destination != LOG_ONLY_TO_SYSTEM_DEBUG_LOG &&
      InitializeLogFileHandle()) {
    // We can have multiple threads and/or processes, so try to prevent them
    // from clobbering each other's writes.
    if (lock_log_file == LOCK_LOG_FILE) {
      // Ensure that the mutex is initialized in case the client app did not
      // call InitLogging. This is not thread safe. See below.
      InitLogMutex();

      pthread_mutex_lock(&log_mutex);
    } else {
      // use the lock
      if (!log_lock) {
        // The client app did not call InitLogging, and so the lock has not
        // been created. We do this on demand, but if two threads try to do
        // this at the same time, there will be a race condition to create
        // the lock. This is why InitLogging should be called from the main
        // thread at the beginning of execution.
        log_lock = new base::Mutex();
      }
      log_lock->Lock();
    }

    fprintf(log_file, "%s", str_newline.c_str());
    fflush(log_file);

    if (lock_log_file == LOCK_LOG_FILE) {
      pthread_mutex_unlock(&log_mutex);
    } else {
      log_lock->Unlock();
    }
  }

  if (severity_ == LOG_FATAL) {
    // display a message or break into the debugger on a fatal error
    if (DebugUtil::BeingDebugged()) {
      DebugUtil::BreakDebugger();
    } else {
      if (log_assert_handler) {
        // make a copy of the string for the handler out of paranoia
        log_assert_handler(std::string(stream_.str()));
      } else {
        // Don't use the string with the newline, get a fresh version to send to
        // the debug message process. We also don't display assertions to the
        // user in release mode. The enduser can't do anything with this
        // information, and displaying message boxes when the application is
        // hosed can cause additional problems.
#ifndef NDEBUG
        DisplayDebugMessageInDialog(stream_.str());
#endif
        // Crash the process to generate a dump.
        DebugUtil::BreakDebugger();
      }
    }
  } else if (severity_ == LOG_ERROR_REPORT) {
    // We are here only if the user runs with --enable-dcheck in release mode.
    if (log_report_handler) {
      log_report_handler(std::string(stream_.str()));
    } else {
      DisplayDebugMessageInDialog(stream_.str());
    }
  }
}

SystemErrorCode GetLastSystemErrorCode() {
  return errno;
}

ErrnoLogMessage::ErrnoLogMessage(const char* file,
                                 int line,
                                 LogSeverity severity,
                                 SystemErrorCode err)
    : err_(err),
      log_message_(file, line, severity) {
}

ErrnoLogMessage::~ErrnoLogMessage() {
  stream() << ": " << safe_strerror(err_);
}

void CloseLogFile() {
  if (!log_file)
    return;

  CloseFile(log_file);
  log_file = NULL;
}

void RawLog(int level, const char* message) {
  if (level >= min_log_level) {
    size_t bytes_written = 0;
    const size_t message_len = strlen(message);
    int rv;
    while (bytes_written < message_len) {
      rv = HANDLE_EINTR(
          write(STDERR_FILENO, message + bytes_written,
                message_len - bytes_written));
      if (rv < 0) {
        // Give up, nothing we can do now.
        break;
      }
      bytes_written += rv;
    }

    if (message_len > 0 && message[message_len - 1] != '\n') {
      do {
        rv = HANDLE_EINTR(write(STDERR_FILENO, "\n", 1));
        if (rv < 0) {
          // Give up, nothing we can do now.
          break;
        }
      } while (rv != 1);
    }
  }

  if (level == LOG_FATAL)
    DebugUtil::BreakDebugger();
}

LogAdditionInfo* LogAdditionInfo::GetInstance() {
  static LogAdditionInfo *instance = new LogAdditionInfo;
  return instance;
}

LogAdditionInfo::LogAdditionInfo() {
  pthread_key_create(&business_id_key_, NULL);
}

LogAdditionInfo::~LogAdditionInfo() {
  pthread_key_delete(business_id_key_);
}

std::ostream& operator<<(std::ostream &out, const LogAdditionInfo &info) {
  void *value = pthread_getspecific(info.business_id_key_);
  if (value) {
    out << "[bid:" << reinterpret_cast<uint64>(value) << "] ";
  }

  return out;
}

void LogAdditionInfo::AddBusinessIDByThread(uint64 bid) {
  pthread_setspecific(business_id_key_, reinterpret_cast<void*>(bid));
}

void LogAdditionInfo::RemoveBusinessIDByThread() {
  pthread_setspecific(business_id_key_, NULL);
}

}  // namespace logging

std::ostream& operator<<(std::ostream& out, const wchar_t* wstr) {
  return out << WideToUTF8(std::wstring(wstr));
}
