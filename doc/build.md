To simplify the configuration of the build environment, the [configure](../clients/blackbox/util/debug/configure) script has been provided to generate a standard build environment. It may be necessary for you to regenerate the [dr-env-32](../clients/blackbox/util/debug/dr-env-32) script--see comments within the script for instructions. To build BlackBox on a fresh install of Windows 7:

1. Install Microsoft Visual Studio 2005-2012
2. Install [ninja](https://ninja-build.org/)
3. Setup your favorite unix-like terminal (e.g., [msys](http://www.mingw.org/wiki/MSYS) or [cygwin](https://www.cygwin.com/))
3. Create a build directory and `cd` there
4. Export environment variable `DYNAMORIO_SRC`, pointed to your blackbox source tree
5. `./configure`
6. `ninja`
7. Test the build: `bin32/drrun.exe -debug ls`

Note that although BlackBox runs in 32-bit mode, it can be build on 64-bit Windows.
