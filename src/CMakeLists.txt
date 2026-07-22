# ── Library target ──────────────────────────────────────────────────────────
#   Target names follow the project name set in the root CMakeLists.txt.
#   ${PROJECT_NAME}    → e.g. "my_project"
#   ${PROJECT_NAME}_lib → e.g. "my_project_lib"
add_library(${PROJECT_NAME}_lib
    math_utils.hpp
    math_utils.cpp
)

target_include_directories(${PROJECT_NAME}_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

apply_firewall(${PROJECT_NAME}_lib)
apply_sanitizers(${PROJECT_NAME}_lib)

if(${PROJ_PREFIX}_ENABLE_STL_HARDENING)
    apply_stl_hardening(${PROJECT_NAME}_lib)
endif()

# ── Executable target ──────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${PROJECT_NAME}_lib
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

apply_firewall(${PROJECT_NAME})
apply_sanitizers(${PROJECT_NAME})

if(${PROJ_PREFIX}_ENABLE_STL_HARDENING)
    apply_stl_hardening(${PROJECT_NAME})
endif()
