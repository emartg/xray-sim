## CMakeLists.txt for the tests of the libraries of the simulator

# Test of the adapter of gVirtualXRay: it needs no OpenGL context, so it runs on any machine
add_executable(Gvxr_Adapter_Test xray/Gvxr_AdapterTest.cpp)
target_link_libraries(Gvxr_Adapter_Test PRIVATE XRSim::Xray)

add_test(NAME Gvxr_Adapter_Test COMMAND Gvxr_Adapter_Test)
set_tests_properties(Gvxr_Adapter_Test PROPERTIES
	LABELS unit
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 60
)
