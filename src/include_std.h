#ifndef SRC_INCLUDE_STD_H_
#define SRC_INCLUDE_STD_H_

#include <unistd.h>
#include <signal.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <utility>
#include <iostream>

using std::string;
using std::vector;
using std::set;
using std::map;
using std::queue;
using std::make_pair;
using std::iostream;
using std::ostream;

#if __cplusplus >= 201103L
#include <memory>
#include <thread>
#include <functional>
#include <atomic>

using std::shared_ptr;
using std::thread;
using std::function;
using std::bind;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::atomic;
#else
// #include <boost/thread.hpp>
// #include <boost/shared_ptr.hpp>
// #include <boost/bind.hpp>
// #include <boost/function.hpp>
// 
// using boost::shared_ptr;
// using boost::thread;
// using boost::function;
// using boost::bind;
#endif

#endif  // SRC_INCLUDE_STD_H_
