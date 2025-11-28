# AlgoProject

Priority queue playground that currently includes a binary heap implementation plus placeholders for Fibonacci and Hollow heaps. The project builds a small console executable that demonstrates the `BinaryHeap` API.

## Requirements
- CMake 3.16+
- A C++17 compiler (Visual Studio 2019/2022, clang, or GCC)

## Configure & Build (Windows / Visual Studio)
```powershell
# from the repository root
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

## Configure & Build (Ninja or Makefiles)
```bash
mkdir -p build && cd build
cmake -G Ninja ..
ninja
```
Replace `Ninja` with `Unix Makefiles` if you prefer make.

## Run
After a successful build, run the executable from the build directory:
```powershell
./Release/AlgoProject.exe
```
Use `./AlgoProject` if you built with Ninja/Make. You can also use the shortcut script from the repo root:
```powershell
.
un
```
which internally calls `build.ps1`.

## Project Layout
- `src/main.cpp` — sample driver exercising the binary heap implementation.
- `src/*.cpp` — implementation files for each heap variant.
- `include/*.h(pp)` — public headers, interfaces, and helpers (`PriorityQueue.h`, `Timer.hpp`, etc.).
- `Data/` — sample datasets for future experiments.
- `build.ps1`, `run.cmd` — one-command configure/build/run helpers.

Feel free to expand the project by fleshing out the other heap implementations or swapping in different drivers/benchmarks.
