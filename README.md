# molumes

## Setup

### Prerequisites

- [CMake](https://cmake.org)
- [Git](https://git-scm.com)
- Any C++20 compatible compiler (Microsoft Visual Studio 2019, GCC 11, Clang 14)
  - Uses std::format, which currently only MSVC supports (see https://en.cppreference.com/w/cpp/compiler_support/20). std::format can be replaced with [fmt](https://github.com/fmtlib/fmt) for compilers that don't support it.
- A graphics card capabable of running OpenGL 4.5

### Windows

- Open a shell and run `./fetch-libs.cmd` to download all dependencies.
- Run `./build-libs.cmd` to build the dependencies.
- Run `./copy-libs.cmd` to copy runtime dependencies to binary directory
- Either open the root folder as a CMake project in an IDE or manually build from command line:
  - *(Optional: )* Make a build folder and navigate to it: `mkdir build; cd build`
  - Build with CMake: `cmake ..; cmake --build . --config Debug`

### Linux
 - Fetch the libraries in a similar way to the fetch-libs.cmd (just git clone them all)
 - Run `./linux-build-libs.sh`
 - Finally, build project or open in IDE similar to in the Windows approach

## 3D Printing

This branch of the molumes project features a hexplot visualization -> 3d print pipeline. Most of the logic for the pipeline happens inside the `CrystalRenderer` class which generates the geometry and visualizes a 3D preview. After suitable results are achieved, the `STLExporter` class can finally export the model as a printable STL file which can be run through some slicer software to generate g-code for a specific printer, which can then be read and printed by the printer.

### How it works
After a 2D visualization of the data is finished, a compute shader generates a 3D copy by displacing each hexagon by the binned value in the z-axis and connecting every hexagon to it's neighbour via a quad. After this new surface has been created, the surface is then sent to a background CPU thread where a convex hull around each non-empty hexagon is calculated. This convex hull is then used by another compute shader which culls the hexagons outside of the hull and extrudes a surface from the boundary-line created from the convex hull down to become the base of the model. After that, additional compute shaders are run to create more geometry and transform the model. Finally the model is rendered, and sent back to the CPU where it's OpenGL vertices and triangles are exported as an STL file.

### Example
Here's a small example of how one could use the pipeline, from dataset to final print:

 1. As a first step, open the dataset in the application (<kbd>Ctrl</kbd> + <kbd>O</kbd>)
 2. Open the **Illuminated Point Plots** menu and tweak the settings to get some nice looking results
    - Note: In order to get regression planes later on, they have to be generated first by enabling **Render Tile Normals**.
 3. When results are nice looking, switch into the 3D view by going to **Settings** -> **Switch to 3D view** or <kbd>Ctrl</kbd> + <kbd>→</kbd>.
 4. Use the **Crystal** menu to change and tweak settings for the 3D geometry.
 5. When satisfied, export the model by going to **File** -> **Export File...** or <kbd>Ctrl</kbd> + <kbd>S</kbd>.
 6. Save the file as a *binary stl* (ascii stl is mostly for debugging but can be used by some slicers)
 7. In the slicer, import the STL and fix any geometry issues encountered.
    - Note: The pipeline doesn't completely fix the geometry in places where any slicer can trivially fix it. Things like filling holes and/or reorganizing triangles is left as work to the slicers.
 8. Slice the model and export as g-code (or whatever format your printer uses)
 9. Run the g-code on the printer and enjoy the result

### Notes on printing
 - As mentioned in the example above, the pipeline only generates needed geometry. Holes and triangles that can be trivially fixed are left to the slicer to fill (if needed). If the slicer can't fix them automatically, a good tips is to use the free version of [Autodesk Netfabb](https://www.autodesk.com/products/netfabb/overview) to fix it (open model in Netfabb, and click *repair pair*).
 - The final geometrical structure of the STL file is almost self supporting. So unless you are printing it very large, there's typically no need for much infill.
 - The model is always built standing up, so there's normally no need for additional supports. Except for the concave/mirrored models where it may be beneficial to have supports on the lower half of the model.