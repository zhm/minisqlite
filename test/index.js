import { Database, Statement } from '../src';

import assert from 'assert';
import fs from 'fs';
import path from 'path';
import rimraf from 'rimraf';

const databaseFileName = 'test.db';
const sql = fs.readFileSync(path.join(__dirname, 'test.sql')).toString();

rimraf.sync(databaseFileName);

let db = null;

const execSQL = (database, command, callback) => {
  // if (err) {
  //   throw err;
  // }

  db.query(command).each((err, {finished, columns, values, index, statement, next}) => {
    /* eslint-disable callback-return */
    callback(err, {finished, columns, values, index, statement, next});

    next();
    /* eslint-enable callback-return */

    // if (finished) {
    //   pool.release(client);
    // }
  });
};

describe('minisqlite', () => {
  beforeEach((done) => {
    db = new Database();
    db.open(databaseFileName, null, null, done);
  });

  it('should query the database', (done) => {
    let lastIndex = 0;
    let lastColumns = null;
    let lastValues = null;

    execSQL(db, sql, (err, {finished, columns, values, index, statement}) => {
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
        assert.equal(db.lastInsertID, 3);
        done();
      }
    });
  });

  it('should return errors', (done) => {
    execSQL(db, 'sele', (err, {finished, columns, values, index, statement}) => {
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

  it('should return errors for constraint violations', (done) => {
    const constraintSQL = `
      CREATE UNIQUE INDEX idx_t1 ON test_table (t1);

      INSERT INTO test_table (t1, t2) VALUES ('testvalue', 4);
      INSERT INTO test_table (t1, t2) VALUES ('testvalue', 4);
    `;

    execSQL(db, constraintSQL, (err, {finished, columns, values, index, statement}) => {
      if (finished) {
        assert.equal(columns, null);
        assert.equal(values, null);
        assert.equal(index, 1);
        assert.equal(err.message, 'UNIQUE constraint failed: test_table.t1');
        done();
      }
    });
  });

  it('should work properly for empty result sets', (done) => {
    let lastColumns = null;

    execSQL(db, 'SELECT 1 AS count WHERE 1 = 0', (err, {finished, columns, values, index, statement}) => {
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
    db.createScalarFunction('TESTFUNC', (args) => {
      return args[0] + 1;
    });

    let lastValues = null;

    db.query('SELECT TESTFUNC(1337)').each((err, {finished, columns, values, index, next}) => {
      if (values) {
        lastValues = values;
      }

      if (finished) {
        assert.equal(lastValues[0], 1338);
        done();
      }

      next();
    });
  });

  it('should allow definition of custom aggregate functions', (done) => {
    db.createAggregateFunction('TEXTCONCAT', '', (args, context) => {
      context.result = context.result + args[0];
    });

    db.createAggregateFunction('SUMTEST', '', (args, context) => {
      context.result = context.result + args[0] + args[1];
    });

    let lastValues = null;

    db.query('SELECT SUMTEST(t2, t2), TEXTCONCAT(t2) FROM test_table').each((err, {finished, columns, values, index, next}) => {
      if (values) {
        lastValues = values;
      }

      if (finished) {
        assert.equal(lastValues[0], '11223344');
        assert.equal(lastValues[1], '1234');
        done();
      }

      next();
    });
  });

  it('should close the database with open statements', (done) => {
    db.query('SELECT * FROM test_table').each((err, {finished, columns, values, index, next}) => {
    });

    db.query('SELECT * FROM test_table').each((err, {finished, columns, values, index, next}) => {
    });

    db.close();

    done();
  });

  it('should initialize GeoPackage', (done) => {
    db.query('SELECT InitSpatialMetadata()').each((err, {finished, columns, values, index, next}) => {
      if (finished && err == null) {
        done();
      }

      next();
    });
  });
});
