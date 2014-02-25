# LIDARgl #

OpenGL-based LIDAR simulator

* [Applied Space Exploration Laboratory](http://asel.mae.wvu.edu/)
* [West Virginia Robotic Technology Center](http://wvrtc.com/)

## Description ##

LIDARgl is an extremely simple simulator for LIDAR, written in C++, using assimp and OpenGL.

## Installation ##

Pull from the repository, then you'll want to do `make`. Make sure you have OpenGL, the assimp library, and ImageMagick
(including Magick++) installed. This has only been tested under clang before, but should work just fine in GNU GCC as well.

## Usage ##

  ./lidargl n20r4_d5.obj scale

## License ##

Copyright 2014, John O. Woods, Ph.D.; West Virginia University Applied Space Exploration Laboratory; and West Virginia
Robotic Technology Center. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.