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
  ./src/sarsa_agent.cpp
  ./src/sarsa_action_factories.cpp)

# Main entry point.
add_executable(main
  ./src/main.cpp)

# Link core to main.
target_link_libraries(main
  core)

# Add flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -fno-rtti -g")

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
