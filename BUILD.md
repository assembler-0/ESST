## 🧱 Build Instructions

### 🔹 Normal Build

```bash
mkdir build
cd build
cmake ..
make
```

### 🔸 Old Version Build

```bash
mkdir build
cd build
cmake -DBUILD_OLD_VERSION=ON ..
make
```
> Tip: use any build system of your choice, Make, Ninja, etc.
---

> Tip: Use `build_old/` as the folder name if you want to build both versions side by side:

```bash
mkdir build_old
cd build_old
cmake -DBUILD_OLD_VERSION=ON ..
make
```
