cmake_minimum_required(VERSION 3.10)

project(kpinentry VERSION 0.1.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(ProhibitInSource)
include(DefaultBuildType)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(
    -Wall -Wextra -Wpedantic -Wconversion
    "$<IF:$<CONFIG:Debug>,-g;-O0,>"
)
add_compile_definitions(
    "$<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>"
)
set(CMAKE_VERBOSE_MAKEFILE on)

find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings NO_POLICY_SCOPE)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# Qt components
set(QT_MIN_VERSION "5.12.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui)

# KDE components
set(KF5_MIN_VERSION "5.66.0")
# silence FindPackageHandleStandardArgs warnings
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    I18n
    WidgetsAddons
    Wallet)

# Other packages/libraries
find_package(LibAssuan REQUIRED)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(src)
