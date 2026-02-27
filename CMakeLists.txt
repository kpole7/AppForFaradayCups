cmake_minimum_required(VERSION 3.20)

project(AppForFaradayCups LANGUAGES CXX)

# Projekt jest Linux-only
if(NOT UNIX OR APPLE)
  message(FATAL_ERROR "This project is intended to be built on Linux (e.g., Linux Mint) only.")
endif()

add_executable(appForFaradayCups
  source/main.cpp
  source/peripheral_thread.cpp
  source/shared_data.cpp
  source/modbus_rtu_master.cpp
  source/gui_widgets.cpp
  source/settings_file.cpp
)

# Odpowiedniki: -g -rdynamic -Wall -Wextra -Wpedantic
target_compile_options(appForFaradayCups PRIVATE
  -g
  -Wall
  -Wextra
  -Wpedantic
)

# -rdynamic jest linker option
target_link_options(appForFaradayCups PRIVATE
  -g
  -rdynamic
)

# Odpowiedniki: -Iinclude -I/usr/include/modbus
target_include_directories(appForFaradayCups PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  /usr/include/modbus
)

# Odpowiedniki: -lfltk -lX11 -lpthread -lmodbus -lfltk_images -lpng -lz
# (Linux-only: zostawiamy jako "plain" link libs jak w Makefile)
target_link_libraries(appForFaradayCups PRIVATE
  fltk
  X11
  pthread
  modbus
  fltk_images
  png
  z
)

# ------------------------------------------------------------------------------
# Kopiowanie plików do katalogu build (jak cp ... build/)
# CMake ma różne katalogi wyjściowe; ustawiamy runtime do build/ żeby było podobnie.
# ------------------------------------------------------------------------------
set_target_properties(appForFaradayCups PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_custom_command(TARGET appForFaradayCups POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/PomiarWiązki.cfg"
          "$<TARGET_FILE_DIR:appForFaradayCups>/PomiarWiązki.cfg"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/doc/Pomiar_Wiązki.pdf"
          "$<TARGET_FILE_DIR:appForFaradayCups>/Pomiar_Wiązki.pdf"
  COMMENT "Copying PomiarWiązki.cfg and doc/*.pdf to build output directory"
)

# ------------------------------------------------------------------------------
# Sanitizery: odpowiedniki targetów make asan/ubsan/sanitize + run-*
# ------------------------------------------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_ASAN OR ENABLE_UBSAN)
  target_compile_options(appForFaradayCups PRIVATE -O1 -fno-omit-frame-pointer)
  target_link_options(appForFaradayCups PRIVATE -O1 -fno-omit-frame-pointer)
endif()

if(ENABLE_ASAN)
  target_compile_options(appForFaradayCups PRIVATE -fsanitize=address)
  target_link_options(appForFaradayCups PRIVATE -fsanitize=address)
endif()

if(ENABLE_UBSAN)
  target_compile_options(appForFaradayCups PRIVATE -fsanitize=undefined)
  target_link_options(appForFaradayCups PRIVATE -fsanitize=undefined)
endif()

add_custom_target(run
  COMMAND "$<TARGET_FILE:appForFaradayCups>"
  DEPENDS appForFaradayCups
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:appForFaradayCups>"
)

add_custom_target(run-asan
  COMMAND ${CMAKE_COMMAND} -E env "ASAN_OPTIONS=detect_leaks=1:halt_on_error=0"
          "$<TARGET_FILE:appForFaradayCups>"
  DEPENDS appForFaradayCups
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:appForFaradayCups>"
)

add_custom_target(run-ubsan
  COMMAND ${CMAKE_COMMAND} -E env "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
          "$<TARGET_FILE:appForFaradayCups>"
  DEPENDS appForFaradayCups
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:appForFaradayCups>"
)

add_custom_target(run-sanitize
  COMMAND ${CMAKE_COMMAND} -E env
          "ASAN_OPTIONS=detect_leaks=1:halt_on_error=0"
          "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
          "$<TARGET_FILE:appForFaradayCups>"
  DEPENDS appForFaradayCups
  WORKING_DIRECTORY "$<TARGET_FILE_DIR:appForFaradayCups>"
)

