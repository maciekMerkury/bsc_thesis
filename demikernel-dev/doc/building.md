
# Building Demikernel

This document contains instructions on how to build Demikernel on Linux. To build on Windows, [follow the instructions
here.](building-windows.md)

> The instructions in this file assume that you have at your disposal at one
Linux machine with Demikernel's development environment set up. For more
information on how to set up Demikernel's development environment, check out
instructions in the `README.md` file.

## Table of Contents

- [Table of Contents](#table-of-contents)
- [Building Demikernel with Default Parameters](#building-demikernel-with-default-parameters)
- [Installing Artifacts (Optional)](#installing-artifacts-optional)
- [Building API Documentation (Optional)](#building-api-documentation-optional)
- [Custom Build Parameters for Catnip LibOS (Optional)](#custom-build-parameters-for-catnip-libos-optional)
  - [Override Default Path for DPDK Libraries](#override-default-path-for-dpdk-libraries)
  - [Override Path to DPDK Package Config File](#override-path-to-dpdk-package-config-file)

## Building Demikernel with Default Parameters

```bash
# Builds Demikernel with default LibOS.
# This defaults to LIBOS=catnap.
make

# Build Demikernel with Linux Sockets LibOS.
make LIBOS=catnap

# Build Demikernel with DPDK LibOS.
make LIBOS=catnip

# Build Demikernel with Raw Sockets LibOS
make LIBOS=catpowder
```

## Installing Artifacts (Optional)

```bash
# Copies build artifacts to your $HOME directory.
make install

# Copies build artifacts to a specific location.
make install INSTALL_PREFIX=/path/to/location
```

## Building API Documentation (Optional)

```bash
cargo doc --no-deps    # Build API Documentation
cargo doc --open       # Open API Documentation
```

## Custom Build Parameters for Catnip LibOS (Optional)

The following instructions enable you to tweak the building process for Catnip
LibOS.

### Override Default Path for DPDK Libraries

Override this parameter if your `libdpdk` installation is not located in your
`$HOME` directory.

```bash
# Build Catnip LibOS with a custom location for DPDK libraries.
make LIBOS=catnip LD_LIBRARY_PATH=/path/to/dpdk/libs
```

### Override Path to DPDK Package Config File

Override this parameter if your `libdpdk` installation is not located in your
`$HOME` directory.

```bash
# Build Catnip LibOS with a custom location for DPDK package config files.
make PKG_CONFIG_PATH=/path/to/dpdk/pkgconfig
```
