## Test of the pin guard (cmake/submodule_pins.cmake). It builds small Git repositories in a work directory: a repository whose index
## records a commit for a folder that is a checkout of its own (what a submodule is, without needing a remote), and runs the guard on
## them with the folder at the recorded commit, at another commit, with the option that allows it, with a new pin in the index, and in
## the situations where the check is skipped (no Git, no Git checkout, a folder that was not fetched, no commit recorded).
## cmake -DGUARD_MODULE=<path of submodule_pins.cmake> -DWORK_DIR=<directory for the repositories> -P submodule_pin_guard_test.cmake

set(CASE_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/submodule_pin_guard_case.cmake")

find_package(Git QUIET)
if(NOT GIT_EXECUTABLE)
	message(STATUS "SKIPPED: Git was not found, so the pin guard cannot be tested")
	return()
endif()

# A failed check is reported and remembered, so that the other cases still run
function(fail description)
	message(SEND_ERROR "[ERROR::SUBMODULE_PIN_GUARD_TEST] ${description}")
	set_property(GLOBAL PROPERTY PIN_GUARD_TEST_FAILED TRUE)
endfunction()

# Runs Git in a directory, with an identity and without signing, and stops if it fails. The output is returned (without the end of line)
function(git directory output)
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" -c user.name=Test -c user.email=test@example.com -c commit.gpgsign=false -C "${directory}" ${ARGN}
		OUTPUT_VARIABLE text
		ERROR_VARIABLE error_text
		RESULT_VARIABLE exit_code
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT exit_code EQUAL 0)
		message(FATAL_ERROR "[ERROR::SUBMODULE_PIN_GUARD_TEST] git ${ARGN} failed in ${directory}: ${error_text}")
	endif()
	set(${output} "${text}" PARENT_SCOPE)
endfunction()

# Creates an empty repository
function(make_repository directory)
	file(MAKE_DIRECTORY "${directory}")
	git("${directory}" ignored init --quiet)
endfunction()

# Runs the guard on a repository and checks the result: the exit code (0 or "fail"), and that the output has every given text
function(check_case name source_dir expected_result)
	execute_process(
		COMMAND "${CMAKE_COMMAND}" -DGUARD_MODULE=${GUARD_MODULE} -DSOURCE_DIR=${source_dir} ${CASE_OPTIONS} -P "${CASE_SCRIPT}"
		OUTPUT_VARIABLE out
		ERROR_VARIABLE err
		RESULT_VARIABLE exit_code)
	set(output "${out}\n${err}")

	if(expected_result STREQUAL "ok" AND NOT exit_code EQUAL 0)
		fail("${name}: the guard stopped, and it should not have: ${output}")
	elseif(expected_result STREQUAL "stop" AND exit_code EQUAL 0)
		fail("${name}: the guard did not stop: ${output}")
	endif()
	foreach(text IN LISTS ARGN)
		string(FIND "${output}" "${text}" position)
		if(position EQUAL -1)
			fail("${name}: the output does not have '${text}': ${output}")
		endif()
	endforeach()
	message(STATUS "${name}: ${expected_result}")
endfunction()

file(REMOVE_RECURSE "${WORK_DIR}")

# The repository "super" records a commit for its folder "engine", which is a repository with two commits
set(super "${WORK_DIR}/super")
make_repository("${super}")
make_repository("${super}/engine")
file(WRITE "${super}/engine/file.txt" "one\n")
git("${super}/engine" ignored add file.txt)
git("${super}/engine" ignored commit --quiet -m one)
git("${super}/engine" first_commit rev-parse HEAD)
file(WRITE "${super}/engine/file.txt" "two\n")
git("${super}/engine" ignored commit --quiet -am two)
git("${super}/engine" second_commit rev-parse HEAD)

# The index records the first commit (the pin), and the folder is at the first commit
git("${super}/engine" ignored checkout --quiet ${first_commit})
git("${super}" ignored update-index --add --cacheinfo 160000,${first_commit},engine)

set(CASE_OPTIONS "")
check_case("at the recorded commit" "${super}" ok "is at the recorded commit")

# The folder is at another commit: the guard stops, and the message names the folder, both commits, and the two ways out
git("${super}/engine" ignored checkout --quiet ${second_commit})
check_case("at another commit" "${super}" stop
	"The folder 'engine' is not at"
	"${first_commit}" "${second_commit}"
	"git submodule update engine" "git add engine" "-DXRSIM_ALLOW_UNPINNED_SUBMODULES=ON")

# The option turns the stop into a warning that says the same
set(CASE_OPTIONS -DXRSIM_ALLOW_UNPINNED_SUBMODULES=ON)
check_case("at another commit with the option" "${super}" ok
	"CMake Warning" "The folder 'engine' is not at"
	"${first_commit}" "${second_commit}" "Continuing because XRSIM_ALLOW_UNPINNED_SUBMODULES is ON")
set(CASE_OPTIONS "")

# A new pin in the index (git add engine) is a pin: the folder is at the recorded commit again
git("${super}" ignored update-index --cacheinfo 160000,${second_commit},engine)
check_case("with a new pin in the index" "${super}" ok "is at the recorded commit")

# The situations where the check is skipped: it must never stop the configuration
set(CASE_OPTIONS -DXRSIM_PIN_GUARD_GIT_EXECUTABLE=)
check_case("without Git" "${super}" ok "skipped: Git was not found")
set(CASE_OPTIONS "")

file(MAKE_DIRECTORY "${WORK_DIR}/archive/engine")
check_case("not a Git checkout" "${WORK_DIR}/archive" ok "skipped: the sources are not a Git checkout")

make_repository("${WORK_DIR}/not_fetched")
file(MAKE_DIRECTORY "${WORK_DIR}/not_fetched/engine")
git("${WORK_DIR}/not_fetched" ignored update-index --add --cacheinfo 160000,${first_commit},engine)
check_case("folder not fetched" "${WORK_DIR}/not_fetched" ok "skipped: the folder is not a checkout of its own")

make_repository("${WORK_DIR}/not_recorded")
make_repository("${WORK_DIR}/not_recorded/engine")
check_case("no commit recorded" "${WORK_DIR}/not_recorded" ok "skipped: the index of the repository records no commit for it")

get_property(failed GLOBAL PROPERTY PIN_GUARD_TEST_FAILED)
if(failed)
	message(FATAL_ERROR "[ERROR::SUBMODULE_PIN_GUARD_TEST] at least one case failed")
endif()

message(STATUS "[SUCCESS::SUBMODULE_PIN_GUARD_TEST] The pin guard stops on a folder at another commit, warns with the option, and skips when it cannot check")
