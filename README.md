# CSCE311-Program2

SETUP GUIDE:

1. Install Dependencies (RiskV + QEMU)

Ubuntu:

sudo apt-get update
sudo apt-get install gcc-riscv64-unknown-elf gdb-multiarch

sudo apt-get install qemu-system-misc


Mac (HomeBrew):

brew tap riscv/riscv
brew install riscv-tools

brew install qemu

2. Run Demo Program

- cd into /riscv-os
- make clean
- make
- make run