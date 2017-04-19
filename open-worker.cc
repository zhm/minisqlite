#include "open-worker.h"

OpenWorker::OpenWorker(Nan::Callback *callback, Database *database, std::string connectionString, int flags, std::string vfs)
    : AsyncWorker(callback),
      database_(database),
      connectionString_(connectionString),
      flags_(flags),
      vfs_(vfs)
{}

OpenWorker::~OpenWorker()
{}

void OpenWorker::Execute() {
  const char *vfs = vfs_.length() ? vfs_.c_str() : nullptr;

  int result = sqlite3_open_v2(connectionString_.c_str(), &database_->db_, flags_, vfs);

  database_->SetLastError(result);

  if (result != SQLITE_OK) {
    database_->Close();
    SetErrorMessage(database_->lastErrorMessage_.c_str());
    return;
  }
}
