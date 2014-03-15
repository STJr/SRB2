project(win_iconv)

cmake_minimum_required(VERSION 2.6)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})

set(CPACK_GENERATOR "TBZ2")
include(CPack)
option(BUILD_TEST "build test executable" OFF)
option(BUILD_STATIC "build the static lib" OFF)
if(BUILD_TEST)
    enable_testing()
endif(BUILD_TEST)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

if(NOT WINCE)
    add_definitions(-DUSE_LIBICONV_DLL)
    if(DEFAULT_LIBICONV_DLL)
        add_definitions(-DDEFAULT_LIBICONV_DLL=${DEFAULT_LIBICONV_DLL})
    endif(DEFAULT_LIBICONV_DLL)
else(NOT WINCE)
    find_package(Wcecompat REQUIRED)
    include_directories(${WCECOMPAT_INCLUDE_DIR})
endif(NOT WINCE)

add_library(iconv SHARED win_iconv.c iconv.def)
set_target_properties(iconv PROPERTIES COMPILE_FLAGS "-DMAKE_DLL")

add_executable(win_iconv win_iconv.c)
set_target_properties(win_iconv PROPERTIES COMPILE_FLAGS "-DMAKE_EXE")

if(WINCE)
    target_link_libraries(iconv ${WCECOMPAT_LIBRARIES})
    target_link_libraries(win_iconv ${WCECOMPAT_LIBRARIES})
endif(WINCE)

install(TARGETS iconv win_iconv RUNTIME DESTINATION bin
                                LIBRARY DESTINATION lib
                                ARCHIVE DESTINATION lib)

if(BUILD_STATIC)
    add_library(iconv-static STATIC win_iconv.c)
    set_target_properties(iconv-static PROPERTIES OUTPUT_NAME "iconv")
    if(WINCE)
        target_link_libraries(iconv-static ${WCECOMPAT_LIBRARIES})
    endif(WINCE)
    install(TARGETS iconv-static RUNTIME DESTINATION bin
                                 LIBRARY DESTINATION lib
                                 ARCHIVE DESTINATION lib)
endif(BUILD_STATIC)


install(FILES iconv.h DESTINATION include)

if(BUILD_TEST)
    # tests:
    add_executable(win_iconv_test win_iconv_test.c)
    add_test(win_iconv_test win_iconv_test)
    if(WINCE)
        target_link_libraries(win_iconv_test ${WCECOMPAT_LIBRARIES})
    endif(WINCE)
endif(BUILD_TEST)
