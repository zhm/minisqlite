#ifndef __STATEMENT_H__
#define __STATEMENT_H__

#include <map>

#include "sqlite3.h"
#include "database.h"
#include <nan.h>

class ConnectWorker;

class Statement : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

  static v8::Local<v8::Object> ConvertValues(int count, sqlite3_value **values);

  void Finalize();

private:
  explicit Statement();

  ~Statement();

  static NAN_METHOD(New);

  static NAN_METHOD(Query);

  static NAN_METHOD(GetResults);

  static NAN_METHOD(Close);

  static NAN_METHOD(IsFinished);

  static NAN_METHOD(CreateFunction);

  static Nan::Persistent<v8::Function> constructor;

  void Close();

  void CreateNextStatement();

  v8::Local<v8::Value> ProcessSingleResult(bool returnMetadata);

  static v8::Local<v8::Object> CreateResult(sqlite3_stmt *statement, bool includeValues, bool includeMetadata);

  Database *database_;

  sqlite3_stmt *statement_;

  std::string sql_;

  std::string lastErrorMessage_;

  std::map<std::string, std::string> lastError_;

  bool finished_;

  bool empty_;
};

#endif
