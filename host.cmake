add_library(sqlite STATIC ${sqlitepath}/sqlite3.c)
set_source_files_properties(${sqlitepath}/sqlite3.c PROPERTIES GENERATED TRUE)
target_include_directories(sqlite PUBLIC ${sqlitepath})
target_compile_definitions(sqlite PUBLIC SQLITE_THREADSAFE=0 SQLITE_OMIT_LOAD_EXTENSION)
add_dependencies(sqlite sqliteproject)

add_executable(pkgj_cli
  pkgi_db.cpp
  pkgi_download.cpp
  pkgi_simulator.cpp
  pkgi_aes128.c
  pkgi_sha256.c
  pkgi_filehttp.cpp
  pkgi_zrif.c
  puff.c
  pkgi_cli.cpp
)

add_dependencies(pkgj_cli Boost fmtproject sqliteproject)

target_link_libraries(pkgj_cli
  fmt
  sqlite
)
