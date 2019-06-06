# toddler-ng
The next generation Toddler

## Major Goals
* Written in C99
* Unified boot protocol for all architectures with Device Tree support, combined with ACPI on x86 and OFW on PPC
* Support more architectures (ARM64, PowerPC64, RISC-V, Alpha, SPARC, m68k) and machines (Multiple ARM boards)
* More efficient IPC
* Refined kernel APIs
* Refined VFS and user model
* Complete standard C library
* Console, shell, and utilities

## Long-term Goals
* NetBSD as an environmental system
* X11
* Package management system
* Run on real machines

## Misc
* Fix up SMP support
* Replace NASM assmebly with GAS
* Multiboot compliant on x86
* Even more architectures - VAX, HPPA, IA64, s390

## Status

### Arch

| Target                | Loader    | HAL       |
| --------------------- | --------- | --------- |
| ia32-pc-multiboot     | Current   | Planned   |
| amd64-pc-multiboot    | Current   | Planned   |
| alpha-clipper-qemu    | Active    | Planned   |
| armv7-raspi2-qemu     | Current   | Planned   |
| aarch64v8-raspi3-qemu | Current   | Planned   |
| mips32l-malta-generic | Current   | Planned   |
| mips32b-malta-generic | Current   | Planned   |
| mips64l-malta-generic | Planned   | Planned   |
| mips64b-malta-generic | Planned   | Planned   |
| powerpc-mac-generic   | Current   | Planned   |
| powerpc64-mac-generic | Planned   | Planned   |
| sparcv8-leon3-qemu    | Current   | Planned   |
| sparcv8-sun4m-generic | Active    | Planned   |
| sparcv9-sun4u-generic | Planned   | Planned   |
| riscv32-spkie-qemu    | Planned   | Planned   |
| riscv64-spkie-qemu    | Planned   | Planned   |
| m68k-mcf-qemu         | Planned   | Planned   |
| sh4-shix-qemu         | Planned   | Planned   |
| or1k-sim-qemu         | Planned   | Planned   |
| ia64-ski-generic      | Planned   | Planned   |
| s390-virtio-qemu      | Planned   | Planned   |
| zarch-virtio-qemu     | Planned   | Planned   |
| hppa-sim-qemu         | Planned   | Planned   |

### Kernel

| Item                  | Status    |
| --------------------- | --------- |
| Process management    | Planned   |
| Memory management     | Planned   |
| IPC                   | Planned   |
| Kernel call           | Planned   |
| System API            | Planned   |

### System

| Server                | Status    |
| --------------------- | --------- |
| Virtual file system   | Planned   |
| User account system   | Planned   |
| Driver management     | Planned   |
| Device management     | Planned   |
| Text UI               | Planned   |
| Graphics UI           | Planned   |

| Driver                | Status    |
| --------------------- | --------- |
| Partition             | Planned   |
| File systems          | Planned   |
| RAM FS                | Planned   |
| Interrupt             | Planned   |
| Serial                | Planned   |
| Video                 | Planned   |
| Disk                  | Planned   |
| Keyboard              | Planned   |
| Mouse                 | Planned   |
| Network               | Planned   |

### Apps

| Item                  | Status    |
| --------------------- | --------- |
| Init                  | Planned   |
| Login                 | Planned   |
| Shell                 | Planned   |
