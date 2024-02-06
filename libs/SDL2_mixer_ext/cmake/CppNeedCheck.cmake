
function(cpp_needed _test_file _macros _include_paths _link_libs _output)
    if(NOT MSVC)
        if("${CMAKE_SYSTEM_NAME}" MATCHES "(Open|Free|Net)BSD")
            set(STDCPP_LIB c++abi)
        else()
            set(STDCPP_LIB stdc++)
        endif()

        try_compile(LIBRARY_WITH_STDCPP
            ${CMAKE_BINARY_DIR}/compile_tests
            ${_test_file}
            COMPILE_DEFINITIONS ${_macros}
            LINK_LIBRARIES ${_link_libs} ${STDCPP_LIB}
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${_link_libs}"
            OUTPUT_VARIABLE MODPLUG_TEST_RESULT
        )
        try_compile(LIBRARY_WITHOUT_STDCPP
            ${CMAKE_BINARY_DIR}/compile_tests
            ${_test_file}
            COMPILE_DEFINITIONS ${_macros}
            LINK_LIBRARIES ${_link_libs}
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${_include_paths}"
            OUTPUT_VARIABLE MODPLUG_TEST_RESULT
        )

        if(LIBRARY_WITH_STDCPP)
            message("-- ${_link_libs} works with stdc++ ${LIBRARY_WITH_STDCPP}")
        else()
            message("-- ${_link_libs} fails with stdc++ ${LIBRARY_WITH_STDCPP}")
        endif()

        if(LIBRARY_WITHOUT_STDCPP)
            message("-- ${_link_libs} works without stdc++ ${LIBRARY_WITHOUT_STDCPP}")
        else()
            message("-- ${_link_libs} fails without stdc++ ${LIBRARY_WITHOUT_STDCPP}")
        endif()

        if(LIBRARY_WITH_STDCPP AND NOT LIBRARY_WITHOUT_STDCPP)
            set(${_output} TRUE PARENT_SCOPE)
        endif()

    else()
        set(${_output} FALSE PARENT_SCOPE)
    endif()
endfunction()
