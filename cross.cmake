set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

function(add_assets target)
  set(result)
  foreach(in_f ${ARGN})
    set(out_f "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${in_f}.o")
    get_filename_component(out_dir ${out_f} DIRECTORY)
    add_custom_command(OUTPUT ${out_f}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
      COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Using ${in_f}"
      VERBATIM
    )
    list(APPEND result ${out_f})
  endforeach()
  set(${target} "${result}" PARENT_SCOPE)
endfunction()

add_assets(assets assets/background.png)

add_executable(pkgj
  ${assets}
  pkgi.cpp
  pkgi_aes128.c
  pkgi_config.cpp
  pkgi_db.cpp
  pkgi_dialog.c
  pkgi_download.cpp
  pkgi_downloader.cpp
  pkgi_vitahttp.cpp
  pkgi_menu.cpp
  pkgi_sha256.c
  pkgi_vita.cpp
  pkgi_zrif.c
  puff.c
)

add_dependencies(pkgj Boost fmtproject)

target_link_libraries(pkgj
  vita2d
  fmt
  png
  z
  m
  SceAppMgr_stub
  SceAppUtil_stub
  SceCommonDialog_stub
  SceCtrl_stub
  SceDisplay_stub
  SceGxm_stub
  SceHttp_stub
  SceNet_stub
  SceNetCtl_stub
  ScePgf_stub
  ScePower_stub
  ScePromoterUtil_stub
  SceShellSvc_stub
  SceSsl_stub
  SceSysmodule_stub
)

vita_create_self(eboot.bin pkgj UNSAFE)

configure_file(
   sce_sys/livearea/contents/template.xml.in
   sce_sys/livearea/contents/template.xml
)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION 0${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
       sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
       sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
       ${CMAKE_CURRENT_BINARY_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

add_custom_target(send
  COMMAND curl -T eboot.bin ftp://"$ENV{PSVITAIP}":1337/ux0:/app/${VITA_TITLEID}/
  DEPENDS eboot.bin
)

add_custom_target(copy
  COMMAND cp eboot.bin ${PSVITADRIVE}/app/${VITA_TITLEID}/eboot.bin
  DEPENDS eboot.bin
)
