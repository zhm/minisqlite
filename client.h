#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <map>

#include "sqlite3.h"
#include <nan.h>

class ConnectWorker;

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

  static Nan::Persistent<v8::Function> constructor;

  void Close();

  void SetLastError();

  void CreateNextStatement();

  void FinalizeStatement();

  v8::Local<v8::Value> ProcessSingleResult(bool returnMetadata);

  static v8::Local<v8::Object> CreateResult(sqlite3_stmt *statement, bool includeValues, bool includeMetadata);

  sqlite3 *connection_;

  sqlite3_stmt *statement_;

  std::string sql_;

  std::string lastErrorMessage_;

  std::map<std::string, std::string> lastError_;

  bool finished_;

  bool empty_;
};

#endif
