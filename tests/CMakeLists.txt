# ── Test target ─────────────────────────────────────────────────────────────
#   Target names follow the project name: ${PROJECT_NAME}_test
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(${PROJECT_NAME}_test
    test_main.cpp
    test_math.cpp
)

target_link_libraries(${PROJECT_NAME}_test
    PRIVATE
        ${PROJECT_NAME}_lib
        Catch2::Catch2WithMain
)

apply_firewall(${PROJECT_NAME}_test)
apply_sanitizers(${PROJECT_NAME}_test)

add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
