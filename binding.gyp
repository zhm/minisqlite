{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "addon.cc",
                   "database.cc",
                   "statement.cc",
                   "open-worker.cc",
                   "deps/sqlite3.c",
                   "deps/gpkg/binstream.c",
                   "deps/gpkg/blobio.c",
                   "deps/gpkg/error.c",
                   "deps/gpkg/fp.c",
                   "deps/gpkg/geomio.c",
                   "deps/gpkg/gpkg.c",
                   "deps/gpkg/gpkg_db.c",
                   "deps/gpkg/gpkg_geom.c",
                   "deps/gpkg/i18n.c",
                   "deps/gpkg/sql.c",
                   "deps/gpkg/spatialdb.c",
                   "deps/gpkg/spl_db.c",
                   "deps/gpkg/spl_geom.c",
                   "deps/gpkg/strbuf.c",
                   "deps/gpkg/wkb.c",
                   "deps/gpkg/wkt.c" ],
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
