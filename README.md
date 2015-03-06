# GLIDAR #

A simple OpenGL-based LIDAR simulator

* [John O. Woods, Ph.D](http://github.com/mohawkjohn) &mdash; john.o.woods@gmail.com
* [WVU Applied Space Exploration Laboratory](http://asel.mae.wvu.edu/)
* [West Virginia Robotic Technology Center](http://wvrtc.com/)
* [NASA GSFC Satellite Servicing Capabilities Office](http://ssco.gsfc.nasa.gov/)

## Description ##

GLIDAR is an extremely simple OpenGL-based LIDAR simulator, written in
C++. It uses GLSL shaders to produce range images of 3D models, which
can be used to test pose determination algorithms.

The simulated range images are a first-order approximation of what a
real LIDAR sensor would observe. Noise can be added in the fragment
shader, and basic textures are supported (bump maps are not yet
available).

Note that GLIDAR does not model a specific sensor system. However, it
can be easily modified to model a sensor.

GLIDAR was written by John O. Woods, Ph.D., who works for the West
Virginia University Applied Space Exploration Laboratory, under
contract with NASA Goddard Space Flight Center's Satellite Servicing
Capabilities Office.

## Requirements ##

GLIDAR has been tested on Mac OS X Mavericks and Yosemite
installations with Homebrew, and on a couple of Ubuntu (Trusty and
later) machine. It has the following requirements:

* CMake
* GLSL 1.2* support (in graphics card)
* GLFW 3
* GLEW 1.10
* GLM 0.9.6+
* Magick++ 6
* [ASSIMP](http://assimp.sourceforge.net/)*
* ZeroMQ 4
* Any version of PCL*


Items with asterisks are pretty much essential, but you might be able
to get the code working with older versions of ImageMagick/Magick++.

There's an older version of the software which uses a regular Makefile
instead of CMake, so it's also possible &mdash; but inadvisable
&mdash; to use GLIDAR without CMake.

ZeroMQ is needed if you want to publish or subscribe to point
clouds. I used to have this as an optional dependency, but it became
too difficult to maintain all of the different `#ifdef` macros. ZeroMQ
is not hard to install, but if you can't get it for your operating
system, you can probably just strip the ZeroMQ-related lines out of
GLIDAR and write your own.

Finally, note that the PCL requirement is essentially just for command
line argument parsing. PCL makes this super easy, but PCL also takes
forever to install. If you would like to implement a different command
line argument parsing specification in lieu of PCL, please feel free
to submit a patch.

### Dependency Installation ###

If you're using Ubuntu, you may need to go and find the correct
version of GLFW (I think it's libglfw3-dev or something like that) and
install it by hand (using `dpkg`).

For those who are Mac OS X users, I recommend using homebrew to
install any dependencies.

## Installation ##

First, you need a local copy of the repository:

    git clone https://github.com/wvu-asel/glidar.git
    cd glidar
    
An in-tree build is not advised. As such, you should create a `build`
subdirectory and `cd` into it, then tell `cmake` where to find the
source tree.
    
    mkdir build
    cd build
    cmake ..
    
If cmake reported no error messages, you can now build:

    make
    
It is *not* recommended that GLIDAR be installed using `make install`.
GLIDAR expects certain resources (e.g., models and textures) in its
source tree.

## Usage ##

GLIDAR should be run in the root of its source tree (not from the
`build/` directory), via command line.

### Example Usage: Linux/Unix ###

Without publishing:

    build/glidar models/bunny.ply --scale 0.24 --model-dr 0.1,0.01,0 --model-r 0,0,0 --camera-z 1000 -w 256 -h 256 --fov 20

With publishing:

    build/glidar models/bunny.ply --scale 0.24 --model-dr 0.1,0.01,0 --model-r 0,0,0 --camera-z 1000 -w 256 -h 256 --fov 20 -p 65431 --pub-rate 15 --subscribers 1

### Running on Mac OS X ###

Run it exactly the same way as in Linux, but instead of using
`build/glidar` as the binary, use
`build/glidar.app/Contents/MacOS/glidar`. Having it packaged in this
way helps the program to interact with Finder.

### Command-Line Arguments ###

The first argument, the model filename, is mandatory. Additional
arguments are also accepted:

* `--scale`: decimal value by which the model should be scaled (default: 1.0)
* `--dr`: rotation rate about _x_, _y_, and _z_ axes respectively (default: 0)
* `--r`: initial model rotation about _x_, _y_, and _z_ axes respectively (default: 0)
* `--camera-z`: sensor's initial distance from the object, typically in meters (default: 1000)
* `--width`, `--height`: sensor resolution (default: 256)
* `--fov`: sensor field-of-view (default: 20 degrees)
* `--pcd`: a file basename (without extension) can be provided for saving the initial image as a PCD
* `--port`: the port to publish to, if ZeroMQ is included (if not given, will not be run in server mode)
* `--subscribers`: the number of subscribers to wait for before beginning to publish
* `--pub-rate`: if publishing, how many render cycles should pass between point cloud publications (default: 15)

Note that if an option is provided for `--pcd`, the rotation rates
should be set to 0. This option will produce two outputs, namely
`filename.pcd` and `filename.transform` files. These are useful in
scripting.

If you are not scripting, you may wish to examine the image before
saving it. This can be accomplished using the `s` key, which will
produce files `buffer.pcd` and `buffer.transform` (and overwrite any
existing files with the same name).

If you wish to publish to a port (if you provide a port option), it is
inadvisable to also supply a `--pcd` option. Additionally, if you are
publishing and you use the `s` key, unpredictable behavior may result.

### GUI ###

The GUI is very basic. You can use `s` as described above, and the `+`
and `-` keys will move the sensor towards and away from the
object. The box, which is 200 meters along each side, may help you
with establishing the appropriate object scale.

### Known Issues ###

The greatest limitation right now is in the ASSIMP library, which
loads the 3D models. I have not been able to get it to work with the
[full high-resolution International Space Station model](http://nasa3d.arc.nasa.gov/detail/iss-hi-res), for example
&mdash; but it should load individual modules of the ISS properly.

Other limitations:

* It takes a long time to write the range images to PCD files.
* The model can be rotated, and the sensor can be moved towards or away from the model; but the sensor itself cannot
  rotate or move in other directions. No sensor path can be programmed yet.
* Model bump textures are not currently utilized.

Improvements needed:

* When I first wrote GLIDAR, it used &mdash; almost exclusively
  &mdash; homogeneous transforms for arranging objects. Later, I added
  quaternion support, but the quaternion rendering is only implemented
  for pipelines that use ZeroMQ. It'd be extremely simple to implement
  non-pipeline rendering using quaternions &mdash; and that would
  enable us to remove some old and fragile code.

### Examples ###

Let's say you want to use the Zarya module of NASA's high-res ISS
scene. The Zarya Lightwave Object file is located in 
`ISS models 2011/Objects/Modules/MLM/MLM.lwo`. I calculated that the scale factor
for this should be about 0.024 if you want to work in meters, or 0.24
if you don't mind working in decimeters.

You could also try the `n20r4_d5.obj` fake asteroid, which is included
in the `models` subdirectory of the source tree.

## Bug Reports ##

If you find a bug, please file it in our [issues tracker](https://github.com/wvu-asel/glidar/issues).
  
## Contributing ##

Contributions are appreciated. We prefer that you submit them as pull requests to 
the [wvu-asel/glidar repository](https://github.com/wvu-asel/glidar). If you don't know how to use `git rebase`, please
feel free to ask.

## License ##

Copyright 2014 - 2015, John O. Woods, Ph.D.; West Virginia University Applied
Space Exploration Laboratory; and West Virginia Robotic Technology
Center. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
   disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation
are those of the authors and should not be interpreted as representing
official policies, either expressed or implied, of the FreeBSD
Project.
