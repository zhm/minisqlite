#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <map>

#include "sqlite3.h"
#include <nan.h>

class ConnectWorker;

struct AggregateContext {
  Nan::Persistent<v8::Object> *context;
};

class Client : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

private:
  friend class ConnectWorker;

  explicit Client();

  ~Client();

  static NAN_METHOD(New);

  static NAN_METHOD(Connect);

  static NAN_METHOD(Query);

  static NAN_METHOD(GetResults);

  static NAN_METHOD(Close);

  static NAN_METHOD(IsFinished);

  static NAN_METHOD(LastError);

  static NAN_METHOD(LastInsertID);

  static NAN_METHOD(CreateFunction);

  static Nan::Persistent<v8::Function> constructor;

  static void CustomFunctionMain(sqlite3_context *context, int argc, sqlite3_value **argv);

  static void CustomFunctionStep(sqlite3_context *context, int argc, sqlite3_value **argv);

  static void CustomFunctionFinal(sqlite3_context *context);

  static void CustomFunctionDestroy(void *pointer);

  void Close();

  void SetLastError(int code);

  void CreateNextStatement();

  void FinalizeStatement();

  v8::Local<v8::Value> ProcessSingleResult(bool returnMetadata);

  static v8::Local<v8::Object> CreateResult(sqlite3_stmt *statement, bool includeValues, bool includeMetadata);

  static v8::Local<v8::Object> ConvertValues(int count, sqlite3_value **values);

  static void SetResult(sqlite3_context *context, v8::Local<v8::Value> result);

  sqlite3 *connection_;

  sqlite3_stmt *statement_;

  std::string sql_;

  std::string lastErrorMessage_;

  std::map<std::string, std::string> lastError_;

  bool finished_;

  bool empty_;
};

#endif
