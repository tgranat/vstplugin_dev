# GGConvolver
A simple VST3 plugin to load a single impulse response file.

Work in progress. IR file hardcoded value.

### Build

See https://steinbergmedia.github.io/vst3_doc/vstinterfaces/cmakeUse.html

Right now I'm using Visual Studio. To create a VS solution I run:
```
cmake -G"Visual Studio 16 2019" -Ax64 -DSMTG_MYPLUGINS_SRC_PATH=C:/<where I have my source code> ../
```


### WDL Convolution Engine

I'm using the WDL convolution engine from https://www.cockos.com/wdl/.

A subset of files copied into this repository from https://github.com/justinfrankel/WDL.

```
Copyright (C) 2005 and later Cockos Incorporated
    
    Portions copyright other contributors, see each source file for more information

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
``