# Description

This is a 3D potion-making game written in [GLFW](https://www.glfw.org/)/C++, and using [Freetype](https://freetype.org/) for font rendering.

# Dependecies 

## Overall

### Required

- C++ compiler
    * Windows: [MSYS2 UCRT64](https://www.msys2.org/) clang++
    * Linux:   [clang++](https://clang.llvm.org/)/[g++](https://gcc.gnu.org/)
- [CMake v3.20](https://cmake.org/)

### Does not require to do anything

- [GLFW v3.4](https://www.glfw.org/) (via CMake)
- [glad v0.1.36](https://gen.glad.sh/) (generated)
- [Freetype v2.13.3](https://freetype.org/) (via CMake)
- [fast_obj v1.3](https://github.com/thisistherk/fast_obj) (embedded)
- [STB](https://github.com/nothings/stb) family header-only libraries (all embedded)
    * [stb_image.h v2.32](https://github.com/nothings/stb/blob/master/stb_image.h)

## Windows Toolchain

- Toolchain:     MSYS2 UCRT64
- Compiler:      Clang 22.1.7
- C++ Standard:  C++23
- CMake:         >3.20
- Generator:     Ninja

# Build & Run

## Note

- Freetype has some deprecated features in cmake, so warning is going to be displayed.

## Linux

```sh
$ cmake -B build/ -DCMAKE_BUILD_TYPE=Release
$ cmake --build build/
$ ./build/cgf
```

## Windows

```sh
$ cmake -B build\ -DCMAKE_BUILD_TYPE=Release
$ cmake --build build\
$ ./build/cgf.exe
```

## Build (Debug) & Run

## Linux

```sh
$ cmake -B build/ -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build/
$ ./build/cgf
```

## Windows

```sh
$ cmake -B build\ -DCMAKE_BUILD_TYPE=Debug
$ cmake --build build\
$ .\build\cgf.exe
```