import { createPool } from '../src';
import fs from 'fs';
import path from 'path';
import rimraf from 'rimraf';

const db = 'bench/test1.db';
const sql = fs.readFileSync(path.join(__dirname, '../test/test.sql')).toString();

const pool = createPool({db: db});

const execSQL = (client, command, callback) => {
  return new Promise((resolve, reject) => {
    client.query(command).each((err, {finished, columns, values, index}) => {
      if (err) {
        return reject(err);
      }

      /* eslint-disable callback-return */
      if (callback) {
        callback(err, {finished, columns, values, index});
      }
      /* eslint-enable callback-return */

      if (finished) {
        return resolve();
      }
    });
  });
};

const acquire = () => {
  return new Promise((resolve, reject) => {
    pool.acquire((err, client) => {
      if (err) {
        return reject(err);
      }

      resolve(client);
    });
  });
};

const ITERATIONS = 1000000;

async function setupDatabase() {
  rimraf.sync(db);

  const client = await acquire();

  const setup = 'CREATE TABLE test_table (t1 TEXT, t2 INTEGER);';

  await execSQL(client, setup);

  const insert = "INSERT INTO test_table (t1, t2) SELECT 'test',";

  await execSQL(client, 'BEGIN');

  for (let i = 0; i < ITERATIONS; ++i) {
    await execSQL(client, insert + i);
  }

  await execSQL(client, 'COMMIT');

  pool.release(client);
}

async function readAllRows() {
  const now = new Date().getTime();

  const client = await acquire();

  const select = "SELECT * FROM test_table limit 1000000";

  await execSQL(client, select, (err, finished, columns, values, index) => {
    // console.log(values);
  });

  console.log('TIME', new Date().getTime() - now);

  pool.release(client);
}

async function customFunction() {
  const SUMCUSTOM = (args, context) => {
    context.result = (context.result || 0) + args[0];
  };

  const SUMCUSTOMFINAL = (context) => {
    return context.result;
  };

  const client = await acquire();

  client.createFunction('SUMCUSTOM', null, null, null, SUMCUSTOM, SUMCUSTOMFINAL);

  const now = new Date().getTime();

  const select = 'SELECT SUMCUSTOM(t2) FROM test_table';

  await execSQL(client, select, (err, {finished, columns, values, index}) => {
  });

  console.log('CUSTOMTIME', new Date().getTime() - now);

  pool.release(client);
}

customFunction().then(() => {
  console.log('done');
  pool.drain(() => {
    pool.destroyAllNow();
  });
}).catch((err) => {
  console.error('error', err);
});

// runTest().then(() => {
//   console.log('done');
//   pool.drain(() => {
//     pool.destroyAllNow();
//   });
// }).catch((err) => {
//   console.error('error', err);
// });
