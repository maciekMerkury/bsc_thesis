# DPDK Rust bindings for Demikernel

Demikernel is a libOS architecture for kernel-bypass devices. Read more about it
at https://aka.ms/demikernel.

We have tested this crate on Windows and Linux.

## Prerequisites

- Install DPDK.
- Set the env. var CFLAGS to add include path to DPDK installation for header files. (-I<path_to_dpdk_headers>)
- Set the env. var LIBDPDK_PATH to point to the root of the DPDK installation.
- Install Clang.
- Set the env. var CC to point to the clang compiler. Add the clang compiler path to PATH variable.

## Related crates:

https://crates.io/search?q=demikernel

We welcome comments and feedback. By sending feedback, you are consenting that
it may be used in the further development of this project.

## Usage Statement

This project is a prototype. As such, we provide no guarantees that it will
work and you are assuming any risks with using the code. We welcome comments
and feedback. Please send any questions or comments to one of the following
maintainers of the project:

- [Irene Zhang](https://github.com/iyzhang) - [irene.zhang@microsoft.com](mailto:irene.zhang@microsoft.com)
- [Anand Bonde](https://github.com/anandbonde) - [abonde@microsoft.com](mailto:abonde@microsoft.com)

## Trademark Notice

This project may contain trademarks or logos for projects, products, or
services. Authorized use of Microsoft trademarks or logos is subject to and must
follow Microsoft’s Trademark & Brand Guidelines. Use of Microsoft trademarks or
logos in modified versions of this project must not cause confusion or imply
Microsoft sponsorship. Any use of third-party trademarks or logos are subject to
those third-party's policies.
