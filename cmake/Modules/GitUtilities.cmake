# Git utilities

if(__GitUtilities)
	return()
endif()

set(__GitUtilities ON)

macro(_git_command)
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" ${ARGN}
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE output
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endmacro()

macro(_git_easy_command)
	_git_command(${ARGN})
	set(${variable} "${output}" PARENT_SCOPE)
endmacro()

function(git_current_branch variable)
	_git_command(symbolic-ref -q --short HEAD)

	# If a detached head, a ref could still be resolved.
	if("${output}" STREQUAL "")
		_git_command(describe --all --exact-match)

		# Get the ref, in the form heads/master or
		# remotes/origin/master so isolate the final part.
		string(REGEX REPLACE ".*/" "" output "${output}")
	endif()

	set(${variable} "${output}" PARENT_SCOPE)
endfunction()

function(git_latest_commit variable)
	_git_easy_command(rev-parse --short HEAD)
endfunction()

function(git_working_tree_dirty variable)
	_git_command(status --porcelain -uno)

	if(output STREQUAL "")
		set(${variable} FALSE PARENT_SCOPE)
	else()
		set(${variable} TRUE PARENT_SCOPE)
	endif()
endfunction()

function(git_subject variable)
	_git_easy_command(log -1 --format=%s)
endfunction()

function(get_git_dir variable)
	_git_easy_command(rev-parse --git-dir)
endfunction()
