# ---------------------------------------------------------------------------
# bomberman_tests
#
# The retake grades "the tests you wrote" as part of the 40% code-quality
# criterion, and the logic library is testable precisely because it does
# not depend on SFML: these run with no window and no audio device.
#
# sif's test framework is reused from the fetched sources rather than
# copied in - one self-registering SIF_TEST macro, nothing to wire up.
# ---------------------------------------------------------------------------

# The framework itself (registrar + runner main) comes from the fetched
# sif sources; only the test *cases* below are ours. Listed rather than
# globbed - see logic/CMakeLists.txt and view/CMakeLists.txt for why.
add_executable(bomberman_tests
        BotAiTests.cpp
        ConfigTests.cpp
        LogicTests.cpp
        PlayerProfileTests.cpp
        PowerUpTests.cpp
        "${SIF_TEST_FRAMEWORK_DIR}/TestFramework.cpp"
        "${SIF_TEST_FRAMEWORK_DIR}/main.cpp"
)

target_include_directories(bomberman_tests PRIVATE "${SIF_TEST_FRAMEWORK_DIR}")
target_link_libraries(bomberman_tests PRIVATE bomberman_logic)

add_test(NAME bomberman_logic_tests COMMAND bomberman_tests)
