#include "client.h"
#include "connect-worker.h"

#include <iostream>

static const int RESULT_BATCH_SIZE = 100;

Nan::Persistent<v8::Function> Client::constructor;

Client::Client() : connection_(nullptr), statement_(nullptr), finished_(true), empty_(true) {
}

Client::~Client() {
  Close();
}

void Client::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Client").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "connect", Connect);
  Nan::SetPrototypeMethod(tpl, "query", Query);
  Nan::SetPrototypeMethod(tpl, "close", Close);
  Nan::SetPrototypeMethod(tpl, "getResults", GetResults);
  Nan::SetPrototypeMethod(tpl, "lastError", LastError);
  Nan::SetPrototypeMethod(tpl, "finished", IsFinished);

  constructor.Reset(tpl->GetFunction());

  exports->Set(Nan::New("Client").ToLocalChecked(), tpl->GetFunction());
}

NAN_METHOD(Client::New) {
  if (info.IsConstructCall()) {
    Client *obj = new Client();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(cons->NewInstance(argc, argv));
  }
}

NAN_METHOD(Client::Connect) {
  std::string connectionString = *Nan::Utf8String(info[0]);
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
  std::string vfs = "";

  if (info.Length() >= 1 && info[1]->IsInt32()) {
    flags = Nan::To<int>(info[1]).FromJust();
  }

  if (info.Length() >= 2 && info[2]->IsString()) {
    vfs = *Nan::Utf8String(info[2]);
  }

  Client* client = ObjectWrap::Unwrap<Client>(info.Holder());

  Nan::Callback *callback = new Nan::Callback(info[3].As<v8::Function>());

  Nan::AsyncQueueWorker(new ConnectWorker(callback, client, connectionString, flags, vfs));

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Client::Close) {
  Client* client = ObjectWrap::Unwrap<Client>(info.Holder());

  client->Close();

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Client::LastError) {
  Client* client = ObjectWrap::Unwrap<Client>(info.Holder());

  if (client->lastErrorMessage_.empty()) {
    info.GetReturnValue().SetNull();
  }
  else {
    auto errorObject = Nan::New<v8::Object>();

    for (auto const &entry : client->lastError_) {
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

NAN_METHOD(Client::IsFinished) {
  Client* client = ObjectWrap::Unwrap<Client>(info.Holder());

  info.GetReturnValue().Set(Nan::New(client->finished_));
}

NAN_METHOD(Client::Query) {
  Client* client = ObjectWrap::Unwrap<Client>(info.Holder());

  Nan::Utf8String commandText(info[0]);

  client->finished_ = false;
  client->empty_ = true;
  client->sql_ = *commandText;

  client->CreateNextStatement();

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Client::GetResults) {
  Client* client = ObjectWrap::Unwrap<Client>(info.Holder());

  bool returnMetadata = false;

  if (!info[0]->IsUndefined()) {
    returnMetadata = Nan::To<bool>(info[0]).FromMaybe(false);
  }

  auto results = Nan::New<v8::Array>();

  int index = 0;

  while (true) {
    auto result = client->ProcessSingleResult(returnMetadata && index == 0);

    if (client->finished_) {
      break;
    }

    Nan::Set(results, index, result);

    ++index;

    if (index >= RESULT_BATCH_SIZE || client->empty_) {
      break;
    }
  }

  info.GetReturnValue().Set(results);
}

v8::Local<v8::Value> Client::ProcessSingleResult(bool returnMetadata) {
  if (!statement_) {
    finished_ = true;
    return Nan::Null();
  }

  int code = sqlite3_step(statement_);

  empty_ = true;

  SetLastError();

  switch (code) {
    case SQLITE_BUSY: {
      empty_ = true;
      return Nan::Null();
      break;
    }

    case SQLITE_DONE: {
      empty_ = true;

      auto resultObject = CreateResult(statement_, false, returnMetadata);

      CreateNextStatement();

      return resultObject;
      break;
    }

    case SQLITE_ERROR:
    case SQLITE_MISUSE: {
      empty_ = true;
      FinalizeStatement();
      return Nan::Null();
      break;
    }

    case SQLITE_ROW: {
      empty_ = false;

      auto resultObject = CreateResult(statement_, true, returnMetadata);

      return resultObject;
      break;
    }
  }
}

void Client::FinalizeStatement() {
  if (statement_) {
    sqlite3_finalize(statement_);
    statement_ = nullptr;
  }
}

void Client::CreateNextStatement() {
  FinalizeStatement();

  const char *rest = NULL;

  int result = sqlite3_prepare_v2(connection_, sql_.c_str(), -1, &statement_, &rest);

  SetLastError();

  if (result != SQLITE_OK) {
    sql_ = "";
    return;
  }

  if (rest) {
    sql_ = rest;
  }

  if (statement_) {
    sqlite3_reset(statement_);
    sqlite3_clear_bindings(statement_);
  }
}

void Client::Close() {
  FinalizeStatement();

  if (connection_) {
    sqlite3_close(connection_);
    connection_ = nullptr;
  }
}

void Client::SetLastError() {
  int code = sqlite3_errcode(connection_);

  lastErrorMessage_ = code == SQLITE_OK || code == SQLITE_ROW ? "" : sqlite3_errmsg(connection_);

  if (lastErrorMessage_.empty()) {
    return;
  }

  lastError_["message"] = lastErrorMessage_;
}

v8::Local<v8::Object> Client::CreateResult(sqlite3_stmt *statement, bool includeValues, bool includeMetadata) {
  int fieldCount = sqlite3_column_count(statement);

  auto resultObject = Nan::New<v8::Object>();
  auto columns = Nan::New<v8::Array>();
  auto values = Nan::New<v8::Array>();

  for (int i = 0; i < fieldCount; ++i) {
    if (includeMetadata) {
      auto column = Nan::New<v8::Object>();

      const char *columnName = sqlite3_column_name(statement, i);
      int columnType = sqlite3_column_type(statement, i);
      const char *columnTable = sqlite3_column_table_name(statement, i);
      int columnNumber = i + 1;

      if (columnName) {
        Nan::Set(column, Nan::New("name").ToLocalChecked(),
                         Nan::New(columnName).ToLocalChecked());
      } else {
        Nan::Set(column, Nan::New("name").ToLocalChecked(), Nan::Null());
      }

      if (columnTable) {
        Nan::Set(column, Nan::New("table").ToLocalChecked(),
                         Nan::New(columnTable).ToLocalChecked());
      } else {
        Nan::Set(column, Nan::New("table").ToLocalChecked(), Nan::Null());
      }

      Nan::Set(column, Nan::New("column").ToLocalChecked(),
                       Nan::New(columnNumber));

      Nan::Set(column, Nan::New("type").ToLocalChecked(),
                       Nan::New(columnType));

      Nan::Set(columns, i, column);
    }

    if (includeValues) {
      int columnType = sqlite3_column_type(statement, i);

      switch (columnType) {
        case SQLITE_NULL:
           Nan::Set(values, i, Nan::Null());
           break;

        case SQLITE_TEXT:
           Nan::Set(values, i, Nan::New((const char *)sqlite3_column_text(statement, i)).ToLocalChecked());
           break;

        case SQLITE_FLOAT:
           Nan::Set(values, i, Nan::New(sqlite3_column_double(statement, i)));
           break;

        case SQLITE_INTEGER:
           Nan::Set(values, i, Nan::New(sqlite3_column_int(statement, i)));
           break;

        case SQLITE_BLOB:
           const void *data = sqlite3_column_blob(statement, i);
           int size = sqlite3_column_bytes(statement, i);
           Nan::Set(values, i, Nan::CopyBuffer((char *)data, size).ToLocalChecked());
           break;
      }
    }
  }

  if (includeMetadata) {
    Nan::Set(resultObject, Nan::New("columns").ToLocalChecked(), columns);
  }

  if (includeValues) {
    Nan::Set(resultObject, Nan::New("values").ToLocalChecked(), values);
  }

  return resultObject;
}
