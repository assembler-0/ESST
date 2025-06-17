# ESST (Extreme System Stability Tester) 

> ⚠️ **WARNING: POTENTIAL HARDWARE DAMAGE**  
> This tool can cause system instability, crashes, data loss, and permanent hardware damage from overheating.  
> **Requirements:** Adequate compute power, proper cooling.

> **⚠️ Please export CXX=hipcc**

## ✅ Current version

![Version](https://img.shields.io/badge/Current%20Version-v0.8-blue)

## 🐧 Compatibility
![Linux](https://img.shields.io/badge/Compatibility-Linux-important)
![Linux](https://img.shields.io/badge/Compatibility-Windows-blue)

## 💥 Features

| Module | Target |
|--------|--------|
| **Integer Arithmetic** (`3np1.asm`/`primes.asm`) | ALUs, Branch Prediction |
| **AES Encryption/Decryption** (`aesENC.asm`/`aesDEC.asm`)| Crypto Accelerators |
| **AVX/FMA Floating-Point** (`avx.asm`) | Vector Units |
| **Disk I/O Stress** (`diskWrite.asm`) | Storage Subsystem |
| **Memory Flooding** (`flood.asm`) | DRAM & Cache Integrity |
| **LZMA compression & decompression** (`lzma.module.cpp`) | Monitor CPU usage and temps |
| **System monitor** (`systemManager.manage.cpp`) | CPU compression/decompression |
| **GPU stressing with ROCm & HIP** (`core.hip.cpp`) | Raw computaion, Memory test, Atomic operations |

## 🚀 Versions

| Version | Description |
|---------|-------------|
| `esst` (Full) ![Full Version](https://img.shields.io/badge/Version-Full-red) | Maximum system saturation |
| `esstCLI` ![Full Version](https://img.shields.io/badge/Version-CLI-red) | Maximum system saturation without system monitor |
| `esstLIB` (Full) ![Old Version](https://img.shields.io/badge/Version-Library-blue) | Static library for ESST (Full) |
| `esstLIB` (No GPU) ![Old Version](https://img.shields.io/badge/Version-Library%20No%20GPU-blue) | Static library for ESST (No GPU) |

## 📜 License

![GitHub license](https://img.shields.io/badge/license-MIT-green)

## 📦 Prerequisites

Make sure you have the following installed:

* **CMake** ≥ 4.0
* **Clang++ (hipcc)** ≥ v19 (or compatible modern compiler)
* **C++20** standard support
* **Standard libraries** (libc++, libstdc++, etc.)
* **NASM** (latest version recommended)
* **Make** (or Ninja or any other build system you prefer)
* **ROCm** and **HIP** libraries and runtime (for AMD GPUs)

## 🔧 Build Instructions

See [BUILD.md](./BUILD.md) for full build instructions.

## Disclaimer

> ⚠️ This software comes with absolutely no warranty.
