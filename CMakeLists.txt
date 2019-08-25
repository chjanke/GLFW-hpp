cmake_minimum_required(VERSION 3.9...3.15)
project(glfw-hpp VERSION 1.0.0 LANGUAGES CXX)

find_package(glfw3 3.3 REQUIRED)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
	${PROJECT_NAME} INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include/glfw-hpp>
	)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

add_library(glfwhpp::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} INTERFACE glfw)


install(
	TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
	INCLUDES DESTINATION include
)

install(
	EXPORT ${PROJECT_NAME}Targets
	FILE ${PROJECT_NAME}Targets.cmake
	NAMESPACE glfwhpp::
	DESTINATION lib/cmake/glfw-hpp
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	${PROJECT_NAME}Version.cmake
	VERSION ${PACKAGE_VERSION}
	COMPATIBILITY SameMajorVersion
)

install(
	FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/glfw-hppConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/glfw-hppVersion.cmake"
	DESTINATION lib/cmake/glfw-hpp
)

install(FILES include/glfw.hpp DESTINATION include/glfw-hpp)
