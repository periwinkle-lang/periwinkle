# Знаходить бібліотеку PCRE2.

# Позначення змінних
# PCRE2_INCLUDE_DIRS - де знаходиться pcre2.h.
# PCRE2_LIBRARIES - список бібліотек.
# PCRE2_FOUND - чи була бібліотека знайдена.

# Знаходить хідер.
find_path(PCRE2_INCLUDE_DIR NAMES pcre2.h PATHS ${CMAKE_SOURCE_DIR}/libs/include)

# Знаходить бінарні файли бібліотеки.
find_library(PCRE2_LIBRARY_RELEASE NAMES pcre2-8 pcre2-8-static PATHS ${CMAKE_SOURCE_DIR}/libs)
find_library(PCRE2_LIBRARY_DEBUG NAMES pcre2-8d pcre2-8-staticd PATHS ${CMAKE_SOURCE_DIR}/libs)

# Встановлює значення PCRE2_FOUND.
include(FindPackageHandleStandardArgs)
include(SelectLibraryConfigurations)
SELECT_LIBRARY_CONFIGURATIONS(PCRE2)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE2 DEFAULT_MSG PCRE2_LIBRARY PCRE2_INCLUDE_DIR)

# Копіювання результатів в вихідні змінні.
if(PCRE2_FOUND)
	set(PCRE2_LIBRARIES ${PCRE2_LIBRARY})
	set(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
else()
	set(PCRE2_LIBRARIES)
	set(PCRE2_INCLUDE_DIRS)
endif()

mark_as_advanced(PCRE2_INCLUDE_DIRS PCRE2_LIBRARIES)
