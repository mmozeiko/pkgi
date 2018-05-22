add_executable(pkgj_cli
  pkgi_download.cpp
  pkgi_simulator.cpp
  pkgi_aes128.c
  pkgi_sha256.c
  pkgi_filehttp.cpp
  pkgi_zrif.c
  puff.c
  pkgi_cli.cpp
)

add_dependencies(pkgj_cli Boost fmtproject)

target_link_libraries(pkgj_cli
  fmt
)
