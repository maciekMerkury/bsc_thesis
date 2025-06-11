cmake_minimum_required(VERSION 3.13)
project(demi_epoll_project C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior sanitizers" ON)

add_compile_options(-Wall -Werror -Wno-trigraphs)

if (ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif ()

add_library(demi_epoll SHARED)

target_compile_options(demi_epoll
        PRIVATE
        $<$<CONFIG:Release>:-Os>
        $<$<CONFIG:Debug>:-O0 -ggdb>
)

FILE(GLOB DEMI_EPOLL_SOURCES lib/src/*.c)
target_sources(demi_epoll
        PRIVATE
        ${DEMI_EPOLL_SOURCES}
)
target_include_directories(demi_epoll
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)
target_link_libraries(demi_epoll PUBLIC demikernel)

install(TARGETS demi_epoll
        LIBRARY DESTINATION lib
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/include/
        DESTINATION include/demi_epoll
)

add_executable(demi_epoll_tests)
FILE(GLOB TEST_EXE_SOURCES test_exe/*.c)
target_sources(demi_epoll_tests
        PRIVATE
        ${TEST_EXE_SOURCES}
)
target_link_libraries(demi_epoll_tests PRIVATE demi_epoll)
