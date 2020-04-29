set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to ${default_build_type} as none was explicitly specified")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "${default_build_type}")
endif()
