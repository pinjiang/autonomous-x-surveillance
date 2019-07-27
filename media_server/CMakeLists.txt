project(gst-viewer C)

set(CMAKE_BUILD_TYPE DEBUG)
# DEBUG/MINSIZEREL/RELWITHDEBINFO/RELEASE
set(CMAKE_C_FLAGS "-Werror -Wextra -DG_LOG_DOMAIN=\\\"media_server\\\"")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS}")

# -Wno-unused-parameter -Wno-ignored-qualifiers -Werror=missing-field-initializers"

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/")

find_package(glib-2.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)
pkg_check_modules (LIBSOUP REQUIRED libsoup-2.4>=2.26)

add_executable(gst-viewer "")
target_sources(gst-viewer
    PRIVATE
        main.c
        viewer_soup_server.c
        ${CMAKE_CURRENT_LIST_DIR}/../utils/utils.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ${JSON_GLIB_INCLUDE_DIRS} ${LIBSOUP_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/utils)

target_link_libraries(${PROJECT_NAME}
    gst-rtsp-lib
    glib-2.0
    gobject-2.0
    ${JSON_GLIB_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
)
