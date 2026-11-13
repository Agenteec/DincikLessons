# Урок 6: «Основы C шаг за шагом» — консольный тренажёр: переменные, циклы, указатели, массивы,
# строки, стек и куча, структуры, void*, простые алгоритмы.

set(BASICS_SOURCES
  src/main.c
  src/ui.c
  src/checks.c
  src/quiz.c
  src/ui.h
  src/checks.h
  src/quiz.h
  src/tasks.h
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.c
  ${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack.h)

# BasicsLab — версия Дины (tasks.c), BasicsLab_Teacher — с ответами (tasks_fixed.c).
add_executable(BasicsLab ${BASICS_SOURCES} src/tasks.c)
add_executable(BasicsLab_Teacher ${BASICS_SOURCES} src/tasks_fixed.c)

foreach(target BasicsLab BasicsLab_Teacher)
  set_property(TARGET ${target} PROPERTY C_STANDARD 11)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/Common/memtrack src)
  if (MSVC)
    target_compile_options(${target} PRIVATE /utf-8)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
endforeach()

# malloc/free в заданиях незаметно идут через счётчик памяти: так программа видит утечки и мусор.
foreach(file src/tasks.c src/tasks_fixed.c)
  if (MSVC)
    set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "/FI${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack_redirect.h")
  else()
    set_source_files_properties(${file} PROPERTIES COMPILE_OPTIONS "-include;${CMAKE_SOURCE_DIR}/Common/memtrack/memtrack_redirect.h")
  endif()
endforeach()
