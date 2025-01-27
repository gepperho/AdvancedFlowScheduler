cmake_minimum_required(VERSION 3.24)

include(../cmake/flags.cmake)
include(../cmake/gtest.cmake)

add_executable(unit_tests
        GraphCreationTest.cpp
        routing/DijkstraOverlapTest.cpp
        routing/KShortestTest.cpp
        routing/RoutingTests.cpp
        UtilTest.cpp
        UtilizationListTest.cpp
        UtilizationListSearchTransmissionOpportunities.cpp
        MultiLayeredGraphTest.cpp
        scheduler/EdfTest.cpp
        scheduler/FirstFitTest.cpp
        scheduler/HermesTest.cpp
        main.cpp
)

target_compile_features(unit_tests PRIVATE cxx_std_20)

target_link_libraries(unit_tests PRIVATE AdvancedFlowSchedulerLib gtest gtest_main)
target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_flags(unit_tests)
setup_linker(unit_tests)
set_sanitizers(unit_tests)

include(GoogleTest)
gtest_discover_tests(unit_tests)
