include(FetchContent)
include(GNUInstallDirs)

# Define variables for configuration
set(GTEST_FORCE_SHARED_CRT ON CACHE BOOL "" FORCE) # Use the shared CRT on Windows
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE) # Disable building of GMOCK
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE) # Disable installation of GTEST

# FetchContent configuration for GoogleTest
FetchContent_Declare(
        gtest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

FetchContent_MakeAvailable(gtest)
