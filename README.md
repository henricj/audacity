
# Audacity

## Platforms

### Windows

|   Branch   | [Azure](https://dev.azure.com/henricj/audacity/_build) | [Appveyor](https://ci.appveyor.com/project/henricj/audacity) |
|------------|------------|------------|
|   master   |            | [![Build status](https://ci.appveyor.com/api/projects/status/4fdw8hf8ca6ktofd/branch/master?svg=true)](https://ci.appveyor.com/project/henricj/audacity-n5suy/branch/master) |
|    build   | [![Build Status](https://dev.azure.com/henricj/audacity/_apis/build/status/audacity-CI?branchName=build)](https://dev.azure.com/henricj/audacity/_build/latest?definitionId=4&branchName=build) | [![Build status](https://ci.appveyor.com/api/projects/status/acbs8m3vavu51yeu/branch/build?svg=true)](https://ci.appveyor.com/project/henricj/audacity/branch/build) |
|     x64    | [![Build Status](https://dev.azure.com/henricj/audacity/_apis/build/status/audacity-CI?branchName=x64)](https://dev.azure.com/henricj/audacity/_build/latest?definitionId=4&branchName=x64) | [![Build status](https://ci.appveyor.com/api/projects/status/acbs8m3vavu51yeu/branch/x64?svg=true)](https://ci.appveyor.com/project/henricj/audacity/branch/x64) |

#### "Build" Branch

Windows "build" branch binaries can be found in the **[Releases](../../releases/latest)**
named "build-\*".

The 32-bit [Microsoft Visual C++ Redistributable for Visual Studio
2015-2019](https://www.visualstudio.com/downloads/)
must also be installed. (The link may appear as "Microsoft Visual C++ Redistributable
for Visual Studio 2019" on the Visual Studio download page, but should appear as
2015-2019 in the list of installed programs.)  If not already available,
the installer can be downloaded here:
**[vc_redist.x86.exe](https://aka.ms/vs/16/release/vc_redist.x86.exe)**

The "build" branch is nearly the same as the official Audacity "master" branch.  There
are a few changes to how builds are performed, in order to simplify adding support for
targeting 64-bit Windows.  A LAME MP3 shared library (DLL) and the ```mod-null```,
```mod-nyq-bench```, ```mod-script-pipe``` Audacity modules are also built and
included in the download.

#### "x64" Branch

Windows "x64" branch binaries can be found in the **[Releases](../../releases/latest)**
named "x64-\*".

The [Microsoft Visual C++ Redistributable for Visual Studio
2015-2019](https://www.visualstudio.com/downloads/)
must also be installed. If not already available, the installers can be
downloaded from:

* **[vc_redist.x86.exe](https://aka.ms/vs/16/release/VC_redist.x86.exe)** (32-bit)
* **[vc_redist.x64.exe](https://aka.ms/vs/16/release/VC_redist.x64.exe)** (64-bit)
* **[vc_redist.arm64.exe](https://aka.ms/vs/16/release/VC_redist.arm64.exe)** (ARM64)

The "x64" branch includes the "build" branch changes and also:

* Supports targeting 64-bit Windows (both x64 and ARM64)
* Uses C++'s ```<random>``` instead of Microsoft's broken (and deprecated) implementation
of C's ```rand()```.
* The Noise Generator supports an optional Gaussian white noise source in addition
  to the existing uniform white noise source.
* A "whiteg" command has been added to Nyquist that produces white Gaussian noise.

Binaries built from the "x64" branch are available for 32-bit Windows (x86), 64-bit
Windows (x64), 64-bit Windows (x64) with AVX2 support, and Windows 10 on ARM (ARM64).

Note that the 64-bit AVX2 build requires both a CPU that supports those extensions
and Window 7 SP1 or later since earlier versions of Windows do not support the AVX2
extensions.

While the ARM64 binaries are provided, they have not been tested extensively.

***

### Linux

|   Branch   | [Travis](https://travis-ci.org/henricj/audacity) |
|------------|------------|
|   master   | [![Build Status](https://travis-ci.org/henricj/audacity.svg?branch=master)](https://travis-ci.org/henricj/audacity) |
|   build    | [![Build Status](https://travis-ci.org/henricj/audacity.svg?branch=build)](https://travis-ci.org/henricj/audacity) |
|    x64     | [![Build Status](https://travis-ci.org/henricj/audacity.svg?branch=x64)](https://travis-ci.org/henricj/audacity) |

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
