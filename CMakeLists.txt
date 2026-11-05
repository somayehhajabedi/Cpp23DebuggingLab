cmake_minimum_required(VERSION 3.25)
project(Cpp23DebuggingLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

option(MDLAB_ENABLE_WARNINGS "Build with -Wall -Wextra -Wpedantic" ON)
option(MDLAB_BUILD_TESTS "Build the GoogleTest suite" ON)

add_library(mdlab_warnings INTERFACE)
if(MDLAB_ENABLE_WARNINGS)
    target_compile_options(mdlab_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# std::stacktrace (used only by the logger's fatal-log path) needs a
# separate runtime support library on libstdc++ (libstdc++exp). Probe with
# that library already on the link line, since the header-only check alone
# compiles but fails to link without it.
include(CheckCXXSourceCompiles)
set(MDLAB_STACKTRACE_LIBS "")
set(MDLAB_HAVE_STACKTRACE OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # The compiler driver resolves "stdc++exp" from its own internal search
    # path even though it isn't a standard system library directory, so we
    # pass it by bare name rather than via find_library().
    set(CMAKE_REQUIRED_LIBRARIES "stdc++exp")
    check_cxx_source_compiles(
        "#include <stacktrace>\nint main(){auto s = std::stacktrace::current(); return static_cast<int>(s.size());}"
        MDLAB_STACKTRACE_COMPILES)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(MDLAB_STACKTRACE_COMPILES)
        set(MDLAB_STACKTRACE_LIBS "stdc++exp")
        set(MDLAB_HAVE_STACKTRACE ON)
    endif()
endif()

add_library(mdlab_core STATIC
    src/core/Clock.cpp
    src/logger/Logger.cpp
    src/marketdata/MarketDataTypes.cpp
    src/marketdata/MarketDataParser.cpp
    src/orderbook/OrderBook.cpp
    src/risk/PositionTracker.cpp
    src/risk/RiskManager.cpp
    src/execution/OrderGateway.cpp
    src/execution/ExecutionEngine.cpp
    src/pipeline/MarketDataFeedHandler.cpp
)
target_include_directories(mdlab_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mdlab_core PUBLIC Threads::Threads mdlab_warnings)
if(MDLAB_HAVE_STACKTRACE)
    target_compile_definitions(mdlab_core PUBLIC MDLAB_HAVE_STACKTRACE=1)
    target_link_libraries(mdlab_core PUBLIC ${MDLAB_STACKTRACE_LIBS})
endif()

add_executable(simulation apps/simulation_main.cpp)
target_link_libraries(simulation PRIVATE mdlab_core)

if(MDLAB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
