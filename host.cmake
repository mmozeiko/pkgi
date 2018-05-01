add_executable(extract
  pkgi_download.cpp
  pkgi_simulator.cpp
  pkgi_aes128.c
  pkgi_sha256.c
  pkgi_filehttp.cpp
  pkgi_zrif.c
  puff.c
  extract.cpp
)

add_dependencies(extract Boost fmtproject)

target_link_libraries(extract
  fmt
)
