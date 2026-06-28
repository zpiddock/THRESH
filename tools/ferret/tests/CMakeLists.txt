add_executable(ferret-tests test-roundtrip.cpp)
target_link_libraries(ferret-tests PRIVATE ferret doctest::doctest)
add_test(NAME ferret-tests COMMAND ferret-tests)