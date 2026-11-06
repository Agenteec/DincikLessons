# Урок 4: «Физическая песочница» — чужая библиотека на C (Chipmunk2D): void*, указатели на функции, владение памятью.

# raylib уже подключён уроками 2–3; иначе ищем или скачиваем сами.
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

# Chipmunk2D 7.0.3. Его собственный CMake устарел (демки, pthread), поэтому только скачиваем исходники
# и собираем библиотеку сами.
include(FetchContent)
FetchContent_Declare(chipmunk
  URL https://github.com/slembcke/Chipmunk2D/archive/refs/tags/Chipmunk-7.0.3.tar.gz
  SOURCE_SUBDIR do-not-add-subdirectory)
FetchContent_MakeAvailable(chipmunk)

file(GLOB CHIPMUNK_SOURCES ${chipmunk_SOURCE_DIR}/src/*.c)
list(FILTER CHIPMUNK_SOURCES EXCLUDE REGEX "cpHastySpace\\.c$")   # многопоточная версия нам не нужна

# Предупреждения чужой библиотеки нам не интересны: уровень задаём каждой цели сам.
string(REPLACE "/W3" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

add_library(chipmunk_lesson STATIC ${CHIPMUNK_SOURCES})
target_include_directories(chipmunk_lesson PUBLIC ${chipmunk_SOURCE_DIR}/include)
set_property(TARGET chipmunk_lesson PROPERTY C_STANDARD 11)
# Вся память Chipmunk идёт через наш счётчик (Common/memtrack/memtrack.c) — так сцены видят каждый calloc/free.
target_compile_definitions(chipmunk_lesson PRIVATE cpcalloc=mt_calloc cprealloc=mt_realloc cpfree=mt_free)
if (MSVC)
  target_compile_options(chipmunk_lesson PRIVATE /W0 /FI${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.h)
else()
  target_compile_options(chipmunk_lesson PRIVATE -w -include ${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.h)
endif()

set(PHYSICS_SOURCES
  src/main.c
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.c
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memview.c
  src/phys.c
  src/scene_bodies.c
  src/scene_userdata.c
  src/scene_callbacks.c
  src/scene_bridge.c
  src/scene_cleanup.c
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.h
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memview.h
  src/phys.h
  src/scenes.h
  src/tickets.h
  ${CMAKE_SOURCE_DIR}/Common/viz/viz.c
  ${CMAKE_SOURCE_DIR}/Common/viz/viz.h)

# PhysicsLab — версия Дины с багами в tickets.c, PhysicsLab_Teacher — с исправленным tickets_fixed.c.
add_executable(PhysicsLab ${PHYSICS_SOURCES} src/tickets.c)
add_executable(PhysicsLab_Teacher ${PHYSICS_SOURCES} src/tickets_fixed.c)

foreach(target PhysicsLab PhysicsLab_Teacher)
  set_property(TARGET ${target} PROPERTY C_STANDARD 11)
  target_link_libraries(${target} PRIVATE raylib chipmunk_lesson)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/Common/viz ${CMAKE_SOURCE_DIR}/Common/memtrack src)
  if (MSVC)
    target_compile_options(${target} PRIVATE /utf-8 /W3)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
endforeach()

# В файлах заданий malloc/free тоже идут через счётчик — тогда видно утечки и в нашем коде.
foreach(file src/tickets.c src/tickets_fixed.c)
  if (MSVC)
    set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "/FI${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack_redirect.h")
  else()
    set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "-include;${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack_redirect.h")
  endif()
endforeach()

# AddressSanitizer — тот же пресет "x64 Debug + ASan", что и в уроке 2.
if (AQUARIUM_ASAN AND MSVC)
  string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  get_filename_component(_msvc_bin "${CMAKE_C_COMPILER}" DIRECTORY)
  foreach(target PhysicsLab PhysicsLab_Teacher chipmunk_lesson)
    set_property(TARGET ${target} PROPERTY MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
    target_compile_options(${target} PRIVATE /fsanitize=address)
  endforeach()
  foreach(target PhysicsLab PhysicsLab_Teacher)
    target_link_options(${target} PRIVATE /INCREMENTAL:NO)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${_msvc_bin}/clang_rt.asan_dynamic-x86_64.dll" "$<TARGET_FILE_DIR:${target}>")
  endforeach()
endif()
