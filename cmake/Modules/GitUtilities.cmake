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

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()

function(git_current_branch variable path)
	execute_process(COMMAND ${GIT_EXECUTABLE} "symbolic-ref" "--short" "HEAD"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()

function(git_latest_commit variable path)
	execute_process(COMMAND ${GIT_EXECUTABLE} "rev-parse" "--short" "HEAD"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()

function(git_working_tree_dirty variable path)
	execute_process(COMMAND ${GIT_EXECUTABLE} "status" "--porcelain" "-uno"
		WORKING_DIRECTORY "${path}"
		RESULT_VARIABLE result
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if(output STREQUAL "")
		set(${variable} FALSE PARENT_SCOPE)
	else()
		set(${variable} TRUE PARENT_SCOPE)
	endif()
endfunction()