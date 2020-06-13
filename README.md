
# Audacity

## Build

| [Workflow](../../actions) | Status |
|------------|------------|
|   [Windows](../../actions?query=workflow%3AWindows)  | ![Windows](../../workflows/Windows/badge.svg) |
|   [Ubuntu](../../actions?query=workflow%3AUbuntu)    | ![Ubuntu](../../workflows/Ubuntu/badge.svg) |
|   [MacOS](../../actions?query=workflow%3AMacOS)    | ![MacOS](../../workflows/MacOS/badge.svg) |
| [Audacity mirror](../../actions?query=workflow%3A"Build+Audacity")  | ![Build Audacity](../../workflows/Build%20Audacity/badge.svg?branch=master) |
| [Audacity upstream](https://github.com/audacity/audacity/actions?query=workflow%3A"CMake+Build"+branch%3Amaster) | ![CMake Build](https://github.com/audacity/audacity/workflows/CMake%20Build/badge.svg?branch=master) |

### "x64" Branch

Windows "x64" branch binaries can be found in under **[Releases](../../releases)**.

The latest builds for Windows can be found here:

* **[x86](../../releases/tag/latest-x86)**
* **[x64](../../releases/tag/latest-x64)**
* **[x64-avx2](../../releases/tag/latest-x64-avx2)**
* **[arm64](../../releases/tag/latest-arm64)**

There are both MSI-based and EXE-based installers.  The latter makes
sure the runtime prerequisites are installed before installing.

To manually install the prerequisites, make sure a recent version of
the [Microsoft Visual C++ Redistributable for Visual Studio
2015-2019](https://www.visualstudio.com/downloads/)
is installed. If not already available, the installers can be
downloaded from:

* **[vc_redist.x86.exe](https://aka.ms/vs/16/release/VC_redist.x86.exe)** (32-bit)
* **[vc_redist.x64.exe](https://aka.ms/vs/16/release/VC_redist.x64.exe)** (64-bit)
* **[vc_redist.arm64.exe](https://aka.ms/vs/16/release/VC_redist.arm64.exe)** (ARM64)

The "x64" branch is based on Audacity's master branch and also:

* Supports targeting 64-bit Windows (both x64 and ARM64)
* The Noise Generator supports an optional Gaussian white noise source in addition
  to the existing uniform white noise source.
* A "whiteg" command has been added to Nyquist that produces white Gaussian noise
to complement the existing "white" command for producing uniform white noise.
* Uses C++'s [`<random>`](http://www.cplusplus.com/reference/random/) infrastructure
instead of Microsoft's broken (and deprecated) implementation of C's
[`rand()`](https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/rand).
The [xoshiro256\*\*](http://vigna.di.unimi.it/papers.php#BlVSLPNG) PRNG is used as
the random engine ([adapted](libnyquist/random_xoshiro256starstar.h) from the reference
[xoshiro256starstar.c](http://prng.di.unimi.it/xoshiro256starstar.c)).
* Better support for handling high-DPI displays.  There are still parts of the UI
that need to be fixed, but on Windows it remains usable up to 150% or so.  Changing
the scale while the application is running only works for some parts of the UI.
* Builds both MSI and EXE based installers.
* Uses Ninja for the CI builds (Windows, macOS, and Ubuntu).

Binaries built from the "x64" branch are available for 32-bit Windows (x86), 64-bit
Windows (x64), 64-bit Windows (x64) with AVX2 support, and Windows 10 on ARM (ARM64).

Note that the 64-bit AVX2 build requires both a CPU that supports those extensions
and Window 7 SP1 or later since earlier versions of Windows do not support the AVX2
extensions.

While the ARM64 binaries are provided, they have not been tested extensively.

***
***

[![Audacity](https://forum.audacityteam.org/styles/prosilver/theme/images/Audacity-logo_75px_trans_forum.png)](https://www.audacityteam.org) 
=========================

[**Audacity**](https://www.audacityteam.org) is an easy-to-use, multi-track audio editor and recorder for Windows, Mac OS X, GNU/Linux and other operating systems. Developed by a group of volunteers as open source.

- **Recording** from any real, or virtual audio device that is available to the host system.
- **Export / Import** a wide range of audio formats, extendible with FFmpeg.
- **High quality** using 32-bit float audio processing.
- **Plug-ins** Support for multiple audio plug-in formats, including VST, LV2, AU.
- **Macros** for chaining commands and batch processing.
- **Scripting** in Python, Perl, or any language that supports named pipes.
- **Nyquist** Very powerful built-in scripting language that may also be used to create plug-ins.
- **Editing** multi-track editing with sample accuracy and arbitrary sample rates.
- **Accessibility** for VI users.
- **Analysis and visualization** tools to analyze audio, or other signal data.

## Getting Started

For end users, the latest Windows and macOS release version of Audacity is available from the [Audacity website](https://www.audacityteam.org/download/).
Help with using Audacity is available from the [Audacity Forum](https://forum.audacityteam.org/).
Information for developers is available from the [Audacity Wiki](https://wiki.audacityteam.org/wiki/For_Developers).
