include(CMakeFindDependencyMacro)

find_dependency(glfw3 3.3 REQUIRED)

if(NOT TARGET glfwhpp::glfw-hpp)
	include("${CMAKE_CURRENT_LIST_DIR}/GLFW-hppTargets.cmake")
endif()
