cmake_minimum_required(VERSION 3.16)

project(Барвінок VERSION 0.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
find_package(PCRE2)
find_package(JPCRE2)

configure_file("pconfig.h.in" "pconfig.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR}) # Для видимості pconfig.h в проекті


if(NOT PCRE2_FOUND)
    message(FATAL_ERROR "Бібліотеку PCRE2 не знайдено!")
endif()
if(NOT JPCRE2_FOUND)
    message(FATAL_ERROR "Бібліотеку JPCRE2 не знайдено!")
endif()


# Підтримка юнікоду в msvc
if (MSVC)
    add_compile_options("/utf-8")
endif()


include_directories(
    "src/include"
    "src/include/compiler"
    "src/include/lexer"
    "src/include/object"
    "src/include/parser"
    "src/include/vm"
)


# Бібліотека з реалізацією барвінку
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
    "src/periwinkle/lexer/lexer.cpp" "src/include/lexer/lexer.h"
    "src/periwinkle/compiler/scope.cpp" "src/include/compiler/scope.h"
    "src/periwinkle/compiler/compiler.cpp" "src/include/compiler/compiler.h"
    "src/periwinkle/compiler/decompiler.cpp" "src/include/compiler/decompiler.h"
    "src/periwinkle/parser/parser.cpp" "src/include/parser/parser.h"
    "src/include/parser/expression.h" 
    "src/include/parser/node.h" 
    "src/include/parser/node_kind.h" 
    "src/include/parser/statement.h" 
    "src/include/plogger.h"
    "src/periwinkle/vm/builtins.cpp" "src/include/vm/builtins.h"
    "src/periwinkle/object/call.cpp" "src/include/object/call.h"
    "src/periwinkle/periwinkle.cpp" "src/include/periwinkle.h" 
 "src/include/object/real_object.h" "src/periwinkle/object/real_object.cpp")
target_compile_features(periwinkle PUBLIC cxx_std_20)


# Виконуваний файл
add_executable(periwinkle-bin "src/periwinkle/main.cpp" )
target_compile_features(periwinkle-bin PUBLIC cxx_std_20)
set_target_properties(periwinkle-bin PROPERTIES OUTPUT_NAME periwinkle)


# Додавання бібліотек до виконуваного файлу
target_include_directories(periwinkle PUBLIC ${PCRE2_INCLUDE_DIRS} ${JPCRE2_INCLUDE_DIRS})
target_link_libraries(periwinkle ${PCRE2_LIBRARIES})
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
