# Урок 5: игра «Жизнь» — двумерное поле в одномерной памяти, соседи, два буфера, изменение размера.

if (NOT TARGET raylib)
  find_package(raylib 5.0 QUIET)
  if (NOT raylib_FOUND)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
      URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

set(LIFE_SOURCES
  src/main.c
  src/engine.c
  src/scene_cells.c
  src/scene_neighbors.c
  src/scene_step.c
  src/scene_resize.c
  src/scene_gallery.c
  src/engine.h
  src/life.h
  src/scenes.h
  ${CMAKE_SOURCE_DIR}/Common/viz/viz.c
  ${CMAKE_SOURCE_DIR}/Common/viz/viz.h
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.c
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memview.c)

# LifeLab — версия Дины с багами в tickets.c, LifeLab_Teacher — с исправленным tickets_fixed.c.
add_executable(LifeLab ${LIFE_SOURCES} src/tickets.c)
add_executable(LifeLab_Teacher ${LIFE_SOURCES} src/tickets_fixed.c)

foreach(target LifeLab LifeLab_Teacher)
  set_property(TARGET ${target} PROPERTY C_STANDARD 11)
  target_link_libraries(${target} PRIVATE raylib)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/Common/viz ${CMAKE_SOURCE_DIR}/Common/memtrack src)
  if (MSVC)
    target_compile_options(${target} PRIVATE /utf-8)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
endforeach()

# malloc/free в заданиях незаметно идут через счётчик памяти — так видно утечку в задании 4.
foreach(file src/tickets.c src/tickets_fixed.c)
  if (MSVC)
    set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "/FI${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack_redirect.h")
  else()
    set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "-include;${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack_redirect.h")
  endif()
endforeach()

# AddressSanitizer — пресет "x64 Debug + ASan", как в уроках 2 и 4.
if (AQUARIUM_ASAN AND MSVC)
  string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  get_filename_component(_msvc_bin "${CMAKE_C_COMPILER}" DIRECTORY)
  foreach(target LifeLab LifeLab_Teacher)
    set_property(TARGET ${target} PROPERTY MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
    target_compile_options(${target} PRIVATE /fsanitize=address)
    target_link_options(${target} PRIVATE /INCREMENTAL:NO)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${_msvc_bin}/clang_rt.asan_dynamic-x86_64.dll" "$<TARGET_FILE_DIR:${target}>")
  endforeach()
endif()
