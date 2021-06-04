cmake_minimum_required(VERSION 3.9)

project(m17cxx
    VERSION 0.1
    DESCRIPTION "M17 Digital Voice modulation and demodulation"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
    message(STATUS "Build type not specified: defaulting to release.")
endif()

# Check for dependencies
message(STATUS "# Checking dependencies")

#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads REQUIRED)

include(FindPkgConfig)
include(GNUInstallDirs)

pkg_check_modules(CODEC2 REQUIRED codec2)
#set(Boost_USE_STATIC_LIBS FALSE)
#find_package(Boost COMPONENTS program_options REQUIRED)

# Add subdirectories
add_subdirectory(src)
#add_subdirectory(apps)

find_package(GTest)
if(GTEST_FOUND)
    enable_testing()
    add_subdirectory(tests)
endif()

# Setup installation
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/m17cxxConfigVersion.cmake"
    VERSION 0.1
    COMPATIBILITY AnyNewerVersion
    )

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/m17cxxConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/m17cxxConfig.cmake"
    INSTALL_DESTINATION lib/cmake/m17cxx
    )

install(EXPORT m17cxxTargets DESTINATION lib/cmake/m17cxx)
install(FILES "${PROJECT_BINARY_DIR}/m17cxxConfigVersion.cmake"
              "${PROJECT_BINARY_DIR}/m17cxxConfig.cmake"
        DESTINATION lib/cmake/m17cxx)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)

# Create uninstall target
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    @ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )

# Print summary
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building version: ${PROJECT_VERSION}")
