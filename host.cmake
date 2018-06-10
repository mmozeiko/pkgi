add_executable(pkgj_cli
  src/pkgi_db.cpp
  src/pkgi_download.cpp
  src/pkgi_simulator.cpp
  src/pkgi_aes128.c
  src/pkgi_sha256.c
  src/pkgi_filehttp.cpp
  src/pkgi_zrif.c
  src/puff.c
  src/pkgi_cli.cpp
)

target_link_libraries(pkgj_cli
  CONAN_PKG::fmt
  CONAN_PKG::boost_scope_exit
  CONAN_PKG::boost_algorithm
  CONAN_PKG::sqlite3
)
