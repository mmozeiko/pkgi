cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(pkgi)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "PKGi")
set(VITA_TITLEID  "PKGI00000")
set(VITA_VERSION  "00.04")

option(PKGI_ENABLE_LOGGING "enables debug logging over udp multicast" OFF)

if(PKGI_ENABLE_LOGGING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPKGI_ENABLE_LOGGING")
endif()

if(NOT $ENV{PKGI_REFRESH_URL} STREQUAL "")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPKGI_REFRESH_URL=\\\"$ENV{PKGI_REFRESH_URL}\\\"")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPKGI_VERSION=\\\"${VITA_VERSION}\\\" -D_GNU_SOURCE -g -Wall -Wextra -Werror -fvisibility=hidden")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

add_executable(pkgi
  pkgi.c
  pkgi_aes128.c
  pkgi_config.c
  pkgi_db.c
  pkgi_dialog.c
  pkgi_download.c
  pkgi_menu.c
  pkgi_sha256.c
  pkgi_vita.c
  pkgi_zrif.c
  puff.c
)

target_link_libraries(pkgi
  vita2d
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

vita_create_self(eboot.bin pkgi UNSAFE)
    
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
       sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
       sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
       sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

add_custom_target(send
  COMMAND curl -T eboot.bin ftp://"$ENV{PSVITAIP}":1337/ux0:/app/${VITA_TITLEID}/
  DEPENDS eboot.bin
)

add_custom_target(copy
  COMMAND cp eboot.bin ${PSVITADRIVE}/app/${VITA_TITLEID}/eboot.bin
  DEPENDS eboot.bin
)
