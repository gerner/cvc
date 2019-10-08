cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# Call cmake with -D TESTS=ON to set this flag to true.
option(TESTS "build tests" OFF)

project(sample_project CXX C)

# Core and main are split. This allows us to link core to main and tests.

# Core library. *.cpp should be added here.
add_library(core
  ./src/core.cpp
  ./src/decision_engine.cpp
  ./src/action.cpp
  ./src/action_factories.cpp
  ./src/sarsa/sarsa_agent.cpp
  ./src/sarsa/sarsa_action_factories.cpp)

# Main entry point.
add_executable(main
  ./src/main.cpp)

# Link core to main.
target_link_libraries(main
  core)

# Add flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -fno-rtti -g")

if(TESTS)

  #include(GoogleTest)

  enable_testing()

  # Process the CMakeLists.txt in third-party/googletest, sets project(gtest) and
  # project(gmock), gives access to ${gtest_SOURCE_DIR} and ${gmock_SOURCE_DIR}.
  add_subdirectory(third-party/googletest)

  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
  include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

  # Tests. *-test.cpp should be added here.
  add_executable(tests
    ./test/core_test.cpp
    ./test/decision_engine_test.cpp
    ./test/sarsa_agent_test.cpp)
#
  # Link core, pthread and gtest to tests.
  target_link_libraries(tests
    pthread
    gtest
    gtest_main
    core)

  #gtest_discover_tests(tests)

endif()

if(CMAKE_BUILD_TYPE STREQUAL "coverage" OR CODE_COVERAGE)
    if("${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
        message("Building with llvm Code Coverage Tools")

        # Warning/Error messages
        find_program(LLVM_COV_PATH llvm-cov)
        if(NOT LLVM_COV_PATH)
            message(FATAL_ERROR "llvm-cov not found! Aborting.")
        endif()

        # set Flags
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")

    elseif(CMAKE_COMPILER_IS_GNUCXX)
        message("Building with lcov Code Coverage Tools")

        # Warning/Error messages
        if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug"))
            message(WARNING "Code coverage results with an optimized (non-Debug) build may be misleading")
        endif()
        if(NOT LCOV_PATH)
            message(FATAL_ERROR "lcov not found! Aborting...")
        endif()
        if(NOT GENHTML_PATH)
            message(FATAL_ERROR "genhtml not found! Aborting...")
        endif()

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    else()
        message(FATAL_ERROR "Code coverage requires Clang or GCC. Aborting.")
    endif()
endif()
