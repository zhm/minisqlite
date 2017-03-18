import sqlite3 from 'sqlite3';
import fs from 'fs';
import path from 'path';
import rimraf from 'rimraf';

const DATABASE_PATH = 'test2.db';

// rimraf.sync(DATABASE_PATH);

let db = null;

const createDatabase = () => {
  return new Promise((resolve, reject) => {
    db = new sqlite3.Database(DATABASE_PATH, (err) => {
      if (err) {
        return reject(err);
      }
      return resolve();
    });
  });
};

const execSQL = (client, command, callback) => {
  return new Promise((resolve, reject) => {
    const statement = db.prepare(command);

    statement.run();

    statement.finalize((err) => {
      if (err) {
        return reject(err);
      }
      return resolve();
    });
  });
};

const ITERATIONS = 1000000;

async function runTest() {
  const client = await createDatabase();

  const setup = 'CREATE TABLE test_table (t1 TEXT, t2 INTEGER);';
  await execSQL(client, setup);

  const insert = "INSERT INTO test_table (t1, t2) SELECT 'test',";

  await execSQL(client, 'BEGIN');
  for (let i = 0; i < ITERATIONS; ++i) {
    await execSQL(client, insert + i);
  }
  await execSQL(client, 'COMMIT');

  db.close();
}

async function runReadTest() {
  const now = new Date().getTime();

  await createDatabase();

  // await runTest();

  const select = "SELECT * FROM test_table limit 1000000";

  db.each(select, (err, row) => {
    // console.log(row);
  }, () => {
    console.log('TIME', new Date().getTime() - now);
    db.close();
  });

}

runReadTest().then(() => {
  console.log('done now');
}).catch((err) => {
  console.error('error', err);
});

// runTest().then(() => {
//   console.log('done now');
// }).catch((err) => {
//   console.error('error', err);
// });

