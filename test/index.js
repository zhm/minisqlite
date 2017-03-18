import { createPool } from '../src';

import assert from 'assert';
import fs from 'fs';
import path from 'path';
import rimraf from 'rimraf';

const db = 'test.db';
const sql = fs.readFileSync(path.join(__dirname, 'test.sql')).toString();

rimraf.sync(db);

const pool = createPool({db: db});

const execSQL = (database, command, callback) => {
  pool.acquire((err, client) => {
    if (err) {
      throw err;
    }

    console.log('RUNNING', command);

    client.query(command).each((err, finished, columns, values, index) => {
      /* eslint-disable callback-return */
      callback(err, finished, columns, values, index);
      /* eslint-enable callback-return */

      if (finished) {
        pool.release(client);
      }
    });
  });
};

describe('minisqlite', () => {
  it('should query the database', (done) => {
    let lastIndex = 0;
    let lastColumns = null;
    let lastValues = null;

    execSQL(db, sql, (err, finished, columns, values, index) => {
      if (err) {
        throw err;
      }

      if (columns) {
        lastColumns = columns;
      }

      if (values) {
        lastValues = values;
        lastIndex = index;
      }

      if (finished) {
        assert.equal(lastColumns.length, 2);
        assert.equal(lastIndex, 2);
        assert.deepEqual(lastValues, [ 'test3', 3 ]);
        done();
      }
    });
  });

  it('should return errors', (done) => {
    execSQL(db, 'sele', (err, finished, columns, values, index) => {
      if (finished) {
        assert.equal(columns, null);
        assert.equal(values, null);
        assert.equal(index, 0);
        assert.equal(err.message, 'near "sele": syntax error');
        // assert.equal(err.primary, 'syntax error at or near "sele"');
        // assert.equal(err.severity, 'ERROR');
        // assert.equal(err.position, '1');
        done();
      }
    });
  });

  it('should work properly for empty result sets', (done) => {
    let lastColumns = null;

    execSQL(db, 'SELECT 1 AS count WHERE 1 = 0', (err, finished, columns, values, index) => {
      if (err) {
        throw err;
      }

      if (columns) {
        lastColumns = columns;
      }

      if (finished) {
        assert.equal(lastColumns.length, 1);
        assert.equal(values, null);
        assert.equal(index, 0);
        done();
      }
    });
  });
});
