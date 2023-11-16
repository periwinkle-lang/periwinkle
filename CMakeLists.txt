cmake_minimum_required(VERSION 3.20)

project(Барвінок VERSION 0.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(PCRE2)
find_package(JPCRE2)
if(NOT WIN32)
    set(Python3_FIND_UNVERSIONED_NAMES NEVER)
endif()
find_package(Python3 3.10)

configure_file("pconfig.h.in" "pconfig.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # Для видимості pconfig.h в проекті


if(NOT PCRE2_FOUND)
    message(FATAL_ERROR "Бібліотеку PCRE2 не знайдено!")
endif()
if(NOT JPCRE2_FOUND)
    message(FATAL_ERROR "Бібліотеку JPCRE2 не знайдено!")
endif()
if (NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "Python 3 не знайдено!")
endif()


# Підтримка юнікоду в msvc
if (MSVC)
    add_compile_options("/utf-8")
endif()


include_directories(
    "src/include"
    "src/include/compiler"
    "src/include/object"
    "src/include/ast"
    "src/include/vm"
)


# Бібліотека з реалізацією Барвінку
add_library(periwinkle STATIC
    "src/include/types.h"
    "src/include/string_enum.h"
    "src/periwinkle/utils.cpp" "src/include/utils.h"
    "src/periwinkle/vm/exception.cpp" "src/include/vm/exception.h"
    "src/periwinkle/object/object.cpp" "src/include/object/object.h"
    "src/periwinkle/object/bool_object.cpp" "src/include/object/bool_object.h"
    "src/periwinkle/object/int_object.cpp" "src/include/object/int_object.h"
    "src/periwinkle/object/function_object.cpp" "src/include/object/function_object.h"
    "src/periwinkle/object/native_function_object.cpp" "src/include/object/native_function_object.h"
    "src/periwinkle/object/null_object.cpp" "src/include/object/null_object.h"
    "src/periwinkle/object/code_object.cpp" "src/include/object/code_object.h"
    "src/periwinkle/object/string_object.cpp" "src/include/object/string_object.h"
    "src/periwinkle/vm/vm.cpp" "src/include/vm/vm.h"
    "src/periwinkle/compiler/scope.cpp" "src/include/compiler/scope.h"
    "src/periwinkle/compiler/compiler.cpp" "src/include/compiler/compiler.h"
    "src/periwinkle/compiler/disassembler.cpp" "src/include/compiler/disassembler.h"
    "src/include/ast/ast.h"
    "src/include/ast/keyword.h"
    "src/include/plogger.h"
    "src/periwinkle/vm/builtins.cpp" "src/include/vm/builtins.h"
    "src/periwinkle/periwinkle.cpp" "src/include/periwinkle.h"
    "src/include/object/real_object.h" "src/periwinkle/object/real_object.cpp"
    "src/include/object/exception_object.h" "src/periwinkle/object/exception_object.cpp"
    "src/include/object/cell_object.h" "src/periwinkle/object/cell_object.cpp"
    "src/include/object/native_method_object.h" "src/periwinkle/object/native_method_object.cpp"
    "src/include/object/list_object.h" "src/periwinkle/object/list_object.cpp"
    "src/include/object/end_iteration_object.h" "src/periwinkle/object/end_iteration_object.cpp"
    "src/include/vm/argument_parser.h" "src/periwinkle/vm/argument_parser.cpp"
    "src/include/object/method_with_instance_object.h" "src/periwinkle/object/method_with_instance_object.cpp"
    "src/include/object/string_vector_object.h" "src/periwinkle/object/string_vector_object.cpp"
    "src/include/object/validate_args.h" "src/periwinkle/object/validate_args.cpp"
)
target_compile_features(periwinkle PUBLIC cxx_std_20)


# Парсер
add_library(parser STATIC
    "parser.hpp"
    "parser.cpp"
)
target_compile_features(parser PUBLIC cxx_std_17)


# Генерація парсера
add_custom_command(
    OUTPUT "parser.hpp" "parser.cpp"
    PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/thirdparty/pparser.py ${CMAKE_SOURCE_DIR}/барвінок.граматика
    DEPENDS ${CMAKE_SOURCE_DIR}/барвінок.граматика
    COMMENT "Генерація парсера..."
)


# Виконуваний файл
add_executable(periwinkle-bin "src/periwinkle/main.cpp")
target_compile_features(periwinkle-bin PUBLIC cxx_std_20)
set_target_properties(periwinkle-bin PROPERTIES OUTPUT_NAME барвінок)


# Додавання бібліотек до виконуваного файлу
target_include_directories(periwinkle PUBLIC ${PCRE2_INCLUDE_DIRS} ${JPCRE2_INCLUDE_DIRS})
target_link_libraries(periwinkle ${PCRE2_LIBRARIES})
target_link_libraries(periwinkle parser)
target_link_libraries(periwinkle-bin periwinkle)


# Додаткові параметри компілятора
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(periwinkle PUBLIC -Wall -Wpedantic)
        target_compile_options(periwinkle-bin PUBLIC -Wall -Wpedantic)
    endif()
endif()


# Визначення DEBUG макроса
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(periwinkle PRIVATE DEBUG)
    target_compile_definitions(periwinkle-bin PRIVATE DEBUG)
endif()
