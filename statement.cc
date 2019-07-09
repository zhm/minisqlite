#include "statement.h"

#include <iostream>

static const int RESULT_BATCH_SIZE = 100;

Nan::Persistent<v8::Function> Statement::constructor;

Statement::Statement() : database_(nullptr), statement_(nullptr), finished_(true), empty_(true) {
}

Statement::~Statement() {
  Close();
}

void Statement::Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Statement").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "query", Query);
  Nan::SetPrototypeMethod(tpl, "close", Close);
  Nan::SetPrototypeMethod(tpl, "getResults", GetResults);
  Nan::SetPrototypeMethod(tpl, "finished", IsFinished);

  auto function = Nan::GetFunction(tpl).ToLocalChecked();

  constructor.Reset(function);

  Nan::Set(exports, Nan::New("Statement").ToLocalChecked(), function);
}

NAN_METHOD(Statement::New) {
  if (info.IsConstructCall()) {
    Statement *obj = new Statement();
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 1;
    v8::Local<v8::Value> argv[argc] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(Statement::Close) {
  Statement* statement = ObjectWrap::Unwrap<Statement>(info.Holder());

  statement->Close();

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Statement::IsFinished) {
  Statement* statement = ObjectWrap::Unwrap<Statement>(info.Holder());

  info.GetReturnValue().Set(Nan::New(statement->finished_));
}

NAN_METHOD(Statement::Query) {
  Statement* statement = ObjectWrap::Unwrap<Statement>(info.Holder());

  Database* db = ObjectWrap::Unwrap<Database>(info[0].As<v8::Object>());
  Nan::Utf8String commandText(info[1]);

  statement->database_ = db;
  statement->finished_ = false;
  statement->empty_ = true;
  statement->sql_ = *commandText;

  statement->CreateNextStatement();

  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Statement::GetResults) {
  Statement* statement = ObjectWrap::Unwrap<Statement>(info.Holder());

  bool returnMetadata = false;

  if (!info[0]->IsUndefined()) {
    returnMetadata = Nan::To<bool>(info[0]).FromMaybe(false);
  }

  auto results = Nan::New<v8::Array>();

  int index = 0;

  while (true) {
    auto result = statement->ProcessSingleResult(returnMetadata && index == 0);

    if (statement->finished_) {
      break;
    }

    Nan::Set(results, index, result);

    ++index;

    if (index >= RESULT_BATCH_SIZE || statement->empty_) {
      break;
    }
  }

  info.GetReturnValue().Set(results);
}

v8::Local<v8::Value> Statement::ProcessSingleResult(bool returnMetadata) {
  empty_ = true;

  if (!statement_) {
    finished_ = true;
    return Nan::Null();
  }

  int code = sqlite3_step(statement_);

  database_->SetLastError(code);

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

    case SQLITE_ROW: {
      empty_ = false;

      auto resultObject = CreateResult(statement_, true, returnMetadata);

      return resultObject;
      break;
    }

    default: {
      // SQLITE_ERROR
      // SQLITE_MISUSE
      empty_ = true;
      Finalize();
      return Nan::Null();
      break;
    }
  }
}

void Statement::Finalize() {
  if (statement_) {
    sqlite3_finalize(statement_);
    statement_ = nullptr;
    database_->RemoveStatement(this);
  }
}

void Statement::CreateNextStatement() {
  Finalize();

  const char *rest = NULL;

  int result = sqlite3_prepare_v2(database_->GetDatabase(), sql_.c_str(), -1, &statement_, &rest);

  database_->SetLastError(result);

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
    database_->AddStatement(this);
  }
}

void Statement::Close() {
  Finalize();
}

v8::Local<v8::Object> Statement::CreateResult(sqlite3_stmt *statement, bool includeValues, bool includeMetadata) {
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
           Nan::Set(values, i, Nan::New<v8::Number>(sqlite3_column_int64(statement, i)));
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

v8::Local<v8::Object> Statement::ConvertValues(int count, sqlite3_value **values) {
  auto resultObject = Nan::New<v8::Array>();

  for (int i = 0; i < count; ++i) {
    auto value = values[i];

    int valueType = sqlite3_value_type(value);

    switch (valueType) {
      case SQLITE_NULL:
         Nan::Set(resultObject, i, Nan::Null());
         break;

      case SQLITE_TEXT:
         Nan::Set(resultObject, i, Nan::New((const char *)sqlite3_value_text(value)).ToLocalChecked());
         break;

      case SQLITE_FLOAT:
         Nan::Set(resultObject, i, Nan::New(sqlite3_value_double(value)));
         break;

      case SQLITE_INTEGER:
         Nan::Set(resultObject, i, Nan::New<v8::Number>(sqlite3_value_int64(value)));
         break;

      case SQLITE_BLOB:
         const void *data = sqlite3_value_blob(value);
         int size = sqlite3_value_bytes(value);
         Nan::Set(resultObject, i, Nan::CopyBuffer((char *)data, size).ToLocalChecked());
         break;
    }
  }

  return resultObject;
}
