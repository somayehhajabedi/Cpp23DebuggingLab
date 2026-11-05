find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest CMake package not found; building from /usr/src/googletest")
    add_subdirectory(/usr/src/googletest ${CMAKE_BINARY_DIR}/googletest-build)
    add_library(GTest::gtest ALIAS gtest)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()

include(GoogleTest)

add_executable(mdlab_tests
    ClockTest.cpp
    LoggerTest.cpp
    MarketDataParserTest.cpp
    MarketDataParserRobustnessTest.cpp
    SPSCQueueTest.cpp
    QueueDataIntegrityTest.cpp
    OrderBookTest.cpp
    OrderBookInvariantTest.cpp
    PositionTrackerTest.cpp
    RiskManagerTest.cpp
    RiskLimitBoundaryTest.cpp
    NotionalOverflowTest.cpp
    ExecutionEngineTest.cpp
    ExecutionEngineLifecycleTest.cpp
    RiskManagerConcurrencyTest.cpp
    PipelineLivenessTest.cpp
    SimulationThroughputSmokeTest.cpp
)

target_link_libraries(mdlab_tests PRIVATE mdlab_core GTest::gtest GTest::gtest_main Threads::Threads)
target_compile_definitions(mdlab_tests PRIVATE
    MDLAB_SAMPLE_FEED_PATH="${CMAKE_SOURCE_DIR}/data/sample_ticks.csv"
    MDLAB_MALFORMED_FEED_PATH="${CMAKE_SOURCE_DIR}/data/malformed_ticks.csv"
)

gtest_discover_tests(mdlab_tests PROPERTIES TIMEOUT 30)
