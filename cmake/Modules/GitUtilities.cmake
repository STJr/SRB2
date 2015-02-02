# Git utilities

if(__GitUtilities)
	return()
endif()

set(__GitUtilities ON)

function(git_describe variable path)
	execute_process(COMMAND "${GIT_EXECUTABLE}" "describe"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	#if(NOT result EQUAL 0)
	#	set(${variable} "GITERROR-${result}-NOTFOUND" CACHE STRING "revision" FORCE)
	#endif()

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()
