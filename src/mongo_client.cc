#include "src/mongo_client.h"

#include <mongoc.h>
#include "deps/jsoncpp/include/json/json.h"
#include "deps/base/flags.h"
#include "deps/base/logging.h"
#include "deps/base/string_util.h"
#include "deps/base/concurrent_queue.h"

DEFINE_string(mongo_user_db_uri,
              "mongodb://mongodb.liveaa.com:27017",
              "");
DEFINE_int32(mongo_connection_pool_size, 4, "");
DEFINE_string(mongo_user_db_name, "xuexibao2", "");
DEFINE_string(mongo_user_collection_name, "users", "");

namespace xcomet {

struct MongoRequest {
  StringPtr devid;
  GetUserNameCallback cb;
};

struct MongoConnection {
  mongoc_client_t* client;
  mongoc_collection_t* collection;

  MongoConnection(const char* uri, const char* db, const char* coll_name) {
    client = mongoc_client_new(uri);
    CHECK(client);
    collection = mongoc_client_get_collection(client, db, coll_name);
    CHECK(collection);
  }
  ~MongoConnection() {
    if (collection) mongoc_collection_destroy(collection);
    if (client) mongoc_client_destroy(client);
  }
  mongoc_cursor_t* FindOne(const bson_t* query, const bson_t* fields) {
    return mongoc_collection_find(collection, MONGOC_QUERY_NONE,
        0, 1, 0, query, fields, NULL);
  }
  
};

struct MongoClientPrivate {
  const int conn_pool_size;
  const string db_uri;
  const string db_name;
  const string collection_name;
  bson_t* fields;
  vector<thread> mongo_worker_threads;
  vector<shared_ptr<MongoConnection> > mongo_conns;
  base::ConcurrentQueue<shared_ptr<MongoRequest> > request_queue;
  std::atomic<bool> stoped;

  MongoClientPrivate()
      : conn_pool_size(FLAGS_mongo_connection_pool_size),
        db_uri(FLAGS_mongo_user_db_uri),
        db_name(FLAGS_mongo_user_db_name),
        collection_name(FLAGS_mongo_user_collection_name),
        fields(BCON_NEW("name", BCON_INT32(1))),
        stoped(false) {
    CHECK(conn_pool_size > 0);
    mongo_worker_threads.resize(conn_pool_size);
    mongo_conns.resize(conn_pool_size);
    mongoc_init();
    for (int i = 0; i < conn_pool_size; ++i) {
      mongo_conns[i].reset(new MongoConnection(db_uri.c_str(),
                                               db_name.c_str(),
                                               collection_name.c_str()));
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
    bson_destroy(fields);
    mongoc_cleanup();
  }

  void Run(shared_ptr<MongoConnection> conn) {
    while (!stoped) {
      shared_ptr<MongoRequest> req;
      request_queue.Pop(req);
      if (req.get() == NULL) {
        continue;
      }
      bson_t* query = BCON_NEW("deviceid", BCON_UTF8(req->devid->c_str()));
      mongoc_cursor_t* cursor = conn->FindOne(query, fields);
      const bson_t* doc;
      bson_error_t error;
      if (mongoc_cursor_next(cursor, &doc)) {
        char* json_str = bson_as_json(doc, NULL);
        VLOG(4) << "mongo response: " << json_str;
        Json::Value json;
        if (json_str != NULL &&
            Json::Reader().parse(json_str, json) &&
            json.isMember("name")) {
          StringPtr name(new string(json["name"].asCString()));
          req->cb(NULL, name);
        } else {
          LOG(ERROR) << "mongo response is invalid: " << json_str;
          req->cb("invalid mongo response", NULL);
        }
        bson_free(json_str);
      } else if (mongoc_cursor_error(cursor, &error)) {
        LOG(ERROR) << "mongo cursor error: " << error.message;
        req->cb(error.message, NULL);
      } else {
        req->cb(NULL, NULL);
      }
      mongoc_cursor_destroy(cursor);
      bson_destroy(query);
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
