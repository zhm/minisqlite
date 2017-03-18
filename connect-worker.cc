#include "connect-worker.h"

ConnectWorker::ConnectWorker(Nan::Callback *callback, Client *client, std::string connectionString, int flags, std::string vfs)
    : AsyncWorker(callback),
      client_(client),
      connectionString_(connectionString),
      flags_(flags),
      vfs_(vfs)
{}

ConnectWorker::~ConnectWorker()
{}

void ConnectWorker::Execute() {
  const char *vfs = vfs_.length() ? vfs_.c_str() : nullptr;

  int result = sqlite3_open_v2(connectionString_.c_str(), &client_->connection_, flags_, vfs);

  client_->SetLastError(result);

  if (result != SQLITE_OK) {
    client_->Close();
    SetErrorMessage(client_->lastErrorMessage_.c_str());
    return;
  }
}
