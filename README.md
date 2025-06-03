# 🔥 ESST (Extreme System Stability Stresser) 

**Extreme System Stability Stresser (ESST)** is a brutally aggressive benchmark written in C++ and x86_64 Assembly designed to push server hardware to its absolute breaking point. Not for the faint of heart.

![GitHub license](https://img.shields.io/badge/license-MIT-red)
![Platform](https://img.shields.io/badge/Platform-Linux%20Only-red)
![Danger Level](https://img.shields.io/badge/DANGER-Hardware%20Damage%20Risk-orange)
![CPU Stress](https://img.shields.io/badge/CPU%20Stress-Maximum%20Thermals-red)
![RAM Stress](https://img.shields.io/badge/RAM%20Stress-Rowhammer%20Enabled-critical)

> ⚠️ **WARNING: POTENTIAL HARDWARE DAMAGE**  
> This tool can cause system instability, crashes, data loss, and permanent hardware damage from overheating.  
> **Requirements:** Adequate compute power, proper cooling, power.

## 🐧 Linux Only
![Linux](https://img.shields.io/badge/Compatibility-Linux%20Only-important)

## 💥 Features

| Module | Intensity | Target |
|--------|-----------|--------|
| **Integer Arithmetic** (`3np1.asm`/`primes.asm`) | Extreme | ALUs, Branch Prediction |
| **AES Encryption/Decryption** (`aesENC.asm`/`aesDEC.asm`)|  Nuclear | Crypto Accelerators |
| **AVX/FMA Floating-Point** (`avx.asm`) | Meltdown | Vector Units |
| **Disk I/O Stress** (`diskWrite.asm`) | SSD Killer | Storage Subsystem |
| **Memory Flooding** (`flood.asm`) | Rowhammer | DRAM Integrity |

## 🚀 Versions

| Version | Description |
|---------|-------------|
| `esst` (Full) ![Full Version](https://img.shields.io/badge/Version-Full%20Destruction-red) | Maximum system saturation |
| `esstOld` ![Old Version](https://img.shields.io/badge/Version-Legacy%20Pain-orange) | Higher intensity, less efficient |
