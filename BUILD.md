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
mkdir buildOLD
cd buildOLD
cmake -DBUILD_OLD_VERSION=ON ..
make
```

### 🔸 Library Build

```bash
mkdir buildLIB
cd buildLIB
cmake -DBUILD_LIBRARY=ON ..
make
```

> Tip: use any build system of your choice, Make, Ninja, etc.