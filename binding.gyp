{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "addon.cc",
                   "client.cc",
                   "connect-worker.cc",
                   "deps/sqlite3.c" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "./deps",
			],
      "defines": [
        'SQLITE_THREADSAFE=1',
        'SQLITE_ENABLE_FTS3',
        'SQLITE_ENABLE_FTS4',
        'SQLITE_ENABLE_FTS5',
        'SQLITE_ENABLE_JSON1',
        'SQLITE_ENABLE_RTREE',
        'SQLITE_ENABLE_COLUMN_METADATA',
        'SQLITE_ENABLE_NULL_TRIM',
        'SQLITE_ENABLE_PREUPDATE_HOOK',
        'SQLITE_ENABLE_STAT4',
        'SQLITE_API=__attribute__ ((visibility ("hidden")))' # hide the sqlite symbols
      ],
      'conditions': [
      ]
    }
  ],
}
