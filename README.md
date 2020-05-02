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
* NetBSD as an environment system
* X11
* Package management system
* Run on real machines

## Misc
* Fix up SMP support
* Replace NASM assmebly with GAS
* Multiboot compliant on x86
* Even more architectures - OpenRISC, HPPA, IA64, s390, VAX

## Status

Planned = Not yet started
Initial = Initial exploration
Active  = Active development
Current = Up to date

### Arch

| Target                | Loader    | HAL       | Note               |
| --------------------- | --------- | --------- | ------------------ |
| ia32-pc-multiboot     | Current   | Planned   | Minor fixes needed |
| amd64-pc-multiboot    | Current   | Planned   | Minor fixes needed |
| alpha-clipper-qemu    | Active    | Planned   |                    |
| armv7-raspi2-qemu     | Current   | Current   |                    |
| aarch64v8-raspi3-qemu | Current   | Planned   | Minor fixes needed |
| mips32l-malta-generic | Current   | Active    |                    |
| mips32b-malta-generic | Current   | Active    |                    |
| mips64l-malta-generic | Initial   | Planned   |                    |
| mips64b-malta-generic | Initial   | Planned   |                    |
| powerpc-mac-generic   | Current   | Planned   | Minor fixes needed |
| powerpc64-mac-generic | Initial   | Planned   |                    |
| sparcv8-leon3-qemu    | Current   | Planned   | Minor fixes needed |
| sparcv8-sun4m-generic | Active    | Planned   |                    |
| sparcv9-sun4u-generic | Planned   | Planned   |                    |
| riscv32-virt-qemu     | Current   | Planned   | Minor fixes needed |
| riscv64-virt-qemu     | Current   | Planned   | Minor fixes needed |
| m68k-atari-aranym     | Active    | Planned   |                    |
| sh4-r2d-qemu          | Active    | Planned   |                    |
| openrisc-sim-qemu     | Current   | Planned   |                    |
| ia64-sim-ski          | Planned   | Planned   |                    |
| s390-virtio-qemu      | Planned   | Planned   |                    |
| zarch-virtio-qemu     | Planned   | Planned   |                    |
| hppa-sim-qemu         | Planned   | Planned   |                    |
| vax-sim-simh          | Initial   | Planned   |                    |

### Kernel

| Component             | Status    |
| --------------------- | --------- |
| Process management    | Active    |
| Page allocation       | Current   |
| Object allocation     | Current   |
| IPC                   | Planned   |
| Kernel call           | Planned   |
| System API            | Planned   |

### System

| Server                | Status    |
| --------------------- | --------- |
| Virtual file system   | Planned   |
| User account system   | Planned   |
| Device management     | Planned   |
| Text UI               | Planned   |
| Graphics UI           | Planned   |

| Driver                | Status    |
| --------------------- | --------- |
| Partition             | Planned   |
| File systems          | Planned   |
| RAM FS                | Planned   |
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
