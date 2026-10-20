## submodule_pins.cmake: the pin guard of the simulator.
## The build uses the contents of the folders of the submodules (engine/ and gvirtual-xray/), not the commits that the repository
## records for them, so a folder that is at another commit (for example, a newer version that is being worked on) makes the build
## differ from what the repository says it builds. xrsim_check_submodule_pin() compares the commit that is checked out in a folder
## with the commit that the index of the repository records for it, and stops the configuration if they differ. The index (not
## HEAD) is used so that a new pin can be tried with "git add <folder>" before it is committed.
##
## Usage: xrsim_check_submodule_pin(<folder> [SOURCE_DIR <directory of the repository>])
## Option: XRSIM_ALLOW_UNPINNED_SUBMODULES (OFF by default) turns the stop into a warning that says the same.
## The check is skipped, with a status message, when it cannot be made: Git is not available, the sources are not a Git checkout
## (for example, a source archive), the folder is not a checkout of its own (the submodule was not fetched), the repository records
## no commit for it, or its commit cannot be read.

# For the tests of this module only: the path of the Git program to use (an empty value means that Git is not available)
# XRSIM_PIN_GUARD_GIT_EXECUTABLE

# Returns a short description of a commit of a folder (the output of "git describe"), or an empty string if it cannot be made
function(_xrsim_describe_commit git folder_dir commit result)
	execute_process(
		COMMAND "${git}" -C "${folder_dir}" describe --tags --always "${commit}"
		OUTPUT_VARIABLE description
		RESULT_VARIABLE exit_code
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(exit_code EQUAL 0 AND NOT description STREQUAL "")
		set(${result} "${description}" PARENT_SCOPE)
	else()
		set(${result} "" PARENT_SCOPE)
	endif()
endfunction()

function(xrsim_check_submodule_pin folder)
	cmake_parse_arguments(PARSE_ARGV 1 ARG "" "SOURCE_DIR" "")
	if(NOT ARG_SOURCE_DIR)
		set(ARG_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
	endif()
	set(folder_dir "${ARG_SOURCE_DIR}/${folder}")

	# Git
	if(DEFINED XRSIM_PIN_GUARD_GIT_EXECUTABLE)
		set(git "${XRSIM_PIN_GUARD_GIT_EXECUTABLE}")
	else()
		find_package(Git QUIET)
		set(git "${GIT_EXECUTABLE}")
	endif()
	if(NOT git)
		message(STATUS "Pin check of '${folder}' skipped: Git was not found")
		return()
	endif()

	# A source archive has no .git, and a submodule that was not fetched has no .git of its own
	if(NOT EXISTS "${ARG_SOURCE_DIR}/.git")
		message(STATUS "Pin check of '${folder}' skipped: the sources are not a Git checkout")
		return()
	endif()
	if(NOT EXISTS "${folder_dir}/.git")
		message(STATUS "Pin check of '${folder}' skipped: the folder is not a checkout of its own (git submodule update --init ${folder})")
		return()
	endif()

	# The commit that the index records for the folder ("160000 <commit> <stage><TAB><folder>")
	execute_process(
		COMMAND "${git}" -C "${ARG_SOURCE_DIR}" ls-files -s -- "${folder}"
		OUTPUT_VARIABLE index_line
		RESULT_VARIABLE exit_code
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT exit_code EQUAL 0 OR NOT index_line MATCHES "^160000 ([0-9a-f]+) ")
		message(STATUS "Pin check of '${folder}' skipped: the index of the repository records no commit for it")
		return()
	endif()
	set(recorded "${CMAKE_MATCH_1}")

	# The commit that is checked out in the folder
	execute_process(
		COMMAND "${git}" -C "${folder_dir}" rev-parse HEAD
		OUTPUT_VARIABLE found
		RESULT_VARIABLE exit_code
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT exit_code EQUAL 0 OR found STREQUAL "")
		message(STATUS "Pin check of '${folder}' skipped: the commit checked out in the folder could not be read")
		return()
	endif()

	if(found STREQUAL recorded)
		string(SUBSTRING "${recorded}" 0 10 short)
		message(STATUS "'${folder}' is at the recorded commit ${short}")
		return()
	endif()

	_xrsim_describe_commit("${git}" "${folder_dir}" "${found}" found_description)
	_xrsim_describe_commit("${git}" "${folder_dir}" "${recorded}" recorded_description)
	if(found_description)
		set(found_description " (${found_description})")
	endif()
	if(recorded_description)
		set(recorded_description " (${recorded_description})")
	else()
		set(recorded_description " (not in the clone of the folder: git submodule update ${folder} fetches it)")
	endif()

	string(JOIN "
" text
		"The folder '${folder}' is not at the commit that this repository records for it."
		"  recorded in this repository: ${recorded}${recorded_description}"
		"  checked out in ${folder}/:   ${found}${found_description}"
		"The build uses the contents of the folder, not the recorded commit, so it would not build the version that the repository says it builds."
		"Two ways out:"
		"  1. Go back to the recorded commit:  git submodule update ${folder}"
		"     (commit or discard the work in ${folder}/ first: Git refuses to overwrite it)"
		"  2. Move the pin on purpose:  git add ${folder}  and commit it (the checked-out commit becomes the recorded one)")

	if(XRSIM_ALLOW_UNPINNED_SUBMODULES)
		message(WARNING "${text}
Continuing because XRSIM_ALLOW_UNPINNED_SUBMODULES is ON.")
	else()
		message(FATAL_ERROR "${text}
To configure anyway (for example, to try another version), add -DXRSIM_ALLOW_UNPINNED_SUBMODULES=ON")
	endif()
endfunction()
