fox-cairo
=========

Cairo/Pango device context for the FOX GUI Toolkit.

Installation
------------

First, ensure you have development packages (at least the header files)
for Cairo and Pango (pangocairo), and (of course) FOX-1.6.

Suggest the following autoconfigure procedure:
```
cd <base directory>
mkdir build-lin
cd build-lin
../configure --enable-debug
make
sudo make install
```
Having a build sub-directory (build-lin) is very handy if you 
use git or plan to contribute patches etc., since the distro directory
remains free of cruft.


Compiling Your Applications
---------------------------

At the top of relevant source files:
```
#include "xincs-cairo.h"	// Include cairo and pango headers (optional)
#include "fx.h"			// Normal FOX includes
#include "FXDCCairo.h"		// This project
```

Compile using the following options:
```
gcc ... `fox-cairo-config --cflags` ...
```
Link using:
```
gcc ... `fox-cairo-config --libs` ...
```
(Obviously, add any other flags normally used with FOX etc.)
fox-cairo-config will output the -I and -L flags which will include not only
this project, but also the Cairo and Pango locations.


FXDCCairo Usage
---------------

Basically, you should be able to use FXDCCairo in place of FXDCWindow.
If you do, then you will get (hopefully) superior rendering of the
usual graphics.  In addition, text layout is simpler.

Not everything is exactly the same.  Read the top of FXDCCairo.h for some
caveats.


TODO
----

FXDCCairoEx - extension of FXDCCairo
FXVectorImage
FXVectorIcon - classes for scalable, vector-based graphics.

