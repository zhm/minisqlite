#include "statement.h"
#include "database.h"

using v8::FunctionTemplate;

NAN_MODULE_INIT(Init) {
  Database::Init(target);
  Statement::Init(target);
}

NODE_MODULE(minisqlite, Init)
