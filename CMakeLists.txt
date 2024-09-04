cmake_minimum_required(VERSION 3.14...3.29)

project(minimumError VERSION 0.0.1 LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    include(CTest)
endif()

set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(
        STATUS
        "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")

    set(
        CMAKE_BUILD_TYPE
        "${DEFAULT_BUILD_TYPE}"
        CACHE STRING
        "Choose the type of build."
        FORCE)

    set_property(
        CACHE
        CMAKE_BUILD_TYPE PROPERTY
        STRINGS
        "Debug"
        "Release"
        "MinSizeRel"
        "RelWithDebInfo")
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(main main.cpp)
target_compile_definitions(main PRIVATE NOMINMAX=1)
target_compile_features(main PRIVATE cxx_std_23)
target_link_libraries(
    main
    PRIVATE
    glfw
    Vulkan::Vulkan)