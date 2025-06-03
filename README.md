# esst (Extreme System Stability Stresser)

**Extreme System Stability Stresser (esst)** is a highly aggressive and comprehensive benchmark designed to push server-grade hardware to its absolute limits. This tool is intended for experienced users and system administrators to assess the stability, thermal management, and power delivery capabilities of their Linux-based systems under maximum sustained load.

**WARNING: USE WITH EXTREME CAUTION. This stresser can cause system instability, crashes, data loss, and potentially hardware damage due to overheating or power delivery issues if your system is not adequately cooled and powered. Ensure you have proper monitoring in place before use.**

## Linux Only

This stresser is designed specifically for **Linux** operating systems.

## Features

`esst` encompasses multiple, concurrently running modules to saturate various components of your CPU and memory subsystem:

* **Integer Arithmetic Stress (`3np1.asm`):** Aggressively calculates iterations of the Collatz Conjecture, heavily utilizing integer execution units and branch prediction.
* **AES Encryption/Decryption (`aesENC.asm`, `aesDEC.asm`):** Leverages Intel AES-NI instructions for ultra-intensive parallel AES-256 XTS mode operations, pushing dedicated crypto accelerators and memory bandwidth.
* **AVX/FMA Floating-Point Stress (`avx.asm`):** Saturates AVX and FMA (Fused Multiply-Add) units with complex floating-point computations, generating significant heat and stressing vector processing capabilities.
* **Disk I/O Stress (`diskWrite.asm`):** Performs continuous, high-volume disk write and delete operations, testing storage subsystem throughput and filesystem metadata handling.
* **Memory and Cache Flooding (`flood.asm`):**
    * **L1/L2 Cache Flooding:** Designed to thrash CPU caches, inducing high cache miss rates.
    * **Main Memory Flooding:** Stresses main memory bandwidth with mixed read/write patterns and non-temporal stores.
    * **Rowhammer Attack Module:** Attempts to induce Rowhammer bit flips, a very aggressive memory test for DRAM integrity (operates with `clflush` to bypass caches).
    * **Non-Temporal Flooding:** Specifically targets memory bandwidth using non-temporal (cache-bypassing) stores for streaming workloads.

The stresser is designed to run multiple instances of these tasks simultaneously per CPU thread, creating an unparalleled load.

## Versions

`esst` comes in two versions to suit different testing needs:

* **`esst` (Full / Extreme):** The primary, highly aggressive version designed for maximum system saturation and stability testing. It utilizes all implemented stress modules at their highest intensity and concurrency. This version is intended to push hardware to its absolute breaking point.
* **`esstOld`:** An older version of esst with higher intesity and less efficient.

## Usage

1.  **Compile the Assembly:** You will need an assembler (e.g., NASM or YASM) and a linker (e.g., `ld`) to compile the `.asm` files into an executable. A C/C++ wrapper (`src/main.cpp` or `src/mainLite.cpp`) is required to manage threads and modules.
2.  **Building `esst` (Full Version):**
    ```bash
    mkdir build_full
    cd build_full
    cmake ..
    make
    ```
    This will compile `src/main.cpp` and create an executable named `esst`.
3.  **Building `esst-lite` (Lite Version):**
    ```bash
    mkdir build_lite
    cd build_lite
    cmake -DBUILD_OLD_VERSION=ON ..
    make
    ```
    This will compile `src/mainOld.cpp` and create an executable named `esstOld`.
4.  **Run from a Dedicated Terminal:** **Do not run `esst` or `esstOld` directly in interactive console environments like Jupyter Notebooks or standard shell sessions without proper process management.** These environments are not designed to handle such extreme resource contention and will likely become unresponsive or crash.
5.  **Use Process Management Tools:**
    * **`nohup`:** `nohup ./esst_executable > output.log 2>&1 &` (Runs in background, redirects output, keeps running after disconnect).
    * **`screen` or `tmux`:** Create persistent terminal sessions to start `esst`, detach, and reattach later to monitor (if the system isn't completely saturated).
6.  **Redirect Output:** Always redirect stdout and stderr to a log file (`> output.log 2>&1`) for post-analysis, as live console output will likely be impossible to view.
7.  **Implement Timeouts:** For robust benchmarking, consider implementing internal time limits for each stress module within your code to ensure controlled execution and allow for result collection.

### Monitoring

Due to the extreme load, traditional monitoring tools (like `top`, `htop`, `ls`) run on the same system may become unresponsive. It is highly recommended to use:

* **Remote/Out-of-Band Monitoring:** Utilize your server's **BMC (Baseboard Management Controller) or IPMI interface** for real-time temperature, power consumption, and system health readings. These systems operate independently of the main OS.
* **Internal Logging:** Design `esst` to log its own progress, completion times, and any error messages directly to files for post-run analysis.

## Reference Performance

**System:** 2x Intel Xeon Platinum 8468V CPUs, 1TB RAM, Intel Max AI GPUs
**Threads:** 192 logical threads, 6 tasks per thread running simultaneously.

**Reference Time:** 01:25:36 (1 hour, 25 minutes, 36 seconds)
**Status at Reference Time:** Crashed/Did Not Finish (DNF) - The system completed approximately 1/6th of the total assigned tasks before experiencing a critical event (crash or unrecoverable state). VERSION v0.4

This reference highlights the sheer intensity of `esst`. If your system can run for longer, it indicates higher stability and resilience under extreme conditions.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## Disclaimer

This software is provided "as is," without warranty of any kind, express or implied. The author disclaims all liability for any direct, indirect, incidental, special, exemplary, or consequential damages arising from the use or inability to use this software. By using `esst`, you acknowledge and agree to assume all risks associated with its operation.
