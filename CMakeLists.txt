# CMakeList.txt: файл проекта CMake верхнего уровня; проведите глобальную конфигурацию
# и включите сюда подпроекты.
#
cmake_minimum_required (VERSION 3.10)

# Включение горячей перезагрузки для компиляторов MSVC, если поддерживается.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("DincikLessons")

# Включите подпроекты.
add_subdirectory ("DincikLessons")
add_subdirectory ("Lesson2_Aquarium")
add_subdirectory ("Lesson3_Memory")
add_subdirectory ("Lesson4_Physics")
add_subdirectory ("Lesson5_Life")
add_subdirectory ("Lesson6_Basics")
