# Minimum for GameNetworkingSockets
cmake_minimum_required(VERSION 3.15)

# Uses C and C++
project(CushiNet LANGUAGES C CXX)

# Generate compile_commands.json (useful for IDEs)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# GameNetworkingSockets Dependencies
set(GAME_NETWORKING_SOCKETS_VERSION 1.6.0)
find_package(GameNetworkingSockets ${GAME_NETWORKING_SOCKETS_VERSION} QUIET)
if (NOT GameNetworkingSockets_FOUND)
  include(FetchContent)
  FetchContent_Declare(
      GameNetworkingSockets
      DOWNLOAD_EXTRACT_TIMESTAMP OFF
      URL https://github.com/ValveSoftware/GameNetworkingSockets/archive/refs/tags/v${GAME_NETWORKING_SOCKETS_VERSION}.tar.gz
  )
  FetchContent_GetProperties(GameNetworkingSockets)
  if (NOT GameNetworkingSockets_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(GameNetworkingSockets)
  endif()
endif()

# Gather all .cpp files inside src/ and its subdirectories
file(GLOB_RECURSE libsrc CONFIGURE_DEPENDS "src/*.cpp")

# Gather all .h files inside include/ and its subdirectories
file(GLOB_RECURSE libheaders CONFIGURE_DEPENDS "include/**/*.h")

# Create the library (Static or Shared based on BUILD_SHARED_LIBS)
add_library(${PROJECT_NAME} ${libsrc} ${libheaders})

# Create a namespace alias
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Bind C++17
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Expose the include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link GameNetworkingSockets to CushiNet
target_link_libraries(${PROJECT_NAME} PUBLIC GameNetworkingSockets)

# ================================================== #
# Allow users to run `make install` and use `find_package(CushiNet)`
# ================================================== #

include(GNUInstallDirs)

# Install the library binaries
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export the CMake targets so find_package works
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
