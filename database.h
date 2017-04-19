#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <map>

#include "sqlite3.h"
#include <nan.h>
#include <vector>

class ConnectWorker;
class Statement;

struct AggregateContext {
  Nan::Persistent<v8::Object> *context;
};

class Database : public Nan::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

  void SetLastError(int code);

  inline sqlite3 *GetDatabase() { return db_; };

  void AddStatement(Statement *statement);

  void RemoveStatement(Statement *statement);

private:
  friend class OpenWorker;

  explicit Database();

  ~Database();

  static NAN_METHOD(New);

  static NAN_METHOD(Open);

  static NAN_METHOD(Close);

  static NAN_METHOD(CreateFunction);

  static NAN_METHOD(LastError);

  static NAN_METHOD(LastInsertID);

  static Nan::Persistent<v8::Function> constructor;

  static void CustomFunctionMain(sqlite3_context *context, int argc, sqlite3_value **argv);

  static void CustomFunctionStep(sqlite3_context *context, int argc, sqlite3_value **argv);

  static void CustomFunctionFinal(sqlite3_context *context);

  static void CustomFunctionDestroy(void *pointer);

  static void SetResult(sqlite3_context *context, v8::Local<v8::Value> result);

  void Close();

  std::string lastErrorMessage_;

  std::map<std::string, std::string> lastError_;

  sqlite3 *db_;

  std::vector<Statement *> statements_;
};

#endif
