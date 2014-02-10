# dmscanlib

A C++ library decoding storage tubes etched with Data Matrix 2D barcodes. Normally, one or more
tubes are placed in a 96-Well microplates. An image of the microplate, containing the barcodes, 
is scanned with the flatbed scanner and then each tube is decoded. 

The library has an acompanying [JNI](http://en.wikipedia.org/wiki/Java_Native_Interface) API that 
is used by the Biobank application to link the contents of the storage tubes to specimens collected
from patients.

## Build with Makefile

The Makefile crates an executable that runs the tests. Tests were written using 
[Google Test](https://code.google.com/p/googletest/). See below for instructions on how to set up 
on your development environment. 

## Using Eclipse for development

You need Eclipse CDT. The following projects are configured for Eclipse:

* **Linux-Debug**

    This project runs the unit tests.
    
* **Linux-imageinfo**
    
    This project creates the template info files for the testing images.

* **Linux-sharedlib**
    
    This project creates the shared library that is used by Biobank.

## Running on Ubuntu (or Linux)

  - `libdmtx-dev`
  - `libopenthreads-dev`
  - `libgoogle-glog-dev`
  - `libgtest-dev`
  - `libgflags-dev`
  - `libopencv-dev`
  - `libconfig++-dev`

## JNI

The JNI class used to interface to dmscanlib is DmScanLibJni. To generate 
the C header file use the following commands:

```bash
cd to <scannerConfig_proj>/bin
javah -jni edu.ualberta.med.scannerconfig.dmscanlib.ScanLib
mv edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h <dmscanlib_proj>/src.
```   

Where **scannerConfig_proj** is the Java pluging project used by Biobank.
     
## Google Test

```bash
sudo apt-get install cmake 
cd /usr/src/gtest
sudo cmake .
sudo make
# copy or symlink libgtest.a and libgtest_main.a to your /usr/lib folder
sudo mv libg* /usr/lib/
```

Use `--gtest_filter=*` parameter to run a single test.

## Valgrind

  Use the following commands to check for memory leaks:

```bash
valgrind --track-origins=yes --tool=memcheck --leak-check=yes -v --show-reachable=yes --num-callers=10 Linux-Debug/dmscanlib --gtest_filter=TestDmScanLib.decodeFromInfo
```
