cmake_minimum_required(VERSION 3.10)
project(vtb_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Custom DPDK Path ---
# Ensure this matches your install prefix
set(DPDK_PATH
    "/grid/dpdk/dpdk23/installed"
    CACHE PATH
    "Path to custom DPDK installation")

# --- 2. Project Root ---
get_filename_component(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# --- 3. PkgConfig Setup ---
find_package(PkgConfig REQUIRED)

# Force PkgConfig to look in custom install
set(ENV{PKG_CONFIG_PATH}
    "${DPDK_PATH}/lib/x86_64-linux-gnu/pkgconfig"
    ":${DPDK_PATH}/lib/pkgconfig")

# We use 'IMPORTED_TARGET' which is a modern CMake feature that
# bundles includes and library paths together automatically.
pkg_check_modules(DPDK REQUIRED libdpdk IMPORTED_TARGET)

# --- 4. Source Discovery ---
file(GLOB LIB_SRCS "${PROJ_ROOT}/src/*.cpp")
set(APP_SRC "${PROJ_ROOT}/vtb.cpp")

# --- 5. Define Executable ---
add_executable(vtb_app ${LIB_SRCS} ${APP_SRC})

# --- 6. Flags & Includes ---
target_include_directories(vtb_app PRIVATE "${PROJ_ROOT}/inc")

# --- 7. THE FIX: Link via the Imported Target ---
# Using PkgConfig::DPDK tells CMake to use the exact paths found in step 3.
target_link_libraries(vtb_app PRIVATE
    PkgConfig::DPDK
    pthread
    dl
    m
)

# --- 8. Output Configuration ---
set_target_properties(vtb_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJ_ROOT}"
)
