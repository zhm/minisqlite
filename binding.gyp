{
  "targets": [
    {
      "target_name": "minisqlite",
      "sources": [ "addon.cc",
                   "database.cc",
                   "statement.cc",
                   "open-worker.cc",
                   "deps/sqlite3.c" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "./deps",
			],
      "defines": [
        'SQLITE_THREADSAFE=1',
        'SQLITE_ENABLE_FTS3',
        'SQLITE_ENABLE_FTS3_PARENTHESIS',
        'SQLITE_ENABLE_FTS4',
        'SQLITE_ENABLE_FTS5',
        'SQLITE_ENABLE_JSON1',
        'SQLITE_ENABLE_RTREE',
        'SQLITE_ENABLE_SESSION',
        'SQLITE_ENABLE_COLUMN_METADATA',
        'SQLITE_ENABLE_NULL_TRIM',
        'SQLITE_ENABLE_PREUPDATE_HOOK',
        'SQLITE_ENABLE_STAT4',
        'SQLITE_ENABLE_LOAD_EXTENSION=1',
        'GPKG_HAVE_CONFIG_H'
      ],
      'conditions': [
      ]
    }
  ],
}
