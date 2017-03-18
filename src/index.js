const NativeClient = require('bindings')('addon').Client;
const genericPool = require('generic-pool');

import Cursor from './cursor';

let nextClientID = 0;

export class Client {
  constructor() {
    this.nativeClient = new NativeClient();
    this.id = ++nextClientID;
  }

  connect(string, flags, vfs, callback) {
    this.nativeClient.connect(string, flags, vfs, (err) => {
      if (err) {
        return callback(err, this);
      }

      return callback(null, this);
    });
  }

  query(sql) {
    if (!this.nativeClient.finished()) {
      throw new Error('client in use', this.id);
    }

    this.nativeClient.query(sql);

    return new Cursor(this);
  }

  getResults(returnMetadata, callback) {
    Client.setImmediate(() => {
      callback(this.nativeClient.getResults(returnMetadata));
    });
  }

  close() {
    return this.nativeClient.close();
  }

  get lastError() {
    const error = this.nativeClient.lastError();

    if (error == null) {
      return null;
    }

    const queryError = new Error();

    for (const prop in error) {
      if (error.hasOwnProperty(prop)) {
        queryError[prop] = error[prop];
      }
    }

    return queryError;
  }
}

Client.setImmediate = setImmediate;

export function createPool(options) {
  /* eslint-disable new-cap */
  return genericPool.Pool({
    name: options.name || 'minisqlite',
    create: (callback) => {
      new Client().connect(options.db, null, null, (err, client) => {
        if (err) {
          return callback(client ? client.lastError : err);
        }

        return callback(null, client);
      });
    },
    destroy: (client) => {
      client.close();
    },
    max: options.max || 10,
    idleTimeoutMillis: options.idleTimeoutMillis || 30000,
    reapIntervalMillis: options.reapIntervalMillis || 1000,
    log: options.log
  });
  /* eslint-enable new-cap */
}
