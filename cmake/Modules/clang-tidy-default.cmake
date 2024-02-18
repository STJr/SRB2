find_program(CLANG_TIDY clang-tidy)

# Note: Apple Clang does not ship with clang tools. If you want clang-tidy on
# macOS, it's best to install the Homebrew llvm bottle and set CLANG_TIDY
# in your build directory. The llvm package is keg-only, so it will not
# collide with Apple Clang.

function(target_set_default_clang_tidy target lang checks)
    if("${CLANG_TIDY}" STREQUAL "CLANG_TIDY-NOTFOUND")
        return()
    endif()

    get_target_property(c_clang_tidy_prop SRB2SDL2 C_CLANG_TIDY)
    if(NOT ("${c_clang_tidy_prop}" STREQUAL "c_clang_tidy_prop-NOTFOUND"))
        return()
    endif()

    set_target_properties("${target}" PROPERTIES
	    ${lang}_CLANG_TIDY "${CLANG_TIDY};-checks=${checks}"
    )
endfunction()
