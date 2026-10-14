## CMakeLists.txt for the tests of the libraries of the simulator

# Test of the adapter of gVirtualXRay: it needs no OpenGL context, so it runs on any machine
add_executable(Gvxr_Adapter_Test xray/Gvxr_AdapterTest.cpp)
target_link_libraries(Gvxr_Adapter_Test PRIVATE XRSim::Xray)

# Copy the runtime dependencies (DLLs) beside the executable (MinGW only, see the function of the Engine), and the DLL
# of xraylib, which is a shared library on Windows (it is bundled with gVirtualXRay)
engine_stage_runtime_dependencies(Gvxr_Adapter_Test)
if(WIN32 AND XRAYLIB_SHARED_LIB_PATH)
	add_custom_command(TARGET Gvxr_Adapter_Test POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${XRAYLIB_SHARED_LIB_PATH} $<TARGET_FILE_DIR:Gvxr_Adapter_Test>)
endif()

add_test(NAME Gvxr_Adapter_Test COMMAND Gvxr_Adapter_Test)
set_tests_properties(Gvxr_Adapter_Test PROPERTIES
	LABELS unit
	FAIL_REGULAR_EXPRESSION "[[]ERROR"
	TIMEOUT 60
)
