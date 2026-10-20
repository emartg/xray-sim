## Runs the pin guard once, for the test of the guard (see submodule_pin_guard_test.cmake):
## cmake -DGUARD_MODULE=<path of submodule_pins.cmake> -DSOURCE_DIR=<repository> [-DXRSIM_ALLOW_UNPINNED_SUBMODULES=ON] [-DXRSIM_PIN_GUARD_GIT_EXECUTABLE=] -P submodule_pin_guard_case.cmake

include("${GUARD_MODULE}")
xrsim_check_submodule_pin(engine SOURCE_DIR "${SOURCE_DIR}")
