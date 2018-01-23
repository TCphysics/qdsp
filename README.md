# QDSP

## About

QDSP, short for "Quick Dynamic Scatter Plot" (or alternatively, "Quick and Dirty
Scatter Plot") is a lightweight C library for creating dynamic, real-time
scatter plots. On decent hardware, it should be able to render 10^6 points every
frame. QDSP was originally created to render phase plots for particle-in-cell
simulations.

QDSP uses semantic versioning, so any changes made after 1.0.0 will be
backwards-compatible (until I realize that I've made an unforgivable design
decision and end up releasing 2.0.0).

## Installation

So far, QDSP is only ported to Linux. There's a good chance that it will work on
Mac, but I haven't tested it. At some point in the future, I'll redo the build
system in CMake and make sure it runs on everything.

QDSP requires the following dependencies, which should be available in most
package repositories:

* [GLFW 3](http://www.glfw.org/docs/latest)
* [SOIL](http://www.lonesock.net/soil.html)
* [ImageMagick](http://www.imagemagick.org/script/index.php)

In addition, [Doxygen](http://www.doxygen.org) is required to generate easily
readable documentation.

To install, just run:

    $ make
    $ sudo make install
    $ sudo ldconfig

After building, this will install the shared library to `/usr/local/lib`, the
header to `/usr/local/include`, and resources to `/usr/local/share/qdsp`. You
may need to add `/usr/local/lib` to your system's ldconfig path.

## Usage

Just put `#include <qdsp.h>` in your code and link with -lqdsp.

## Documentation

Documentation can be generated by `cd`ing into the `docs` directory and running
`doxygen`.

Look at the file `src/example1.c` for a basic example of how to use QDSP (run
`make example1` if you want to try it out). You can also look at example 2,
which uses QDSP to render the phase plot of a 1D PIC simulation.

## License

QDSP is licensed under the LGPL. This means you can use it for pretty much
anything, but a modified version of the library itself can only be distributed
under a compatible license. For more information, see the `LICENSE` file.

## Author

QDSP was primarily developed by Matt Mitchell. Please direct all questions,
comments, and concerns to <matthew.s.mitchell@colorado.edu>.

## Contributing

Want to help improve QDSP? See `CONTRIBUTING.md` for more information.

## Acknowledgements

Thanks to Matt Miecnikowski for adding the Fortran bindings.
