# Description

This is a 3D potion-making game written in [GLFW](https://www.glfw.org/)/C++, and using [Freetype](https://freetype.org/) for font rendering.

# Dependecies 

## Overall

- [CMake](https://cmake.org/)
- C++ compiler
- [GLFW 3.4](https://www.glfw.org/) (via CMake)
- [Freetype 2.13.3](https://freetype.org/) (via CMake)

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

```sh

 ```