## Pre-requisites

* Git ([Download](https://git-scm.com/downloads))
* CMake 3.12 or higher ([Download](https://cmake.org/download/))
* A CMake compatible development environment
  * Visual Studio 2019 ([Download](https://visualstudio.microsoft.com/downloads/))
* Vulkan SDK ([Download](https://vulkan.lunarg.com/sdk/home))

## Quick Start

* Clone this repository with the recursive flag
  * `git clone https://github.com/jherico/OpenXRSamples.git --recursive`
  * `cd OpenXRSamples`
  * If you've already cloned without the recursive flag, you can fetch the submodules using the commands
    * `git submodule init`
    * `git submodule update`
* Create a build directory
  * `mkdir build`
  * `cd build`
* Configure using cmake (force 64-bit architecture)
  * `cmake .. -A x64`
  * This step can take a while the first time, since it will automatically download and build all the dependencies
* Build the example
  * `cmake --build . --config Debug --target sdl2_gl_single_file_example`

After those steps, if you haven't encountered any errors, there should be `bin_debug` dir in the main repository dir.  This should contain and executable `sdl2_gl_single_file_example`.  Running it should display an on screen window divided between green and blue and in the HMD should also display green in the left eye and blue in the right eye.


