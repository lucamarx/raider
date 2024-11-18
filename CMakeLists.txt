#
# RAIDER
#
cmake_minimum_required(VERSION 3.30)

project(raider C)

add_compile_options(-D_FORTIFY_SOURCE=2 -g3 -O2 -Wall -Wextra -pedantic -std=gnu99)

add_executable(raider
  src/actions.c
  src/btree.c
  src/display.c
  src/event_loop.c
  src/ls.c
  src/preview.c
  src/preview_xwinsize.c
  src/raider.c
  src/utils.c
)

include_directories(include)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})
target_link_libraries(raider ${CURSES_LIBRARIES})

find_package(X11)

if(X11_FOUND)
  add_compile_definitions(HAS_X11)
  include_directories(${X11_INCLUDE_DIR})
  target_link_libraries(raider ${X11_LIBRARIES})
endif()
