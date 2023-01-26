# Знаходить бібліотеку JPCRE2.

# Позначення змінних
# JPCRE2_INCLUDE_DIRS - де знаходиться jpcre2.h.
# JPCRE2_FOUND - чи була бібліотека знайдена.

# Знаходить хідер.
find_path(JPCRE2_INCLUDE_DIR NAMES jpcre2.hpp PATHS ${CMAKE_SOURCE_DIR}/libs/include)

# Встановлює значення JPCRE2_FOUND.
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JPCRE2 DEFAULT_MSG JPCRE2_INCLUDE_DIR)

# Копіювання результатів в вихідні змінні.
if(JPCRE2_FOUND)
	set(JPCRE2_INCLUDE_DIRS ${JPCRE2_INCLUDE_DIR})
else()
	set(JPCRE2_INCLUDE_DIRS)
endif()

mark_as_advanced(JPCRE2_INCLUDE_DIRS)
