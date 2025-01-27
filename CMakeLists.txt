cmake_minimum_required(VERSION 3.24)
#get options and compiling env
include(cmake/options.cmake)
include(cmake/ccache.cmake)

project(AdvancedFlowSchedulerLib LANGUAGES CXX)
project(AdvancedFlowSchedulerLib VERSION 1.1)


###############################################################################
# don't allow in-source builds
###############################################################################

# Ensure there are no in-source build
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed")
endif()


###############################################################################
# configure flags
###############################################################################
# compile flags
include(cmake/flags.cmake)
# cppcheck target
include(cmake/cppcheck.cmake)
# clang-tidy target
include(cmake/clang-tidy.cmake)

option(BUILD_TESTS "Enable building of tests." ON)
option(ENABLE_SANITIZERS "Enable sanitizers for debugging." OFF)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


###############################################################################
# library
###############################################################################
file(GLOB_RECURSE HEADER_LIST CONFIGURE_DEPENDS "include/*.hpp")
file(GLOB_RECURSE SOURCE_LIST CONFIGURE_DEPENDS "src/*.cpp")

add_library(AdvancedFlowSchedulerLib STATIC ${HEADER_LIST} ${SOURCE_LIST})

target_compile_features(AdvancedFlowSchedulerLib PUBLIC cxx_std_20)

# make headers available
target_include_directories(AdvancedFlowSchedulerLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

###############################################################################
# get dependencies
###############################################################################

# needed for multithreading
find_package(Threads REQUIRED)

include(cmake/namedtype.cmake)
include(cmake/CLI11.cmake)
include(cmake/nlohmann.cmake)
include(cmake/fmt.cmake)

# link against libraries
target_link_libraries(AdvancedFlowSchedulerLib LINK_PUBLIC
        Threads::Threads
        fmt
        NamedType
        nlohmann_json
        CLI11
        #tbb
)


###############################################################################
# Executable
###############################################################################

add_executable(AdvancedFlowSchedulerExec main.cpp)
set_flags(AdvancedFlowSchedulerExec)
setup_linker(AdvancedFlowSchedulerExec)
set_sanitizers(AdvancedFlowSchedulerExec)

target_compile_features(AdvancedFlowSchedulerExec PRIVATE cxx_std_20)

target_link_libraries(AdvancedFlowSchedulerExec PRIVATE
        AdvancedFlowSchedulerLib
)


###############################################################################
# Unit Tests
###############################################################################
if(BUILD_TESTS)
  message(STATUS "Build tests: yes")
  enable_testing()
  add_subdirectory(test)
else()
  message(STATUS "Build tests: no")
endif(BUILD_TESTS)
