cmake_minimum_required(VERSION 3.10)

configure_file(config.h.in config.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(kpinentry
    logging.hpp
    logging.cpp
    assuan.hpp
    pinentry.hpp
    pinentry.cpp
    main.cpp)

target_link_libraries(kpinentry
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::WidgetsAddons
    KF5::Wallet
    PkgConfig::LibAssuan
    )

kde_target_enable_exceptions(kpinentry PRIVATE)
