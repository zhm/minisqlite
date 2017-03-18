'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Client = undefined;
exports.createPool = createPool;

var _assert = require('assert');

var _assert2 = _interopRequireDefault(_assert);

var _cursor = require('./cursor');

var _cursor2 = _interopRequireDefault(_cursor);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

const NativeClient = require('bindings')('addon').Client;
const genericPool = require('generic-pool');

let nextClientID = 0;

class Client {
  constructor() {
    this.nativeClient = new NativeClient();
    this.id = ++nextClientID;
  }

  connect(string, flags, vfs, callback) {
    this.nativeClient.connect(string, flags, vfs, err => {
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

    return new _cursor2.default(this);
  }

  getResults(returnMetadata, callback) {
    Client.setImmediate(() => {
      callback(this.nativeClient.getResults(returnMetadata));
    });
  }

  close() {
    return this.nativeClient.close();
  }

  get lastInsertID() {
    return this.nativeClient.lastInsertID();
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

  createFunction(name, argc, encoding, func, step, final) {
    encoding = encoding || 1; // SQLITE_UTF8
    argc = argc || -1;

    if (func) {
      step = null;
      final = null;
    } else if (typeof step === 'function') {
      final = typeof final === 'function' ? final : o => o.result;
      func = null;
    }

    return this.nativeClient.createFunction(name, argc, encoding, func, step, final);
  }

  createScalarFunction(name, func) {
    return this.createFunction(name, -1, 1, func, null, null);
  }

  createAggregateFunction(name, initialValue, step, final) {
    const aggregate = (args, context) => {
      if (!context.initialized) {
        context.initialized = true;
        context.result = initialValue;
      }

      return step(args, context);
    };

    return this.createFunction(name, -1, 1, null, aggregate, final);
  }
}

exports.Client = Client;
Client.setImmediate = setImmediate;

function createPool(options) {
  /* eslint-disable new-cap */
  return genericPool.Pool({
    name: options.name || 'minisqlite',
    create: callback => {
      new Client().connect(options.db, null, null, (err, client) => {
        if (err) {
          return callback(client ? client.lastError : err);
        }

        return callback(null, client);
      });
    },
    destroy: client => {
      client.close();
    },
    max: options.max || 10,
    idleTimeoutMillis: options.idleTimeoutMillis || 30000,
    reapIntervalMillis: options.reapIntervalMillis || 1000,
    log: options.log
  });
  /* eslint-enable new-cap */
}
//# sourceMappingURL=index.js.map