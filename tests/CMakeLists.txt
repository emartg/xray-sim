## CMakeLists.txt for the tests of the libraries of the simulator

# Test of the adapter of gVirtualXRay: it needs no OpenGL context, so it runs on any machine
add_executable(Gvxr_Adapter_Test xray/Gvxr_AdapterTest.cpp)
target_link_libraries(Gvxr_Adapter_Test PRIVATE XRSim::Xray)

# Copy the runtime dependencies (DLLs) beside the executable (MinGW only, see the function of the Engine)
engine_stage_runtime_dependencies(Gvxr_Adapter_Test)

# The executables of the simulator and of its tests are in the same folder and copy the same DLLs beside themselves. The
# DLL of xraylib is copied once, by a target (see src/CMakeLists.txt), and the test is built after the application, so that
# the copies of the DLLs of vcpkg do not run at the same time either: a copy that finds the file open by another one fails
# with "permission denied"
if(TARGET XRSim_Xraylib_Dll)
	add_dependencies(Gvxr_Adapter_Test XRSim_Xraylib_Dll)
endif()
add_dependencies(Gvxr_Adapter_Test XRSim)

add_test(NAME Gvxr_Adapter_Test COMMAND Gvxr_Adapter_Test)
set_tests_properties(Gvxr_Adapter_Test PROPERTIES
	LABELS unit
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 60
)
