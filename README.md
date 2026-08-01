# LSD kernel
A hobby kernel in C that I aim to improve.

## Running and testing
1. Install dependencies:
#### Arch linux
```
sudo pacman -S qemu-desktop xorriso make git nasm
```

#### NixOS
**For NixOS you can skip dependency installation entirely and use the nix flake:**
```
nix develop
```
This creates a new nix shell that disposes / caches the installed dependencies

2. Install the cross compiler (optional, because you can remove the TOOLCHAIN_PREFIX value in the Makefile)
#### Arch linux
```
yay -S x86_64-elf-gcc
```
3. Build and run the kernel in a virtual machine:
```
make run
```
## TODO:
- Scheduling
- Permissions
- Rewrite allocator

