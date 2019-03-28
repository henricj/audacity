
# Audacity

## Platforms

### Windows

|   Branch   | [Azure](https://henricj.visualstudio.com/audacity/_build) | [Appveyor](https://ci.appveyor.com/project/henricj/audacity) |
|------------|------------|------------|
|   master   |            | [![Build status](https://ci.appveyor.com/api/projects/status/4fdw8hf8ca6ktofd/branch/master?svg=true)](https://ci.appveyor.com/project/henricj/audacity-n5suy/branch/master) |
|    build   | [![Build Status](https://henricj.visualstudio.com/audacity/_apis/build/status/audacity-CI?branchName=build)](https://henricj.visualstudio.com/audacity/_build/latest?definitionId=4&branchName=build) | [![Build status](https://ci.appveyor.com/api/projects/status/acbs8m3vavu51yeu/branch/build?svg=true)](https://ci.appveyor.com/project/henricj/audacity/branch/build) |
|     x64    | [![Build Status](https://henricj.visualstudio.com/audacity/_apis/build/status/audacity-CI?branchName=x64)](https://henricj.visualstudio.com/audacity/_build/latest?definitionId=4&branchName=x64) | [![Build status](https://ci.appveyor.com/api/projects/status/acbs8m3vavu51yeu/branch/x64?svg=true)](https://ci.appveyor.com/project/henricj/audacity/branch/x64) |

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

* **[vc_redist.x86.exe](https://aka.ms/vs/16/release/vc_redist.x86.exe)** (32-bit)
* **[vc_redist.x64.exe](https://aka.ms/vs/16/release/vc_redist.x64.exe)** (64-bit)
* **[vc_redist.arm64.exe](https://aka.ms/vs/16/release/vc_redist.arm64.exe)** (ARM64)

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

While the ARM64 binaries are provided, they have not yet been tested.

***

### Linux

|   Branch   | [Travis](https://travis-ci.org/henricj/audacity) |
|------------|------------|
|   master   | [![Build Status](https://travis-ci.org/henricj/audacity.svg?branch=master)](https://travis-ci.org/henricj/audacity) |
|   build    | [![Build Status](https://travis-ci.org/henricj/audacity.svg?branch=build)](https://travis-ci.org/henricj/audacity) |
|    x64     | [![Build Status](https://travis-ci.org/henricj/audacity.svg?branch=x64)](https://travis-ci.org/henricj/audacity) |

***

Audacity(R): free, open source, cross-platform audio software for
multi-track recording and editing: <https://www.audacityteam.org/>.

User support is provided on Audacity Forum:
<https://forum.audacityteam.org/>.

We welcome feedback on Audacity, suggestions for new or improved features,
and bug reports. Please visit <https://forum.audacityteam.org/viewforum.php?f=25>.

Audacity is copyright (c) 1999-2019 by Audacity Team. This copyright
notice applies to all documents in the Audacity source code archive,
except as otherwise noted (mostly in the lib-src subdirectories).
"Audacity" is a registered trademark of Dominic Mazzoni.

The Audacity documentation is licensed under the Creative Commons
Attribution 3.0 license: <https://creativecommons.org/licenses/by/3.0/legalcode>.

Compilation instructions for Audacity are provided in the source code:

* Windows: [win/build.txt](win/build.txt)
* macOS: [mac/build.txt](mac/build.txt)
* GNU/Linux: [linux/build.txt](linux/build.txt)

Continue to the original [README.txt](README.txt)
