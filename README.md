# LLTech: A C++ Middleware offered by (L)oSeal(L)'s (Tech)nology
Components included:
- Codec: A HW accerlated encode/decode library
- Graphic: A HW based cross-api graphic library (Still in-developement)

# How to Build
## Requirement
1. CMake >= 3.14.0
3. Intel Graphic Driver (For Media SDK)
3. GoogleTest >= 1.8.0 (Optional, for unit tests)
4. Python 3.6 (Optional, for coding style scripts)

## Build on Windows
Set environment `GTEST_ROOT` or `GTEST_DIR`.
```bash
set GTEST_ROOT=X:/GoogleTest/include
```

Check CMAKE options by:
```bash
mkdir build && pushd build
cmake .. -A x64 -DCMAKE_INSTALL_PREFIX=install -L
```

Select the components want to build:
```bash
cmake .. -DLL_BUILD_CODEC=ON -DLL_BUILD_GFX=ON
cmake --build . --config release --target install
popd
```

## Build on Linux
Not tested yet.

## Run Unit Tests
```bash
cd build
cmake --build . --config release --target ALL_BUILD
ctest -c release -j 8 --progress
```

# How to Contribute
TDB...

# How to Reuse

Typically, there are **2** ways to reuse a C++ project:

## By installing package

Build & install `lltech` project. This will generate a `ll-config.cmake` file under installation root. You can add the directory of `ll-config.cmake` to `LL_DIR` and use `find_package(LL)` in your cmake scripts, which will add cmake targets `LL::Codec`, `LL::Graphic`.

```cmake
set(LL_DIR /path/install/lltech)
find_package(LL QUIET)
if(LL_CODEC_FOUND)
  # ...
  target_link_libraries(... PRIVATE LL::Codec)
endif()
if(LL_GFX_FOUND)
  # ...
  target_link_libraries(... PRIVATE LL::Graphic)
endif()
```

## By building from source

You can add cm4nn as a git submodule of your project, and use `add_subdirectory(${LL_DIR}) `to include `lltech`, which will add cmake targets `ixr_codec`, `ixr_engine`.

```cmake
set(LL_DIR /path/source/lltech)
add_subdirectory(${LL_DIR})
target_link_libraries(... PRIVATE ixr_codec)
target_link_libraries(... PRIVATE ixr_engine)
```
