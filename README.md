# ![Luma HDRv](http://lumahdrv.org/img/logo_small.png) **Luma Hdrv**
Project web: http://lumahdrv.org/

## General
The [Luma HDRv](http://lumahdrv.org/) package provides libraries for
encoding and decoding of HDR video sequences. The encoding is done 
using the high bit depth profiles of google's [VP9 codec](http://www.webmproject.org).
To be able to store HDR images with the limited bits available, 
luminance and color transformations are done in such way to ensure 
that quantization errors are below the visibility threshold of the 
human visual system. This assumes that the input is given in physical 
units (cd/m^2).

For examples on how to use the libraries, please see the simple test
examples in `./test`.

The Luma HDRv package also provides command line applications for 
encoding, decoding and playback of HDR video using the libraries. 
The input video for encoding could either be provided as separate 
[OpenEXR](http://www.openexr.com) frames, or piped from [pfstools](http://pfstools.sourceforge.net).
The HDR video is stored in a [Matroska](http://www.matroska.org)
container, for increased flexibility and compatibility.

For further information on how to use the applications, please see
the manual pages provided with each application. After installation
these should be available on the system as:

```
$ man lumaenc
$ man lumadec
$ man lumaplay
```

### Comparison to HDR video formats
Luma HDRv provides capabilities to encode HDR video with a number of 
different settings (transfer function, color space, bit depth etc.). The 
default settings encodes luminance between 0.005 and 10000 cd/m^2 at 11
bits by means of the perceptual quantizer (PQ, SMPTE ST 2084) function, 
and colors at 8 bits/channel in the Lu'v' color space.

It is also possible to encode in a format that is compatible with existing
HDR video standards. The following encodes with PQ at 10 bits, and in the 
YCbCr color space according to recommendation ITU-R BT.2020, with luminance 
range limited to 0.01-1000 cd/m^2:

```
lumaenc \
	--input hdr_frames_%06d.exr \
	--frames 1:500 \
	--output hdr_video.mkv \
	--encoding-bitdepth 10 \
	--color-bitdepth 10 \
	--ptf-bitdepth 10 \
	--profile 2 \
	--transfer-function PQ \
	--color-space YCBCR \
	--pre-scaling 20 \
	--max-luminance 1000 \
	--min-luminance 0.01
```
Please see manual pages for explanation of the different parameters. Note that
the `--pre-scaling` option can be used to approximately calibrate the
input to physical units. The result of this encoding is equivalent to the HDR10
standard, which is the most widely supported HDR video format. This makes the 
video compatible with applications that support HDR10, and that can decode VP9 
codec stored in a Matroska container. It is for example possible to upload the 
result to [Youtube](https://www.youtube.com/) and [Vimeo](https://vimeo.com),
where it is properly recognized as HDR video.

## Included libraries and applications
Depending on what is available on the build system (see compilation 
and installation section), the Luma HDRv package is built with 
different executables. In a full build, the following parts are
included:

* **libluma_encoder** -- Luma HDRv encoding library, providing functionalities
                         for encoding and writing of HDR videos.
* **libluma_decoder** -- Luma HDRv decoding library, providing functionalities
                         for decoding and reading of HDR videos.
* **lumaenc**         -- HDR video encoding application, for encoding HDR 
                         frames into a HDR video.
* **lumadec**         -- HDR video decoding application, for decoding of
                         HDR video into separate frames.
* **lumaplay**        -- Simple playback of encoded HDR videos.
* **test_simple_enc** -- Minimal encoding test example, to demonstrate how
                         to use the **luma_encoder** library.
* **test_simple_dec** -- Minimal decoding test example, to demonstrate how
                         to use the **luma_decoder** library.

## Compilation and installation
Compilation is provided through CMake, and should be able to detect
which applications to build depending on existing dependencies.

For a minimal build, only providing the Luma HDRv libraries 
(**libluma_encoder** and **libluma_decoder**), there are no external 
dependencies. The required dependencies for these are included in the
build tree and compiled together with the Luma HDRv libraries. 

The provided applications are compiled if their dependencies are 
found. The dependencies are as follows:

* **libluma_encoder** and **libluma_decoder**:
   * [libvpx](http://www.webmproject.org) [provided]
   * [libmatroska](http://www.matroska.org) [provided]
   * [libebml](http://matroska-org.github.io/libebml) [provided]

* **lumaenc** and **lumadec**:
   * [openEXR](http://www.openexr.com/)
   * [pfstools](http://pfstools.sourceforge.net) [optional]

* **lumaplay**:
   * [openGL](https://www.opengl.org/)
   * [glut](https://www.opengl.org/resources/libraries/glut/)
   * [glew](http://glew.sourceforge.net/)

* **test_simple_enc** and **test_simple_dec**:
   * [openEXR](http://www.openexr.com/)

#### UNIX
For an out-of-tree build (recommended) and install, do the
following:

```
$ cd <path_to_luma_hdrv>
$ mkdir build
$ cd build
$ cmake ../
$ make
$ make install
```

#### WINDOWS
Since libvpx requires [Cygwin](https://www.cygwin.com) to be built, this must be done 
before compilation of Luma HDRv. A quick guide for installation
of libvpx for linking with Luma HDRv follows (please refer to
instructions on the [WebM webpage](http://www.webmproject.org/code/build-prerequisites)
for more information on Windows compilation of libvpx):

1. Extract the provided libvpx archive (lib/libvpx.tar.gz) in
   place, or [download libvpx](https://github.com/webmproject/libvpx/) and put under lib/libvpx.

2. In Cygwin, go to the build directory of libvpx and build a
   Visual Studio solution:
    ```
    $ cd /cygdrive/<path_to_luma_hdrv>/lib/libvpx/build
    $ ../configure --enable-vp9-highbitdepth --enable-vp9 --enable-multithread
        --as=yasm --disable-avx2 --disable-libyuv --disable-webm_io
        --disable-unit_tests --disable-install-docs --disable-install-bins
        --disable-install-srcs --disable-examples --disable-docs --disable-vp8
        --target=x86-win32-vs10 --enable-static-msvcrt
    $ make
    ```

   The `--target` option should be replaced with the appropriate
   platform and Visual Studio version.

3. Download the [Yasm assembler](http://yasm.tortall.net/Download.html)
   executable (yasm-x.x.x-win32.exe or yasm-x.x.x-win64.exe), rename 
   it to `yasm.exe` and place it in the build directory (`lib/libvpx/build/`),
   or any directory known by Visual Studio.

4. Build the VPX Visual Studio solution, and place the compiled
   library (`vpxmt.lib`) either in the build directory of Luma HDRv
   or in the `lib` folder in the build directory.

5. Run CMake for Luma HDRv. It should be able to detect VPX
   library that has been created, and the include files in
   `lib/libvpx/`.

6. Build Luma HDRv Visual Studio solution, and then run the
   `INSTALL` project in the solution to install Luma HDRv.



## Included libraries

Minimal dependencies are included as is in `./lib`, and with no 
alterations. These can easily be replaced by other versions of the 
libraries, simply by replacing the source files and recompiling
Luma HDRv. The included libraries are:

 * [libvpx](http://www.webmproject.org) uses a BSD license.
 * [libmatroska](http://www.matroska.org) uses the LGPL license.
 * [libebml](http://matroska-org.github.io/libebml) uses the LGPL license.

Please see the respective source directories in `./lib` for license 
information.

## Known issues

The library does not work with libvpx3 provided in Ubuntu
16.04. Uninstall libvpx-dev and compile with the static version of
libvpx (provided in lib, and automatically used if libvpx cannot be found
in the system).

## License

Copyright (c) 2015, The LumaHDRv authors.
All rights reserved.

Luma HDRv is distributed under a BSD license. See `LICENSE` for information.

![Luma HDRv](http://lumahdrv.org/img/quote_bg.jpg)
