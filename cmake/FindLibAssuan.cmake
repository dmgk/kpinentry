find_package(PkgConfig REQUIRED QUIET)

pkg_check_modules(LibAssuan REQUIRED IMPORTED_TARGET libassuan>2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibAssuan
    FOUND_VAR
        LibAssuan_FOUND
    REQUIRED_VARS
        LibAssuan_LIBRARIES
    VERSION_VAR
        LibAssuan_VERSION)
