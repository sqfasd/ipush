#include <mongoc.h>

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "mongoc-tests.h"

static char *gTestUri;


static void
test_has_collection (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_error_t error;
   char *name;
   bool r;
   bson_oid_t oid;
   bson_t b;

   client = mongoc_client_new (gTestUri);
   assert (client);

   name = gen_collection_name ("has_collection");
   collection = mongoc_client_get_collection (client, "test", name);
   assert (collection);

   database = mongoc_client_get_database (client, "test");
   assert (database);

   bson_init (&b);
   bson_oid_init (&oid, NULL);
   bson_append_oid (&b, "_id", 3, &oid);
   bson_append_utf8 (&b, "hello", 5, "world", 5);
   r = mongoc_collection_insert (collection, MONGOC_INSERT_NONE, &b, NULL,
                                 &error);
   if (!r) {
      MONGOC_WARNING ("%s\n", error.message);
   }
   assert (r);
   bson_destroy (&b);

   r = mongoc_database_has_collection (database, name, &error);
   assert (!error.domain);
   assert (r);

   bson_free (name);
   mongoc_database_destroy (database);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_command (void)
{
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   bool r;
   bson_t cmd = BSON_INITIALIZER;
   bson_t reply;

   client = mongoc_client_new (gTestUri);
   assert (client);

   database = mongoc_client_get_database (client, "admin");

   /*
    * Test a known working command, "ping".
    */
   bson_append_int32 (&cmd, "ping", 4, 1);

   cursor = mongoc_database_command (database, MONGOC_QUERY_NONE, 0, 1, 0, &cmd, NULL, NULL);
   assert (cursor);

   r = mongoc_cursor_next (cursor, &doc);
   assert (r);
   assert (doc);

   r = mongoc_cursor_next (cursor, &doc);
   assert (!r);
   assert (!doc);

   mongoc_cursor_destroy (cursor);


   /*
    * Test a non-existing command to ensure we get the failure.
    */
   bson_reinit (&cmd);
   bson_append_int32 (&cmd, "a_non_existing_command", -1, 1);

   r = mongoc_database_command_simple (database, &cmd, NULL, &reply, &error);
   assert (!r);
   assert (error.domain == MONGOC_ERROR_QUERY);
   assert (error.code == MONGOC_ERROR_QUERY_COMMAND_NOT_FOUND);
   assert (strstr (error.message, "a_non_existing_command"));

   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   bson_destroy (&cmd);
}


static void
test_drop (void)
{
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   char *dbname;
   bool r;

   client = mongoc_client_new (gTestUri);
   assert (client);

   dbname = gen_collection_name ("db_drop_test");
   database = mongoc_client_get_database (client, dbname);
   bson_free (dbname);

   r = mongoc_database_drop (database, &error);
   assert (r);
   assert (!error.domain);
   assert (!error.code);

   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}


static void
test_create_collection (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   bson_t options;
   bson_t storage_opts;
   bson_t wt_opts;

   char *dbname;
   char *name;
   bool r;

   client = mongoc_client_new (gTestUri);
   assert (client);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);
   assert (database);
   bson_free (dbname);

   bson_init (&options);
   BSON_APPEND_INT32 (&options, "size", 1234);
   BSON_APPEND_INT32 (&options, "max", 4567);
   BSON_APPEND_BOOL (&options, "capped", true);
   BSON_APPEND_BOOL (&options, "autoIndexId", true);

   BSON_APPEND_DOCUMENT_BEGIN(&options, "storage", &storage_opts);
   BSON_APPEND_DOCUMENT_BEGIN(&storage_opts, "wiredtiger", &wt_opts);
   BSON_APPEND_UTF8(&wt_opts, "configString", "block_compressor=zlib");
   bson_append_document_end(&storage_opts, &wt_opts);
   bson_append_document_end(&options, &storage_opts);


   name = gen_collection_name ("create_collection");
   collection = mongoc_database_create_collection (database, name, &options, &error);
   assert (collection);
   bson_destroy (&options);
   bson_free (name);

   r = mongoc_collection_drop (collection, &error);
   assert (r);

   r = mongoc_database_drop (database, &error);
   assert (r);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_get_collection_info (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error = { 0 };
   bson_iter_t col_iter;
   bson_t capped_options = BSON_INITIALIZER;
   bson_t autoindexid_options = BSON_INITIALIZER;
   bson_t noopts_options = BSON_INITIALIZER;
   bson_t name_filter = BSON_INITIALIZER;
   const bson_t *doc;
   int r;
   int num_infos = 0;

   const char *name;
   char *dbname;
   char *capped_name;
   char *autoindexid_name;
   char *noopts_name;

   client = mongoc_client_new (gTestUri);
   assert (client);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);

   assert (database);
   bson_free (dbname);

   capped_name = gen_collection_name ("capped");
   BSON_APPEND_BOOL (&capped_options, "capped", true);
   BSON_APPEND_INT32 (&capped_options, "size", 10000000);
   BSON_APPEND_INT32 (&capped_options, "max", 1024);

   autoindexid_name = gen_collection_name ("autoindexid");
   BSON_APPEND_BOOL (&autoindexid_options, "autoIndexId", false);

   noopts_name = gen_collection_name ("noopts");

   collection = mongoc_database_create_collection (database, capped_name,
                                                   &capped_options, &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (database, autoindexid_name,
                                                   &autoindexid_options,
                                                   &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (database, noopts_name,
                                                   &noopts_options, &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   /* first we filter on collection name. */
   BSON_APPEND_UTF8 (&name_filter, "name", noopts_name);

   /* We only test with filters since get_collection_names will
    * test w/o filters for us. */

   /* Filter on an exact match of name */
   cursor = mongoc_database_find_collections (database, &name_filter, &error);
   assert (cursor);
   assert (!error.domain);
   assert (!error.code);

   while (mongoc_cursor_next (cursor, &doc)) {
      if (bson_iter_init (&col_iter, doc) &&
          bson_iter_find (&col_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&col_iter) &&
          (name = bson_iter_utf8 (&col_iter, NULL))) {
         ++num_infos;
         assert (0 == strcmp (name, noopts_name));
      } else {
         assert (false);
      }
   }

   assert (1 == num_infos);

   num_infos = 0;
   mongoc_cursor_destroy (cursor);

   r = mongoc_database_drop (database, &error);
   assert (r);
   assert (!error.domain);
   assert (!error.code);

   bson_free (capped_name);
   bson_free (noopts_name);
   bson_free (autoindexid_name);

   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_get_collection_names (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error = { 0 };
   bson_t options;
   int r;
   int namecount = 0;

   char **names;
   char **name;
   char *curname;

   char *dbname;
   char *name1;
   char *name2;
   char *name3;
   char *name4;
   char *name5;
   const char *system_prefix = "system.";

   client = mongoc_client_new (gTestUri);
   assert (client);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);

   assert (database);
   bson_free (dbname);

   bson_init (&options);

   name1 = gen_collection_name ("name1");
   name2 = gen_collection_name ("name2");
   name3 = gen_collection_name ("name3");
   name4 = gen_collection_name ("name4");
   name5 = gen_collection_name ("name5");

   collection = mongoc_database_create_collection (database, name1, &options,
                                                   &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (database, name2, &options,
                                                   &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (database, name3, &options,
                                                   &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (database, name4, &options,
                                                   &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   collection = mongoc_database_create_collection (database, name5, &options,
                                                   &error);
   assert (collection);
   mongoc_collection_destroy (collection);

   names = mongoc_database_get_collection_names (database, &error);
   assert (!error.domain);
   assert (!error.code);

   for (name = names; *name; ++name) {
      /* inefficient, but OK for a unit test. */
      curname = *name;

      if (0 == strcmp (curname, name1) ||
          0 == strcmp (curname, name2) ||
          0 == strcmp (curname, name3) ||
          0 == strcmp (curname, name4) ||
          0 == strcmp (curname, name5)) {
         ++namecount;
      } else if (0 ==
                 strncmp (curname, system_prefix, strlen (system_prefix))) {
         /* Collections prefixed with 'system.' are system collections */
      } else {
         assert (false);
      }

      bson_free (curname);
   }

   assert (namecount == 5);

   bson_free (name1);
   bson_free (name2);
   bson_free (name3);
   bson_free (name4);
   bson_free (name5);

   bson_free (names);

   r = mongoc_database_drop (database, &error);
   assert (r);
   assert (!error.domain);
   assert (!error.code);

   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
cleanup_globals (void)
{
   bson_free (gTestUri);
}


void
test_database_install (TestSuite *suite)
{
   gTestUri = bson_strdup_printf ("mongodb://%s/", MONGOC_TEST_HOST);

   TestSuite_Add (suite, "/Database/has_collection", test_has_collection);
   TestSuite_Add (suite, "/Database/command", test_command);
   TestSuite_Add (suite, "/Database/drop", test_drop);
   TestSuite_Add (suite, "/Database/create_collection", test_create_collection);
   TestSuite_Add (suite, "/Database/get_collection_info",
                  test_get_collection_info);
   TestSuite_Add (suite, "/Database/get_collection_names",
                  test_get_collection_names);

   atexit (cleanup_globals);
}
