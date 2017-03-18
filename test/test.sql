CREATE TABLE test_table (t1 TEXT, t2 REAL);

INSERT INTO test_table (t1, t2)
SELECT 'test1', 1;

INSERT INTO test_table (t1, t2)
SELECT 'test2', 2;

INSERT INTO test_table (t1, t2)
SELECT 'test3', 3;

SELECT t1, t2 FROM test_table;
