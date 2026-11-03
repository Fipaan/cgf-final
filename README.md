# Description

This is a 3D potion-making game written in [GLFW](https://www.glfw.org/)/C++, and using [Freetype](https://freetype.org/) for font rendering.

# Dependecies 

- [CMake](https://cmake.org/)
- C++ compiler
- GLFW (via CMake)
- Freetype (via CMake)

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
