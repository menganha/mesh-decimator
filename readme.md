# Simple Mesh Decimator

This project implements a mesh decimation algorithm based on the seminal paper “Surface Simplification Using Quadric Error Metrics” by Michael Garland and Paul S. Heckbert. The goal is to reduce the complexity of 3D meshes while preserving their overall shape and appearance.

<p align="center">
<img src="./demo.gif"  alt=""/>
</p>

## Features

* Quadric Error Metrics (QEM): Efficient edge-collapse based simplification.
* Linux-only support: Currently tested and supported on Linux systems.
* Dependencies: Requires GLFW and GLM libraries installed on the host machine.
* Custom utilities: Includes a bump allocator and bespoke container implementations as alternatives to certain STL structures, showcasing additional programming techniques.

## Build & Run

1. Build: Run make in the project root. Compiled binaries will be placed in the build/ directory.

2. Run: Execute the binary with an .obj file as input:
    ```bash
    ./build/release/decimator path/to/model.obj
    ```
    Alternatively, use one of the sample models provided in the models/ directory.

3. Profiling: An auxiliary script run.sh is included for profiling and performance analysis.

## Usage

* Press d to decimate the mesh.
* Each decimation step performs 200 iterations, with each iteration collapsing a single edge.
* This progressively reduces mesh complexity while maintaining geometric fidelity.

