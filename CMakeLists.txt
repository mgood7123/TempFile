cmake_minimum_required(VERSION 3.4.3)
project(tmpfile)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(tmpfile src/tmpfile.cpp src/randombytes.c)
target_include_directories(tmpfile PUBLIC include)

add_executable(tmpfile_test example.cpp)
target_link_libraries(tmpfile_test tmpfile)

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Installation directory for headers")

install(TARGETS tmpfile tmpfile_test
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/tmpfile/tmpfile.h DESTINATION "${INSTALL_INC_DIR}/tmpfile")
