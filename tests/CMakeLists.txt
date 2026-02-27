add_executable(VoxelSimTests
    test_main.cpp          # custom entry point + file logger (Catch2::Catch2, no built-in main)
    test_allocator.cpp
    test_math.cpp
    test_compression.cpp
    test_taskscheduler.cpp
    test_registry.cpp
    test_chunk.cpp
    test_camera.cpp
    test_fluid.cpp         # fluid tick constants, pool stress, placement checks
    test_worldgen.cpp      # biome distribution, terrain heights, ore depth, caves, rivers
    ../src/Allocator.cpp
    ../src/Compression.cpp
    ../src/TaskScheduler.cpp
    ../src/Registry.cpp
    ../src/Chunk.cpp
    ../src/StructureGenerator.cpp
    ../src/Camera.cpp
)

target_include_directories(VoxelSimTests PRIVATE
    ../include
    ../external
    ${fastnoise_SOURCE_DIR}/Cpp
)

# Link Catch2 without its built-in main — we provide test_main.cpp
target_link_libraries(VoxelSimTests PRIVATE Catch2::Catch2)

# Compile-time constants injected into test_main.cpp
target_compile_definitions(VoxelSimTests PRIVATE
    VOXEL_SIM_VERSION="Beta-Dev"
    VOXEL_TEST_LOGS_DIR="${CMAKE_SOURCE_DIR}/logs"
)

# Register tests with CTest using Catch2 v3 discovery
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(VoxelSimTests)
