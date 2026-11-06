# Урок 3: «Память на ладони» — указатели, массивы, структуры, списки, static.

# raylib уже может быть подключён уроком 2; иначе ищем или скачиваем сами.
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

# Общие файлы обеих версий; отличаются только student.c и teacher.c.
set(MEMORYLAB_SOURCES
  src/main.c
  ${CMAKE_SOURCE_DIR}/Common/viz/viz.c
  src/scene_pointers.c
  src/scene_arrays.c
  src/scene_structs.c
  src/scene_dynarray.c
  src/scene_list.c
  src/scene_static.c
  src/scene_tree.c
  src/types.h
  ${CMAKE_SOURCE_DIR}/Common/viz/viz.h
  src/scenes.h
  src/student.h)

# MemoryLab — версия для Дины (её student.c), MemoryLab_Teacher — с готовыми решениями.
add_executable(MemoryLab ${MEMORYLAB_SOURCES} src/student.c)
add_executable(MemoryLab_Teacher ${MEMORYLAB_SOURCES} src/teacher.c)

foreach(target MemoryLab MemoryLab_Teacher)
  set_property(TARGET ${target} PROPERTY C_STANDARD 11)
  target_link_libraries(${target} PRIVATE raylib)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/Common/viz)

  # исходники в UTF-8 — иначе кириллица в строках превратится в кракозябры
  if (MSVC)
    target_compile_options(${target} PRIVATE /utf-8)
  endif()
endforeach()
