find_path(UTFCPP_INCLUDE_DIR "utf8.h"
  PATH_SUFFIXES "" "utf8cpp")
mark_as_advanced(UTFCPP_INCLUDE_DIR)
set(UTFCPP_INCLUDE_DIRS ${UTFCPP_INCLUDE_DIR})

if (UTFCPP_INCLUDE_DIR)
  set(UTFCPP_FOUND True)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UTFCPP DEFAULT_MSG UTFCPP_INCLUDE_DIR)
