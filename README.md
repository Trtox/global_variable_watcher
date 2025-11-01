# 🧩 Global Variable Watcher (`gwatch`)

A lightweight Linux debugger that monitors **reads** and **writes** to a specific global variable using hardware watchpoints (`ptrace` + debug registers).

---

## Features

- Tracks **read** and **write** operations on a variable in real time
- Uses **hardware watchpoints (DR0–DR7)** for efficient monitoring
- Supports launching executables with custom arguments
- Includes **unit** and **integration tests**

## Requirements

- `cmake` ≥ 3.16
- `g++` (C++23 support)
- `make`
- `git`

## Building 

Clone the repository first.

```bash
cd global_variable_watcher
./build.sh
```

## Usage

```bash
./run.sh --var <variableName> --exec <programToWatch> [-- program_args...]
```
## Running tests (including unit test and sample test program)

```bash
  ./run_tests.sh
```

## License 

This project is released under the MIT License.
