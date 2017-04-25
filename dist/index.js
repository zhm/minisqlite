'use strict';

Object.defineProperty(exports, "__esModule", {
  value: true
});
exports.Statement = exports.Database = undefined;

var _assert = require('assert');

var _assert2 = _interopRequireDefault(_assert);

var _cursor = require('./cursor');

var _cursor2 = _interopRequireDefault(_cursor);

function _interopRequireDefault(obj) { return obj && obj.__esModule ? obj : { default: obj }; }

const NativeDatabase = require('bindings')('addon').Database;
const NativeStatement = require('bindings')('addon').Statement;

let nextObjectID = 0;

class Database {
  constructor() {
    this._native = new NativeDatabase();
    this.id = ++nextObjectID;
  }

  open(string, flags, vfs, callback) {
    this._native.open(string, flags, vfs, err => {
      if (err) {
        return callback(err, this);
      }

      return callback(null, this);
    });
  }

  query(sql) {
    const statement = new Statement({ database: this });
    return statement.query(sql);
  }

  close() {
    return this._native.close();
  }

  get lastInsertID() {
    return this._native.lastInsertID();
  }

  get lastError() {
    const error = this._native.lastError();

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

    return this._native.createFunction(name, argc, encoding, func, step, final);
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

exports.Database = Database;
class Statement {
  constructor(_ref) {
    let database = _ref.database;

    this._native = new NativeStatement();
    this._database = database;
    this.id = ++nextObjectID;
  }

  query(sql) {
    (0, _assert2.default)(this._database instanceof Database, 'invalid database argument');
    (0, _assert2.default)(this._database._native, 'invalid database handle');

    if (!this._native.finished()) {
      throw new Error('client in use, last statement: ' + this._sql);
    }

    if (sql == null) {
      sql = '';
    }

    sql = sql.replace(/\0/g, '');

    this._sql = sql;

    this._native.query(this._database._native, sql);

    return new _cursor2.default(this);
  }

  getResults(returnMetadata, callback) {
    Statement.setImmediate(() => {
      const results = this._native.getResults(returnMetadata);

      callback(results);
    });
  }

  close() {
    return this._native.close();
  }
}

exports.Statement = Statement;
Statement.setImmediate = setImmediate;
//# sourceMappingURL=index.js.map