# GLIDAR #

A simple OpenGL-based LIDAR simulator

* [John O. Woods, Ph.D](http://github.com/mohawkjohn)
* [WVU Applied Space Exploration Laboratory](http://asel.mae.wvu.edu/)
* [West Virginia Robotic Technology Center](http://wvrtc.com/)
* [NASA GSFC Satellite Servicing Capabilities Office](http://ssco.gsfc.nasa.gov/)

## Description ##

GLIDAR is an extremely simple OpenGL-based LIDAR simulator, written in C++. It uses GLSL shaders to produce range images
of 3D models, which can be used to test pose determination algorithms.

The simulated range images are a first-order approximation of what a real LIDAR sensor would observe. Noise can be added
in the fragment shader, and basic textures are supported (bump maps are not yet available).

Note that GLIDAR does not model a specific sensor system. However, it can be easily modified to model a sensor.

GLIDAR was written by John O. Woods, Ph.D., who works for the West Virginia University Applied Space Exploration 
Laboratory, under contract with NASA Goddard Space Flight Center's Satellite Servicing Capabilities Office.

## Requirements ##

GLIDAR has been tested on a Mac OS X Mavericks installation with Homebrew, and on an Ubuntu (Trusty) machine. It has the
following requirements:

* CMake
* GLSL 1.2* support (in graphics card)
* GLFW 3
* GLEW 1.10
* Magick++ 6
* [ASSIMP](http://assimp.sourceforge.net/)*

Items with asterisks are pretty much essential, but you might be able to get the code working with older versions of
ImageMagick/Magick++ and GLFW.

There's an older version of the software which uses a regular Makefile instead of CMake, so it's also possible -- but
inadvisable -- to use GLIDAR without CMake.

## Installation ##

First, you need a local copy of the repository:

    git clone https://github.com/wvu-asel/glidar.git
    cd glidar
    
An in-tree build is not advised. As such, you should create a `build` subdirectory and `cd` into it, then tell `cmake`
where to find the source tree.
    
    mkdir build
    cd build
    cmake ..
    
If cmake reported no error messages, you can now build:

    make
    
It is *not* recommended that GLIDAR be installed using `make install`. It expects certain resources in its source tree.

## Usage ##

GLIDAR should be run in the root of its source tree (not from the build directory), via command line.

### Linux/Unix ###

  build/glidar object_file scale drx dry drz rxi ryi rzi range xres yres fov [output_basename]
  
### Mac ###

  build/glidar.app/Contents/MacOS/glidar object_file scale drx dry drz rxi ryi rzi range xres yres fov [output_basename]

### Command-Line Arguments ###

* `object_file`: the 3D model to load (the only mandatory argument)
* `scale`: decimal value by which the model should be scaled (default: 1.0)
* `drx`, `dry`, `drz`: rotation rate about _x_, _y_, and _z_ axes respectively (default: 0)
* `rxi`, `ryi`, `rzi`: initial model rotation about _x_, _y_, and _z_ axes respectively (default: 0)
* `range`: sensor's initial distance from the object, typically in meters (default: 1000)
* `xres`, `yres`: sensor resolution (default: 256)
* `fov`: sensor field-of-view (default: 20 degrees)
* `output_basename`: a filename (without extension) can be provided for saving the initial image

Note that if an `output_basename` is provided, the rotation rates should be set to 0. This option will produce two
outputs, namely `output_basename.pcd` and `output_basename.transform` files. These are useful in scripting.

If you are not scripting, you may wish to examine the image before saving it. This can be accomplished using the `s` 
key, which will produce files `buffer.pcd` and `buffer.transform` (and overwrite any existing files with the same name).

### GUI ###

The GUI is very basic. You can use `s` as described above, and the `+` and `-` keys will move the sensor towards and
away from the object. The box, which is 200 meters along each side, may help you with establishing the appropriate
object scale.

### Known Issues ###

The greatest limitation right now is in the ASSIMP library, which loads the 3D models. I have not been able to get it to
work with the [full high-resolution International Space Station model](http://nasa3d.arc.nasa.gov/detail/iss-hi-res), 
for example -- but it should load individual modules of the ISS properly.

Other limitations:

* It takes a long time to write the range images to PCD files.
* The model can be rotated, and the sensor can be moved towards or away from the model; but the sensor itself cannot
  rotate or move in other directions. No sensor path can be programmed yet.
* Model bump textures are not currently utilized.  

### Examples ###

Let's say you want to use the Zarya module of NASA's high-res ISS scene. The Zarya Lightwave Object file is located in
`ISS models 2011/Objects/Modules/MLM/MLM.lwo`. I calculated that the scale factor for this should be about 0.024 if you
want to work in meters, or 0.24 if you don't mind working in decimeters.

You could also try the `n20r4_d5.obj` fake asteroid, which is included in the source tree.

## Bug Reports ##

If you find a bug, please file it in our [issues tracker](https://github.com/wvu-asel/glidar/issues).
  
## Contributing ##

Contributions are appreciated. We prefer that you submit them as pull requests to 
the [wvu-asel/glidar repository](https://github.com/wvu-asel/glidar). If you don't know how to use `git rebase`, please
feel free to ask.

## License ##

Copyright 2014, John O. Woods, Ph.D.; West Virginia University Applied Space Exploration Laboratory; and West Virginia 
Robotic Technology Center. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
   disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the authors and should not be 
interpreted as representing official policies, either expressed or implied, of the FreeBSD Project.