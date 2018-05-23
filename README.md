# Introduction
A tiny utility that converts BMP images into uncompressed PNG images.
The executable binary is only 4,096 bytes with no external dependencies and is compatible with all Win32/64 platforms but Win32s.  

Currently, it supports only 1-bpp, 4-bpp (RGB/RLE), 8-bpp (RGB/RLE), 24-bpp and 32-bpp (ARGB) bottom-up images.
And it probably relies on some CPU instruction sets that wasn't available when Windows 95/NT3.x came out.

## Usage
Just drag & drop files into the .EXE file.

To compress output files, use
[some](http://optipng.sourceforge.net/)
[free](https://pmt.sourceforge.io/pngcrush/)
[recompression](http://www.advsys.net/ken/utils.htm)
[software](http://www.advancemame.it/comp-readme).

## Todos
- [ ] Support minor BMP image format: BitmapCoreHeader, 16-bpp, top-down, bitfields
- [ ] Test on [Wine](https://www.winehq.org/) on some Linux distro
- [ ] Make CLI version that runs on non-Windows platforms

## License
Released under the [WTFPL](http://www.wtfpl.net/about/).
