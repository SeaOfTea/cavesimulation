# A DirectX 10 Procedural Cave Generator

Overview
-
This is the source code for my dissertaion for my 3rd year at the University of Hull (April 2012)
The abstract for this project was to create a simple but effective cave generation technique using [Metaballs](https://en.wikipedia.org/wiki/Metaballs). The walls of the cave were texture mapped using Triplanar texturing, projecting 3 repeating 2D textures along the X,Y and Z axis and then blending them depending on the coordinates surface normal.

It is adapted from [NVidia's Metaballs Direct 3D samples](http://developer.download.nvidia.com/SDK/10.5/direct3d/samples.html#MetaBalls).

![](http://i.imgur.com/RqcOW76.jpg?1)

Installation and Compilation
------------
Unfortunately I cannot recompile this project using Visual Studio 2015 Community Edition, from what I remember it was written using Visual Studio 2008, which you might need to run this project. It also used the DirectX 10 SDK, which might be harder to install on newer versions of windows. However the previously compiled binaries ARE available in the bin/ directory. Start up Metaballs.exe to run the software.

The code is uploaded here for your reference. 

The code is mostly within the source/ directory.

Video
-
https://www.youtube.com/watch?v=1fwXb7K8Wfw
