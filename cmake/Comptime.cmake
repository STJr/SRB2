set(CMAKE_BINARY_DIR "${BINARY_DIR}")
set(CMAKE_CURRENT_BINARY_DIR "${BINARY_DIR}")

# Set up CMAKE path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(GitUtilities)

git_current_branch(SRB2_COMP_BRANCH)
git_summary(SRB2_COMP_REVISION)
git_working_tree_dirty(SRB2_COMP_UNCOMMITTED)

configure_file(src/config.h.in src/config.h)
