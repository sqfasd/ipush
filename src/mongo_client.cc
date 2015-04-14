#include "src/mongo_client.h"

#include <mongo/client/dbclient.h>
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "deps/base/string_util.h"
#include "deps/base/concurrent_queue.h"

DEFINE_string(mongo_user_db_uri,
              "mongodb.liveaa.com:27017",
              "");
DEFINE_int32(mongo_connection_pool_size, 4, "");
DEFINE_string(mongo_user_collection_name, "xuexibao2.users", "");

namespace xcomet {

using namespace mongo;

const bool AUTO_RECONNECT = true;

struct MongoRequest {
  StringPtr devid;
  GetUserNameCallback cb;
};

struct MongoClientPrivate {
  vector<thread> mongo_worker_threads;
  vector<shared_ptr<DBClientConnection> > mongo_conns;
  base::ConcurrentQueue<shared_ptr<MongoRequest> > request_queue;
  const int conn_pool_size;
  const string collection_name;
  const BSONObj fields;
  std::atomic<bool> stoped;

  MongoClientPrivate()
      : conn_pool_size(FLAGS_mongo_connection_pool_size),
        collection_name(FLAGS_mongo_user_collection_name),
        fields(BSONObjBuilder().append("name", 1).obj()),
        stoped(false) {
    CHECK(conn_pool_size > 0);
    mongo_worker_threads.resize(conn_pool_size);
    mongo_conns.resize(conn_pool_size);
    for (int i = 0; i < conn_pool_size; ++i) {
      mongo_conns[i].reset(new DBClientConnection(AUTO_RECONNECT));
      string err;
      CHECK(mongo_conns[i]->connect(FLAGS_mongo_user_db_uri, err))
          << "failed to connect mongo: " << err;
    }
    for (int i = 0; i < conn_pool_size; ++i) {
      mongo_worker_threads[i] = thread(&MongoClientPrivate::Run, this, mongo_conns[i]);
    }
  }

  ~MongoClientPrivate() {
    LOG(INFO) << "~MongoClientPrivate";
    stoped = true;
    for (int i = 0; i < conn_pool_size; ++i) {
      request_queue.Push(NULL);
    }
    for (int i = 0; i < conn_pool_size; ++i) {
      LOG(INFO) << "join mongo worker thread " << i;
      if (mongo_worker_threads[i].joinable()) {
        mongo_worker_threads[i].join();
      }
    }
  }

  void Run(shared_ptr<DBClientConnection> conn) {
    while (!stoped) {
      shared_ptr<MongoRequest> req;
      request_queue.Pop(req);
      if (req.get() == NULL) {
        continue;
      }
      try {
        string query_str = StringPrintf("{deviceid:\"%s\"}",
            req->devid->c_str());
        BSONObj res = conn->findOne(
            collection_name,
            Query(query_str),
            &fields);
        VLOG(4) << "mongo response: " << res;
        StringPtr name(new string(res.getStringField("name")));
        req->cb(NULL, name);
      } catch (std::exception& e) {
        LOG(WARNING) << "mongo query exception: " << e.what();
        req->cb(e.what(), NULL);
      }
    }
  }
};

MongoClient::MongoClient() {
  LOG(INFO) << "MongoClient()";
  p_.reset(new MongoClientPrivate());
}

MongoClient::~MongoClient() {
  LOG(INFO) << "~MongoClient()";
}

void MongoClient::GetUserNameByDevId(const StringPtr& devid,
                                     const GetUserNameCallback& cb) {
  shared_ptr<MongoRequest> req(new MongoRequest());
  req->devid = devid;
  req->cb = cb;
  p_->request_queue.Push(req);
}

}  // namespace xcomet
