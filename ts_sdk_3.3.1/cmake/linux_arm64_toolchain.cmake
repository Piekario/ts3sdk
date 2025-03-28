set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(triple aarch64-linux-gnueabi)
set(CMAKE_C_COMPILER /usr/bin/clang-14)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER /usr/bin/clang++-14)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_SYSROOT /usr/aarch64-linux-gnu)
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=/usr/aarch64-linux-gnu/bin/ld" CACHE FILEPATH "" FORCE)
set(CMAKE_AR /usr/aarch64-linux-gnu/bin/ar CACHE FILEPATH "" FORCE)
