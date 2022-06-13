# 3D Printing and Haptic Rendering of Scalar Fields

This is the code for my master project which is divided into two parts: one that creates a 3D printable model from a hexplot visualization of a 2D scalar field and one that uses a haptic force-feedback device to render a 2D scalar field. Both parts are implemented in the same project which is an extension of the code for the *Honeycomb Plots* paper by Trautner et al. [[1]](#1). The different approaches have been implemented as different "views", which you can switch between using <kbd>Ctrl</kbd>+<kbd>←</kbd>/<kbd>→</kbd> or going to "*settings*" and then clicking the "*switch to ...*" button.

## Views
 - Honeycomb Plot 2D view - This is the view from the previous technique by Trautner et al. [[1]](#1) which takes a CSV file of bivariate scalar data (x and y values) as input and creates a Honeycomb plot
 - 3D Printing view - Takes the *Honeycomb Plot 2D view* as input and constructs a 3D model that can be exported as an STL.
 - Haptic Rendering - Takes the *Honeycomb Plot 2D view* or a grayscale heightmap as input and uses a force-feedback to haptically render it.

## Setup

### Prerequisites

- [CMake](https://cmake.org)
- [Git](https://git-scm.com)
- Any C++20 compatible compiler (Microsoft Visual Studio 2019, GCC 11, Clang 14)
- A graphics card capable of running OpenGL 4.5
- For the haptic rendering part:
  - [Force Dimension SDK](https://www.forcedimension.com/software/sdk) - the environment variable `DHD_HOME` should point to the root installation directory of the SDK (see [config/cmake-modules/FindDHD.cmake](./config/cmake-modules/FindDHD.cmake))
  - A compatible force-feedback haptics device (3D Systems Phantom lineup, Force Dimension Omega, Novint Falcon, e.t.c.)

### Option 1
 - Build CMake project with option `AUTO_FETCH_AND_BUILD_DEPENDENCIES` set to ON:
   - `mkdir build; cd build`
   - `cmake .. -DAUTO_FETCH_AND_BUILD_DEPENDENCIES=ON; cmake --build . --config Release`

### Option 2
#### Windows
- Open a shell in the directory and run `./config/fetch-libs.cmd` to download all dependencies.
- Run `./config/build-libs.cmd` to build the dependencies.
- Run `./config/copy-libs.cmd` to copy runtime dependencies to binary directory
- Either open the root folder as a CMake project in an IDE or manually build from command line:
  - *(Optional: )* Make a build folder and navigate to it: `mkdir build; cd build`
  - Build with CMake: `cmake ..; cmake --build . --config Debug`

### Linux
 - Fetch the libraries in a similar way to the fetch-libs.cmd (just git clone them all)
 - Run `./linux-build-libs.sh`
 - Finally, build project or open in IDE similar to in the Windows approach

## 3D Printing

Most of the logic for this pipeline happens inside the `CrystalRenderer` class which generates the geometry and visualizes a 3D preview. After suitable results are achieved, the `STLExporter` class can finally export the model as a printable STL file (<kbd>Ctrl</kbd>+<kbd>S</kbd>) which can be run through a slicer software to generate toolpaths for a specific 3D printer which can then be read and printed by the printer.

### How it works
After a 2D visualization of the data is finished, a compute shader generates a 3D copy by displacing each hexagon by the binned value in the z-axis and connecting every hexagon to its neighbor via a quad. After this new surface has been created, the surface is then sent to a background CPU thread where a convex hull around each non-empty hexagon is calculated. This convex hull is then used by another compute shader which culls the hexagons outside of the hull and extrudes a surface from the boundary-line created from the convex hull down to become the base of the model. After that, additional compute shaders are run to create more geometry and transform the model. Finally, the model is rendered and sent back to the CPU where its OpenGL vertices and triangles are exported as an STL file.

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

## Haptic Rendering
The haptic rendering view renders a two-dimensional scalar field on a force-feedback haptic device compatible with the [Force Dimension SDK](https://www.forcedimension.com/software/sdk) connected to the computer via forces simulating the surface. As mentioned in [Views](#views), you can view the scalar field output of the Honeycomb Plot View by opening a CSV file, or in this view, you can directly view a scalar field by opening a grayscale heightmap image. Load a file by going to **File** -> **Open File** or <kbd>Ctrl</kbd>+<kbd>O</kbd>.

### How it works
The haptic rendering constructs a surface by using the scalar field as a displacement map for a plane and then conveys the sum of 3 physical forces through the force-feedback device: the normal force, the friction force and the gravity force.

The position of the haptic device in the virtual environment is projected down onto the surface to find the texture coordinate on the surface and the height. If the height is negative ($h<0$) a normal force is applied to push the haptic device out from the surface. The normal force is calculated by finding the normal vector of the tangent plane at the heightfield position, similar to bump mapping, and multiplying it by a user-set force amount interpolated into the surface ($s_f$). The tangent plane normal is calculated using the partial derivatives found by taking the central differences.

Dry friction is calculated by placing a *sticktion point* when the device first enters the surface, and then constantly dragging the user towards that point while also moving the point around to simulate dynamic friction. Gravity is a constant down-pulling force.

A volume representation is also created to give wind-like forces that display a course overview of the surface, enabling users to view high-level details by moving over the surface. The volume is built similar to a mipmap pyramid by stacking progressively smoothed surfaces on top of each other and trilinearly interpolate between the scalar values.

## References
<a id="1">[1]</a> T. Trautner, M. Sbardellati and S. Bruckner, "Honeycomb Plots: Enhancing Hexagonal Binning Plots with Spatialization Cues", *Eurographics Conference on Visualization (EuroVis) 2022*, 2022 (Note: Paper is not yet released as of 13/6/22)