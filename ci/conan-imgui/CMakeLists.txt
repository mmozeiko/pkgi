cmake_minimum_required(VERSION 2.8.11)
project(imgui CXX)

include(conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_VERBOSE_MAKEFILE TRUE)
if (WIN32 AND MSVC AND BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif(WIN32 AND MSVC AND BUILD_SHARED_LIBS)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source_subfolder)
set(EXTRA_FONTS_DIR ${SOURCE_DIR}/misc/fonts)

set(SOURCE_FILES ${SOURCE_DIR}/imgui_draw.cpp
                 ${SOURCE_DIR}/imgui.cpp)
set(HEADER_FILES ${SOURCE_DIR}/imconfig.h
                 ${SOURCE_DIR}/imgui_internal.h
                 ${SOURCE_DIR}/imgui.h
                 ${SOURCE_DIR}/stb_rect_pack.h
                 ${SOURCE_DIR}/stb_textedit.h
                 ${SOURCE_DIR}/stb_truetype.h)
set(EXTRA_FONTS_FILES ${EXTRA_FONTS_DIR}/Karla-Regular.ttf
                      ${EXTRA_FONTS_DIR}/ProggyTiny.ttf
                      ${EXTRA_FONTS_DIR}/ProggyClean.ttf
                      ${EXTRA_FONTS_DIR}/DroidSans.ttf
                      ${EXTRA_FONTS_DIR}/Roboto-Medium.ttf
                      ${EXTRA_FONTS_DIR}/Cousine-Regular.ttf)
set(BINARY_TO_COMPRESSED_BIN binary_to_compressed_c)

add_executable(${BINARY_TO_COMPRESSED_BIN} ${EXTRA_FONTS_DIR}/binary_to_compressed_c.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CONAN_LIBS})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS ${BINARY_TO_COMPRESSED_BIN}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES ${HEADER_FILES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
install(FILES ${EXTRA_FONTS_FILES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/misc/fonts
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
