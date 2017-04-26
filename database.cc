#include "database.h"
#include "statement.h"
#include "open-worker.h"
#include "gpkg/gpkg.h"

#include <iostream>

Nan::Persistent<v8::Function> Database::constructor;

Database::Database() : db_(nullptr), statements_() {
}

Database::~Database() {
  Close();
}

void Database::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Database").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "open", Open);
  Nan::SetPrototypeMethod(tpl, "lastError", LastError);
  Nan::SetPrototypeMethod(tpl, "lastInsertID", LastInsertID);
  Nan::SetPrototypeMethod(tpl, "close", Close);
  Nan::SetPrototypeMethod(tpl, "createFunction", CreateFunction);

  constructor.Reset(tpl->GetFunction());

  exports->Set(Nan::New("Database").ToLocalChecked(), tpl->GetFunction());

  sqlite3_auto_extension((void(*)(void))sqlite3_gpkg_init);
}

NAN_METHOD(Database::New) {
  if (info.IsConstructCall()) {
    Database *obj = new Database();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(Database::Open) {
  std::string connectionString = *Nan::Utf8String(info[0]);
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
  std::string vfs = "";

  if (info.Length() >= 1 && info[1]->IsInt32()) {
    flags = Nan::To<int>(info[1]).FromJust();
  }

  if (info.Length() >= 2 && info[2]->IsString()) {
    vfs = *Nan::Utf8String(info[2]);
  }

  Database* database = ObjectWrap::Unwrap<Database>(info.Holder());

  Nan::Callback *callback = new Nan::Callback(info[3].As<v8::Function>());

  Nan::AsyncQueueWorker(new OpenWorker(callback, database, connectionString, flags, vfs));

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Database::Close) {
  Database* database = ObjectWrap::Unwrap<Database>(info.Holder());

  while (!database->statements_.empty()) {
    database->statements_.at(0)->Finalize();
  }

  database->Close();

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Database::LastError) {
  Database* database = ObjectWrap::Unwrap<Database>(info.Holder());

  if (database->lastErrorMessage_.empty()) {
    info.GetReturnValue().SetNull();
  }
  else {
    auto errorObject = Nan::New<v8::Object>();

    for (auto const &entry : database->lastError_) {
      auto &key = entry.first;
      auto &value = entry.second;

      if (!value.empty()) {
        Nan::Set(errorObject, Nan::New(key).ToLocalChecked(),
                              Nan::New(entry.second).ToLocalChecked());
      }
      else {
        Nan::Set(errorObject, Nan::New(key).ToLocalChecked(),
                              Nan::Null());
      }
    }

    info.GetReturnValue().Set(errorObject);
  }
}

NAN_METHOD(Database::LastInsertID) {
  Database* database = ObjectWrap::Unwrap<Database>(info.Holder());

  auto id = Nan::New<v8::Number>(sqlite3_last_insert_rowid(database->db_));

  info.GetReturnValue().Set(id);
}

NAN_METHOD(Database::CreateFunction) {
  Database* database = ObjectWrap::Unwrap<Database>(info.Holder());

  std::string functionName = *Nan::Utf8String(info[0]);
  int numberOfArguments = Nan::To<int>(info[1]).FromJust();
  int textEncoding = Nan::To<int>(info[2]).FromJust();

  Nan::Callback *mainFunction =
    info[3]->IsFunction() ? new Nan::Callback(info[3].As<v8::Function>()) : nullptr;

  Nan::Callback *stepFunction =
    info[4]->IsFunction() ? new Nan::Callback(info[4].As<v8::Function>()) : nullptr;

  Nan::Callback *finalFunction =
    info[5]->IsFunction() ? new Nan::Callback(info[5].As<v8::Function>()) : nullptr;

  std::vector<Nan::Callback *> *callbacks = new std::vector<Nan::Callback *>();

  callbacks->push_back(mainFunction);
  callbacks->push_back(stepFunction);
  callbacks->push_back(finalFunction);

  auto result = sqlite3_create_function_v2(
    database->db_,
    functionName.c_str(),
    numberOfArguments,
    textEncoding,
    callbacks,
    mainFunction ? CustomFunctionMain : nullptr,
    stepFunction ? CustomFunctionStep : nullptr,
    finalFunction ? CustomFunctionFinal : nullptr,
    CustomFunctionDestroy
  );

  database->SetLastError(result);

  if (result != SQLITE_OK) {
    Nan::ThrowError(database->lastErrorMessage_.c_str());
    return;
  }

  auto id = Nan::New<v8::Number>(result);

  info.GetReturnValue().Set(id);
}

void Database::AddStatement(Statement *statement) {
  statements_.push_back(statement);
}

void Database::RemoveStatement(Statement *statement) {
  statements_.erase(std::remove(statements_.begin(), statements_.end(), statement), statements_.end());
}

void Database::CustomFunctionMain(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<Nan::Callback *> *callbacks = (std::vector<Nan::Callback *> *)sqlite3_user_data(context);

  v8::Local<v8::Value> arguments[] = { Statement::ConvertValues(argc, argv) };

  auto result = callbacks->at(0)->Call(1, &arguments[0]);

  SetResult(context, result);
}

void Database::CustomFunctionStep(sqlite3_context *context, int argc, sqlite3_value **argv) {
  std::vector<Nan::Callback *> *callbacks = (std::vector<Nan::Callback *> *)sqlite3_user_data(context);

  AggregateContext *agg = static_cast<AggregateContext *>(sqlite3_aggregate_context(context, sizeof(AggregateContext)));

  if (!agg->context) {
    auto resultObject = Nan::New<v8::Object>();
    agg->context = new Nan::Persistent<v8::Object>(resultObject);
  }

  v8::Local<v8::Value> arguments[] = { Statement::ConvertValues(argc, argv), Nan::New(*agg->context) };

  callbacks->at(1)->Call(2, &arguments[0]);
}

void Database::CustomFunctionFinal(sqlite3_context *context) {
  std::vector<Nan::Callback *> *callbacks = (std::vector<Nan::Callback *> *)sqlite3_user_data(context);

  AggregateContext *agg = static_cast<AggregateContext *>(sqlite3_aggregate_context(context, sizeof(AggregateContext)));

  v8::Local<v8::Value> arguments[] = { Nan::New(*agg->context) };

  auto result = callbacks->at(2)->Call(1, &arguments[0]);

  if (agg->context) {
    agg->context->Reset();
    delete agg->context;
  }

  SetResult(context, result);
}

void Database::CustomFunctionDestroy(void *pointer) {
  std::vector<Nan::Callback *> *callbacks = (std::vector<Nan::Callback *> *)pointer;

  if (callbacks->at(0)) {
    delete callbacks->at(0);
  }
  if (callbacks->at(1)) {
    delete callbacks->at(1);
  }
  if (callbacks->at(2)) {
    delete callbacks->at(2);
  }

  delete callbacks;
}

void Database::Close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void Database::SetLastError(int code) {
  switch (code) {
    case SQLITE_OK:
    case SQLITE_ROW:
      lastErrorMessage_ = "";
      break;

    case SQLITE_MISUSE:
      lastErrorMessage_ = "misuse";
      break;

    default:
      lastErrorMessage_ = sqlite3_errmsg(db_);
      break;
  }

  if (lastErrorMessage_.empty()) {
    return;
  }

  lastError_["message"] = lastErrorMessage_;
}

void Database::SetResult(sqlite3_context *context, v8::Local<v8::Value> result) {
  if (result->IsNumber()) {
    sqlite3_result_int64(context, (sqlite3_int64)Nan::To<int64_t>(result).FromJust());
  } else if (result->IsNull() || result->IsUndefined()) {
    sqlite3_result_null(context);
  } else if (result->IsString()) {
    sqlite3_result_text(context, *Nan::Utf8String(result), -1, SQLITE_TRANSIENT);
  } else if (result->IsBoolean()) {
    sqlite3_result_int64(context, (int)Nan::To<bool>(result).FromJust());
  } else if (node::Buffer::HasInstance(result)) {
    v8::Local<v8::Object> buffer = Nan::To<v8::Object>(result).ToLocalChecked();
    sqlite3_result_blob(context, node::Buffer::Data(buffer), node::Buffer::Length(buffer), SQLITE_TRANSIENT);
  } else {
    sqlite3_result_text(context, *Nan::Utf8String(result->ToString()), -1, SQLITE_TRANSIENT);
  }
}
