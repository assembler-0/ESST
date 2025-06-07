# ESST (Extreme System Stability Stresser) 

![GitHub license](https://img.shields.io/badge/license-MIT-red)
![Platform](https://img.shields.io/badge/Platform-Linux%20Only-red)
> ⚠️ **WARNING: POTENTIAL HARDWARE DAMAGE**  
> This tool can cause system instability, crashes, data loss, and permanent hardware damage from overheating.  
> **Requirements:** Adequate compute power, proper cooling, power.

## ✅ Current version

![Version](https://img.shields.io/badge/Current%20Version-v0.6-blue)

## 🐧 Linux Only
![Linux](https://img.shields.io/badge/Compatibility-Linux%20Only-important)

## 💥 Features

| Module | Target |
|--------|--------|
| **Integer Arithmetic** (`3np1.asm`/`primes.asm`) | ALUs, Branch Prediction |
| **AES Encryption/Decryption** (`aesENC.asm`/`aesDEC.asm`)| Crypto Accelerators |
| **AVX/FMA Floating-Point** (`avx.asm`) | Vector Units |
| **Disk I/O Stress** (`diskWrite.asm`) | Storage Subsystem |
| **Memory Flooding** (`flood.asm`) | DRAM & Cache Integrity |
| **GPU stressing with ROCm & HIP** (`core.hip.cpp`) | Raw computaion, Memory test, Atomic operations |

## 🚀 Versions

| Version | Description |
|---------|-------------|
| `esst` (Full) ![Full Version](https://img.shields.io/badge/Version-Full-red) | Maximum system saturation |
| `esstOld` ![Old Version](https://img.shields.io/badge/Version-Legacy-orange) | Higher intensity, less efficient |

## 📜 License

![GitHub license](https://img.shields.io/badge/license-MIT-green)

## ☠️ Disclaimer

> ⚠️ This software comes with absolutely no warranty. By running ESST, you accept that:
> * Your CPU may become a paperweight
> * Your RAM might develop PTSD
> * Your cooling solution will cry
> * Your sysadmin will hunt you down
> * Your GPU might be cooked
> * (My system works fine tho)
