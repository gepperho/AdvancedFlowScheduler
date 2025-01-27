find_program(CLANG_TIDY NAMES clang-tidy)
if (CLANG_TIDY)
    add_custom_target(
            clang-tidy
            COMMAND ${CLANG_TIDY}
            ${SOURCE_FILES}
    )
endif ()
