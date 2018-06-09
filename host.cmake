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

target_link_libraries(pkgj_cli
  CONAN_PKG::fmt
  CONAN_PKG::boost_scope_exit
  CONAN_PKG::boost_algorithm
  CONAN_PKG::sqlite3
)
