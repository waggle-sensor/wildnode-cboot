# Nvidia Xavier CBoot Bootloader Build

Compile the NVidia Jetson Bootloader (CBoot) from source to replace the
bootloader provided in the [NVidia L4T BSP](https://developer.nvidia.com/embedded/linux-tegra).
This allows custom changes to be made to the bootloader specific to the SAGE
project.

This will produce a replacement NVidia bootloader (cboot*.bin) to be used by [nx-image](https://github.com/waggle-sensor/nx-image) to produce the Wild Sage Node NX Build.

## Usage Instructions

Builds are created using the `./build.sh` script. For help execute `./build.sh -?`.

**Note**: this build **must be executed on a Linux host**. Executing the Linux
build in a Docker container is not supported at this time.

Building requires 1 thing1:

1. The path to the GCC Linaro toolchain for building the ARM64 bootloader (see below).

Here is an example build command:

```
./build.sh -t /home/jswantek/workspace/scratch/toolchain/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu
```

which will produce a `cboot_t194_sage-<version>-<sha1>.bin` bootloader.

### Prerequisites

This build has been tested on a native "Ubuntu 18.04.4 LTS" server installation

```
$ lsb_release -a
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 18.04.4 LTS
Release:	18.04
Codename:	bionic

$ uname -a
Linux ubuntu-laptop 5.4.0-47-generic #51~18.04.1-Ubuntu SMP Sat Sep 5 14:35:50 UTC 2020 x86_64 x86_64 x86_64 GNU/Linux
```

The following `apt` packages are required to be installed:

```
apt-get update
apt-get install bc build-essential kmod xxd
```

*Note*: I had the above packages installed on my system when building but not
all the above packages may be required.

#### Jetson Linux Driver Toolchain

In order the compile the kernel the Jetson Linux Driver Toolchain must be used.
The download and usage instructions can be found at on the
[Jetson Linux Driver Toolchain](https://docs.nvidia.com/jetson/l4t/index.html#page/Tegra%20Linux%20Driver%20Package%20Development%20Guide/xavier_toolchain.html)
page.

To use the toolchain with the `./build.sh` script, download the toolchain from this
(currently existing) link: [http://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz](http://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz)
and extract to a local directory.

```
mkdir /path/to/toolchain/
cd /path/to/toolchain/
wget http://releases.linaro.org/components/toolchain/binaries/7.3-2018.05/aarch64-linux-gnu/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
tar xf gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu.tar.xz
```

Then when executing the `./build.sh` specify the path that contains the `./bin`
folder for the `-t` argument.

```
./build.sh -t /path/to/toolchain/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu ...
```

## References

See the file `CBoot_Standalone_Readme_t194.txt` within this repository for the
NVidia provided build instructions.

[NVidia CBoot Download Center](https://developer.nvidia.com/embedded/downloads#?search=cboot)
