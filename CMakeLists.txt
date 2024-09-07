cmake_minimum_required(VERSION 3.20)

project(Барвінок VERSION 0.0.0 LANGUAGES CXX)

include(GenerateExportHeader)
if(NOT WIN32)
    set(Python3_FIND_UNVERSIONED_NAMES NEVER)
endif()
find_package(Python3 3.10)

configure_file("include/pconfig.hpp.in" "pconfig.hpp")
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # Для видимості pconfig.hpp в проекті


if (NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "Python 3 не знайдено!")
endif()


# Підтримка юнікоду в msvc
if (MSVC)
    add_compile_options("/utf-8")
endif()


# Бібліотека з реалізацією Барвінку
add_library(periwinkle SHARED
    "include/types.hpp"
    "include/string_enum.hpp"
    "periwinkle/utils.cpp" "include/utils.hpp"
    "periwinkle/object/object.cpp" "include/object/object.hpp"
    "periwinkle/object/bool_object.cpp" "include/object/bool_object.hpp"
    "periwinkle/object/int_object.cpp" "include/object/int_object.hpp"
    "periwinkle/object/function_object.cpp" "include/object/function_object.hpp"
    "periwinkle/object/native_function_object.cpp" "include/object/native_function_object.hpp"
    "periwinkle/object/null_object.cpp" "include/object/null_object.hpp"
    "periwinkle/object/code_object.cpp" "include/object/code_object.hpp"
    "periwinkle/object/string_object.cpp" "include/object/string_object.hpp"
    "periwinkle/vm/vm.cpp" "include/vm/vm.hpp"
    "periwinkle/compiler/scope.cpp" "include/compiler/scope.hpp"
    "periwinkle/compiler/compiler.cpp" "include/compiler/compiler.hpp"
    "periwinkle/compiler/disassembler.cpp" "include/compiler/disassembler.hpp"
    "include/ast/ast.hpp"
    "include/ast/keyword.hpp"
    "include/plogger.hpp"
    "periwinkle/vm/builtins.cpp" "include/vm/builtins.hpp"
    "periwinkle/periwinkle.cpp" "include/periwinkle.hpp"
    "periwinkle/object/real_object.cpp" "include/object/real_object.hpp"
    "periwinkle/object/exception_object.cpp" "include/object/exception_object.hpp"
    "periwinkle/object/cell_object.cpp" "include/object/cell_object.hpp"
    "periwinkle/object/native_method_object.cpp" "include/object/native_method_object.hpp"
    "periwinkle/object/list_object.cpp" "include/object/list_object.hpp"
    "periwinkle/object/end_iteration_object.cpp" "include/object/end_iteration_object.hpp"
    "periwinkle/vm/argument_parser.cpp" "include/vm/argument_parser.hpp"
    "periwinkle/object/method_with_instance_object.cpp" "include/object/method_with_instance_object.hpp"
    "periwinkle/object/string_vector_object.cpp" "include/object/string_vector_object.hpp"
    "periwinkle/program_source.cpp" "include/program_source.hpp"
    "periwinkle/vm/gc.cpp" "include/vm/gc.hpp"
    "periwinkle/unicode.cpp" "include/unicode.hpp" "unicode_database.hpp"
    "include/platform.hpp"
)
target_include_directories(periwinkle PUBLIC
    "include"
    "include/compiler"
    "include/object"
    "include/ast"
    "include/vm"
)
target_compile_features(periwinkle PUBLIC cxx_std_20)
generate_export_header(periwinkle
    BASE_NAME periwinkle
    EXPORT_MACRO_NAME API
    EXPORT_FILE_NAME exports.hpp
)

# Парсер
add_library(parser STATIC
    "parser.hpp"
    "parser.cpp"
)
target_include_directories(parser PUBLIC "include/")
target_compile_features(parser PUBLIC cxx_std_17)
set_property(TARGET parser PROPERTY POSITION_INDEPENDENT_CODE ON)


file(DOWNLOAD
    https://github.com/romanfedyniak/pparser/releases/download/0.1.3/pparser.py
    ${CMAKE_BINARY_DIR}/pparser.py
)

# Генерація парсера
add_custom_command(
    OUTPUT "parser.hpp" "parser.cpp"
    PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_BINARY_DIR}/pparser.py ${CMAKE_SOURCE_DIR}/барвінок.граматика
    DEPENDS ${CMAKE_SOURCE_DIR}/барвінок.граматика ${CMAKE_BINARY_DIR}/pparser.py
    COMMENT "Генерація парсера..."
)


# Генерація бази даних юнікоду
add_custom_command(
    OUTPUT "unicode_database.hpp"
    PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/unicode/generate.py
    DEPENDS ${CMAKE_SOURCE_DIR}/tools/unicode/generate.py
    COMMENT "Генерація бази даних юнікоду..."
)


# Виконуваний файл
add_executable(launcher "launcher/launcher.cpp" "launcher/launcher.hpp")
target_include_directories(launcher PUBLIC "include/")
target_compile_features(launcher PUBLIC cxx_std_20)
set_target_properties(launcher PROPERTIES OUTPUT_NAME барвінок)


# Додавання бібліотек до виконуваного файлу
target_link_libraries(periwinkle parser)
target_link_libraries(launcher periwinkle)


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(periwinkle PRIVATE "IS_LINUX")
  target_compile_definitions(launcher PRIVATE "IS_LINUX")

  target_sources(periwinkle PRIVATE "periwinkle/platform_linux.cpp")
  target_sources(launcher PRIVATE "launcher/main.cpp")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_definitions(periwinkle PRIVATE "IS_MACOS")
  target_compile_definitions(launcher PRIVATE "IS_MACOS")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_definitions(periwinkle PRIVATE "IS_WINDOWS")
  target_compile_definitions(launcher PRIVATE "IS_WINDOWS")

  target_sources(periwinkle PRIVATE "periwinkle/platform_win.cpp")
  target_sources(launcher PRIVATE "launcher/wmain.cpp")
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Додаткові параметри компілятора
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(periwinkle PUBLIC -Wall -Wpedantic)
        target_compile_options(launcher PUBLIC -Wall -Wpedantic)
    endif()

    # Визначення DEV_TOOLS макроса
    target_compile_definitions(periwinkle PRIVATE DEV_TOOLS)
    target_compile_definitions(launcher PRIVATE DEV_TOOLS)
endif()
