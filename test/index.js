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

    client.query(command).each((err, {finished, columns, values, index}) => {
      /* eslint-disable callback-return */
      callback(err, {finished, columns, values, index, client});
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

    execSQL(db, sql, (err, {finished, columns, values, index, client}) => {
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
        assert.equal(client.lastInsertID, 3);
        done();
      }
    });
  });

  it('should return errors', (done) => {
    execSQL(db, 'sele', (err, {finished, columns, values, index, client}) => {
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

    execSQL(db, 'SELECT 1 AS count WHERE 1 = 0', (err, {finished, columns, values, index, client}) => {
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

  it('should allow definition of custom scalar functions', (done) => {
    pool.acquire((err, client) => {
      if (err) {
        throw err;
      }

      client.createScalarFunction('TESTFUNC', (args) => {
        return args[0] + 1;
      });

      let lastValues = null;

      client.query('SELECT TESTFUNC(1337)').each((err, {finished, columns, values, index}) => {
        if (values) {
          lastValues = values;
        }

        if (finished) {
          pool.release(client);
          assert.equal(lastValues[0], 1338);
          done();
        }
      });
    });
  });

  it('should allow definition of custom aggregate functions', (done) => {
    pool.acquire((err, client) => {
      if (err) {
        throw err;
      }

      client.createAggregateFunction('TEXTCONCAT', '', (args, context) => {
        context.result = context.result + args[0];
      });

      client.createAggregateFunction('SUMTEST', '', (args, context) => {
        context.result = context.result + args[0] + args[1];
      });

      let lastValues = null;

      client.query('SELECT SUMTEST(t2, t2), TEXTCONCAT(t2) FROM test_table').each((err, {finished, columns, values, index}) => {
        if (values) {
          lastValues = values;
        }

        if (finished) {
          pool.release(client);
          assert.equal(lastValues[0], '112233');
          assert.equal(lastValues[1], '123');
          done();
        }
      });
    });
  });
});
