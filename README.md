# Building modified NodeJS

Most of the building process can be done via the `build_everything.sh` script. One must install the prerequisites and export relevant env variables first.


The building process consists of following steps:
1. installing prerequisites
2. exporting env variables
3. building and installing Demikernel
4. generating Demikernel config file in CONFIG_PATH
5. building and installing demi_epoll
6. building NodeJS

## Prerequisites

1. install rustup
2. if on ubuntu: `demikernel/scripts/install-dev-packages.sh`. otherwise see Demikernel's documentation

## Environmental Variables

The following variables must be exported:
1. CONFIG_PATH - points to a .yaml configuration file for Demikernel
2. LIBOS - must be exactly one of the following: catnap, catpowder, catnip. See Demikernel's documentation for further explanation


The following variables might be exported depending on the configuration:
1. INSTALL_PREFIX if the default is not desired
2. LD_LIBRARY_PATH - if LIBOS == catnip, LD_LIBRARY_PATH must include the path to libdpdk


These variables can be exported by sourcing the `source_env_vars.sh` file.

## Bulding Demikernel

For most configurations, Demikernel can be built with

```
make init
make all-libs
make install INSTALL_PREFIX=$INSTALL_PREFIX
```

If using DPDK (LIBOS == catnip), run the `scripts/build-install-dpdk.sh` script before building.
Also run the `scripts/setup-hugepages.sh` before running/building NodeJS.

## Generating Demikernel config file

the config file can be generated using the `scripts/generate-config.sh` script. it then must be copied to CONFIG_PATH.

## Building demi_epoll

Provided that Demikernel is installed and can be found, the following is sufficient:

```
make all
```

## Building NodeJS

I've found that using ninja as builder results in faster compile times.

As such, the suggested commands are

```
./configure --ninja --prefix $INSTALL_PREFIX
make all
make install
```


# Building standard NodeJS

The 24.0.0 release can be found [here](https://github.com/nodejs/node/releases/tag/v24.0.0).

After downloading the release and extracting it, it can be built using the same process as the modified version.

