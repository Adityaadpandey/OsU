# OsU - A Custom x86 Operating System

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Architecture](https://img.shields.io/badge/arch-x86-green.svg)](https://en.wikipedia.org/wiki/X86)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)](Makefile)

**OsU** is a minimal, educational x86 operating system written from scratch in C and Assembly. It features a custom bootloader, FAT16 filesystem, interactive shell, vim-like text editor, and two embedded scripting languages (Forth-like and Python-like).

## 🚀 Features

### Core System
- **Custom Bootloader**: Two-stage bootloader (MBR + Stage2) with protected mode transition
- **32-bit Protected Mode**: Full x86 protected mode with GDT setup
- **Interrupt Handling**: Complete IDT with CPU exceptions and hardware IRQs
- **Memory Management**: Simple bump allocator with 4MB heap
- **ATA Disk Driver**: LBA28 disk I/O for persistent storage

### Filesystem
- **FAT16 Implementation**: Full read/write FAT16 filesystem
- **Directory Support**: Hierarchical directory structure with cd, mkdir, rmdir
- **File Operations**: Create, read, write, append, delete files
- **Virtual File System**: Clean VFS abstraction layer

### User Interface
- **Interactive Shell**: Unix-like command-line interface with history
- **Vim-like Editor**: Modal text editor with insert/normal modes
- **VGA Text Mode**: 80x25 color text display with cursor control
- **PS/2 Keyboard**: Full keyboard driver with shift support

### Scripting Languages
- **CosyPy**: Python-like interpreter with variables, functions, loops, conditionals
- **Forth**: Stack-based language for quick calculations
- **Shell Scripts**: Batch execution of shell commands

## 📋 Requirements

### Build Tools
- **Cross Compiler**: `i686-elf-gcc`, `i686-elf-ld`, `i686-elf-objcopy`
- **Assembler**: `nasm` (Netwide Assembler)
- **Emulator**: `qemu-system-i386` (for testing)
- **Build System**: GNU Make

### Installing Cross Compiler

**macOS (Homebrew)**:
```bash
brew install i686-elf-gcc nasm qemu
```

**Linux (Ubuntu/Debian)**:
```bash
sudo apt-get install nasm qemu-system-x86
# For cross compiler, build from source or use package manager
```

**Building from Source**:
See [OSDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler) for detailed instructions.

## 🔨 Building

```bash
# Build the OS image
make

# Build and run in QEMU
make run

# Clean build artifacts
make clean
```

The build process creates `build/os.img`, a bootable disk image.

## 🎮 Usage

### Shell Commands

```bash
# File operations
touch FILE          # Create empty file
cat FILE            # Display file contents
write FILE TEXT     # Write text to file
append FILE TEXT    # Append text to file
rm FILE             # Delete file
edit FILE           # Open vim-like editor

# Directory operations
pwd                 # Print working directory
cd DIR              # Change directory
mkdir DIR           # Create directory
rmdir DIR           # Remove empty directory
ls                  # List directory contents

# Scripting
run FILE.sh         # Execute shell script
pyrun FILE.py       # Execute CosyPy script
python              # Start CosyPy REPL
lang                # Start Forth REPL

# System
help                # Show all commands
clear               # Clear screen
mem                 # Show memory usage
history             # Show command history
reboot              # Reboot system
```

### Text Editor (Vim-like)

```
Normal Mode:
  i       - Enter insert mode
  h/j/k/l - Move cursor (left/down/up/right)
  x       - Delete character
  :w      - Save file
  :q      - Quit (if no changes)
  :q!     - Quit without saving
  :wq     - Save and quit
  Ctrl+S  - Quick save

Insert Mode:
  ESC     - Return to normal mode
  Ctrl+S  - Quick save
  Type normally to insert text
```

### CosyPy Language

```python
# Variables and arithmetic
x = 10
y = x * 2 + 5
print(y)

# Conditionals
if x > 5:
    print("x is large")
else:
    print("x is small")
endif

# Loops
i = 0
while i < 5:
    print(i)
    i = i + 1
endwhile

# Functions
def greet():
    print("Hello from OsU!")
enddef

greet()

# Input
x = input()
print(x * 2)
```

### Forth Language

```forth
# Stack operations
5 10 +        # Push 5, push 10, add → 15
.             # Pop and print

# Stack manipulation
42 dup        # Duplicate top: 42 42
swap          # Swap top two
drop          # Remove top

# Arithmetic
10 5 -        # Subtraction: 5
6 7 *         # Multiplication: 42
20 4 /        # Division: 5

.s            # Show stack contents
clear         # Clear stack
```

## 📁 Project Structure

```
.
├── boot/
│   ├── mbr.asm           # Master Boot Record (stage 1)
│   └── stage2.asm        # Stage 2 bootloader (loads kernel)
├── kernel/
│   ├── entry.asm         # Kernel entry point
│   ├── isr.asm           # Interrupt service routines
│   ├── kmain.c           # Kernel main function
│   ├── idt.c/h           # Interrupt descriptor table
│   ├── vga.c/h           # VGA text mode driver
│   ├── keyboard.c/h      # PS/2 keyboard driver
│   ├── memory.c/h        # Memory allocator
│   ├── disk.c/h          # ATA disk driver
│   ├── fs.c/h            # FAT16 filesystem
│   ├── vfs.c/h           # Virtual filesystem layer
│   ├── shell.c/h         # Interactive shell
│   ├── editor.c/h        # Text editor
│   ├── cospy.c/h         # CosyPy interpreter
│   ├── lang.c/h          # Forth interpreter
│   ├── script.c/h        # Shell script executor
│   ├── string.c/h        # String utilities
│   └── io.h              # Port I/O macros
├── build/                # Build output directory
├── Makefile              # Build configuration
├── linker.ld             # Linker script
└── README.md             # This file
```

## 🏗️ Architecture

### Boot Process
1. **BIOS** loads MBR (sector 0) to 0x7C00
2. **MBR** loads Stage2 bootloader (sectors 1-16)
3. **Stage2** enables A20 line, loads kernel, sets up GDT
4. **Stage2** switches to protected mode and jumps to kernel
5. **Kernel** initializes subsystems and starts shell

### Memory Layout
```
0x00000000 - 0x000003FF  Real mode IVT
0x00000400 - 0x000004FF  BIOS data area
0x00007C00 - 0x00007DFF  MBR (512 bytes)
0x00007E00 - 0x00009FFF  Stage2 bootloader
0x00010000 - 0x0001FFFF  Kernel load area (temporary)
0x00100000 - 0x001FFFFF  Kernel relocated (1MB+)
0x00200000 - 0x003FFFFF  Heap (2MB)
```

### Disk Layout
```
Sector 0:           MBR
Sectors 1-16:       Stage2 bootloader
Sectors 17-144:     Kernel binary
Sectors 4096+:      FAT16 filesystem
```

## 🧪 Testing

Run in QEMU:
```bash
make run
```

QEMU will boot the OS and display the shell prompt. Test various commands:
```bash
# Create and edit a file
touch hello.txt
write hello.txt "Hello, OsU!"
cat hello.txt

# Test directories
mkdir test
cd test
pwd
cd ..

# Test scripting
write test.py "x = 42\nprint(x * 2)"
pyrun test.py
```

## 🐛 Debugging

Enable QEMU debugging:
```bash
qemu-system-i386 -drive format=raw,file=build/os.img -d int,cpu_reset -no-reboot
```

View kernel size:
```bash
ls -lh build/kernel.bin
```

Check for compilation warnings:
```bash
make clean && make 2>&1 | grep warning
```

## 📚 Learning Resources

- [OSDev Wiki](https://wiki.osdev.org/) - Comprehensive OS development guide
- [Intel x86 Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) - CPU architecture reference
- [FAT16 Specification](https://en.wikipedia.org/wiki/File_Allocation_Table) - Filesystem details
- [NASM Documentation](https://www.nasm.us/docs.php) - Assembly syntax

## 🤝 Contributing

This is an educational project. Feel free to:
- Report bugs or issues
- Suggest new features
- Submit improvements
- Fork and experiment

## 📄 License

MIT License - See LICENSE file for details

## 🎯 Future Enhancements

- [ ] Multi-tasking and process scheduling
- [ ] User mode and system calls
- [ ] Network stack (TCP/IP)
- [ ] Graphics mode (VESA)
- [ ] More filesystem support (ext2, FAT32)
- [ ] USB driver support
- [ ] Sound driver (AC97/SB16)
- [ ] More complete Python implementation

## 👨‍💻 Author

Built with ❤️ for learning and exploration of low-level systems programming.

---

**Note**: This OS is for educational purposes. It's not production-ready and lacks many security features required for real-world use.
