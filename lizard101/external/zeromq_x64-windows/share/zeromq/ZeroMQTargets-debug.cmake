#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libzmq" for configuration "Debug"
set_property(TARGET libzmq APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libzmq PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/libzmq-mt-gd-4_3_5.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/libzmq-mt-gd-4_3_5.dll"
  )

list(APPEND _cmake_import_check_targets libzmq )
list(APPEND _cmake_import_check_files_for_libzmq "${_IMPORT_PREFIX}/debug/lib/libzmq-mt-gd-4_3_5.lib" "${_IMPORT_PREFIX}/debug/bin/libzmq-mt-gd-4_3_5.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
