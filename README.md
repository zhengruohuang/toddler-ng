# toddler-ng
The next generation Toddler

## Major Goals
* Written in C99
* Unified boot protocol for all architectures with Device Tree support, combined with ACPI on x86 and OFW on PPC/SPARC
* Support more architectures (ARM, PowerPC, x86, RISC-V, Alpha, SPARC, m68k) and machines (e.g., Multiple ARM boards)
* More efficient IPC
* Refined kernel APIs
* Refined VFS and user model
* Complete standard C library
* Console, shell, and utilities
* Run on QEMU and other emulators
* Run on real machines

## Long-term Goals
* NetBSD as an environment system
* X11
* Package management system

## Misc.
* Fix up SMP support
* x86: replace NASM assmebly with GAS on x86 and multiboot compliant
* More architectures - OpenRISC, HPPA, SuperH, IA64, s390, VAX

## Status

* Planned = Not yet started
* Initial = Initial exploration
* Active  = Active development
* Current = Up to date

### Targets

| Target                | Loader    | HAL       | Note               |
| --------------------- | --------- | --------- | ------------------ |
| ia32-pc-multiboot     | Current   | Active    | Minor fixes needed |
| amd64-pc-multiboot    | Current   | Active    | Minor fixes needed |
| alpha-clipper-qemu    | Active    | Planned   |                    |
| armv7-raspi2-qemu     | Current   | Current   |                    |
| aarch64v8-raspi3-qemu | Current   | Active    | Minor fixes needed |
| mips32l-malta-qemu    | Current   | Current   |                    |
| mips32b-malta-qemu    | Current   | Current   | Minor fixes needed |
| mips64l-malta-qemu    | Current   | Current   |                    |
| mips64b-malta-qemu    | Current   | Current   | Minor fixes needed |
| powerpc-mac-qemu      | Current   | Active    | Minor fixes needed |
| powerpc64-mac-qemu    | Initial   | Planned   |                    |
| sparcv8-leon3-qemu    | Current   | Planned   | Minor fixes needed |
| sparcv8-sun4m-qemu    | Active    | Planned   |                    |
| sparcv9-sun4u-qemu    | Planned   | Planned   |                    |
| riscv32-virt-qemu     | Current   | Current   |                    |
| riscv64-virt-qemu     | Current   | Active    |                    |
| m68k-q800-qemu        | Active    | Planned   |                    |
| sh4-r2d-qemu          | Active    | Planned   |                    |
| openrisc-sim-qemu     | Current   | Current   |                    |
| ia64-sim-ski          | Initial   | Planned   |                    |
| s390-virtio-qemu      | Planned   | Planned   |                    |
| zarch-virtio-qemu     | Planned   | Planned   |                    |
| hppa-sim-qemu         | Planned   | Planned   |                    |
| vax-sim-simh          | Initial   | Planned   |                    |

### Additional Targets

| Target                    | Status    |
| ------------------------- | --------- |
| amd64-pc-uefi             | Planned   |
| aarch64v8-virt-qemu       | Planned   |
| mips32l-ci20-generic      | Planned   |
| mips64l-loongson3-generic | Planned   |
| powerpc-mac-g3            | Planned   |
| powerpc64-mac-g3          | Planned   |
| powerpc64-pseries-generic | Planned   |
| riscv32-sifive_u-qemu     | Planned   |
| riscv64-sifive_u-qemu     | Planned   |

### Kernel

| Component             | Status    |
| --------------------- | --------- |
| Process management    | Current   |
| Page allocation       | Current   |
| Object allocation     | Current   |
| IPC                   | Current   |
| Kernel call           | Current   |
| System API            | Current   |

### System

| Server                | Status    |
| --------------------- | --------- |
| Virtual file system   | Active    |
| User account          | Planned   |
| Device management     | Active    |
| Text UI               | Active    |
| Graphics UI           | Planned   |

| Driver                | Status    |
| --------------------- | --------- |
| Partition             | Planned   |
| File systems          | Active    |
| RAM FS                | Active    |
| Serial                | Active    |
| Video                 | Planned   |
| Disk                  | Planned   |
| Keyboard              | Planned   |
| Mouse                 | Planned   |
| Network               | Planned   |

### Apps

| App                   | Status    |
| --------------------- | --------- |
| Init                  | Active    |
| Login                 | Planned   |
| Shell                 | Active    |
