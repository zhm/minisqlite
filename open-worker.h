#ifndef __OPEN_WORKER_H__
#define __OPEN_WORKER_H__

#include <nan.h>
#include "database.h"

class OpenWorker : public Nan::AsyncWorker {
 public:
  OpenWorker(Nan::Callback *callback, Database *database, std::string connectionString, int flags, std::string vfs);

  virtual ~OpenWorker();

  void Execute() override;

 private:
  Database *database_;

  std::string connectionString_;
  int flags_;
  std::string vfs_;
};

#endif
