# Урок 2: «Аквариум» — ищем баги в чужом коде на C.

# raylib: сначала ищем установленный (например, через vcpkg), иначе скачиваем исходники.
find_package(raylib 5.0 QUIET)
if (NOT raylib_FOUND)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz)
  FetchContent_MakeAvailable(raylib)
endif()

add_executable(Aquarium
  src/main.c
  src/fish.c
  src/food.c
  src/bubbles.c
  src/hud.c
  src/memstats.c
  src/aquarium.h
  src/memstats.h)

set_property(TARGET Aquarium PROPERTY C_STANDARD 11)
target_link_libraries(Aquarium PRIVATE raylib)

# AddressSanitizer — включается пресетом "x64 Debug + ASan" (или -DAQUARIUM_ASAN=ON).
option(AQUARIUM_ASAN "Собрать Aquarium с AddressSanitizer" OFF)
if (AQUARIUM_ASAN AND MSVC)
  # ASan несовместим с /RTC1 и Edit and Continue
  string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  set_property(TARGET Aquarium PROPERTY MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
  target_compile_options(Aquarium PRIVATE /fsanitize=address)
  target_link_options(Aquarium PRIVATE /INCREMENTAL:NO)

  # кладём рантайм ASan рядом с exe, чтобы запускалось и вне Visual Studio
  get_filename_component(_msvc_bin "${CMAKE_C_COMPILER}" DIRECTORY)
  add_custom_command(TARGET Aquarium POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_msvc_bin}/clang_rt.asan_dynamic-x86_64.dll" "$<TARGET_FILE_DIR:Aquarium>")
endif()
