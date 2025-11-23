# TI-84 Plus CE Chart Demos

Beautiful chart visualizations for the TI-84 Plus CE calculator using the Oxygen graphics library.

## Programs

### 1. Pie Chart (Current)
- **File**: `src/piechart.c`
- **Features**:
  - Beautiful pastel colored slices
  - Smart label positioning (inside for large slices, outside with connecting lines for small slices)
  - ENTER: Randomize data
  - +/-: Add/remove slices (3-8 slices)
  - CLEAR: Exit

### 2. Bar Chart
- **File**: `demos/barchart.c`
- **Features**:
  - Rounded-top bars with pastel colors
  - Smart label positioning with staggering
  - ENTER: Randomize data
  - +/-: Add/remove bars
  - GRAPH: Toggle axis break with zigzag indicator
  - CLEAR: Exit

## Building

To build the **Pie Chart** (current):
```bash
make clean && make
```

To build the **Bar Chart** instead:
1. Move files: `mv src/piechart.c demos/ && mv demos/barchart.c src/`
2. Update Makefile: Change `NAME ?= PIECHART` to `NAME ?= BARCHART`
3. Update Makefile: Change description to `"Bar Chart Demo"`
4. Build: `make clean && make`

## Requirements

- CE C Toolchain
- TI-84 Plus CE calculator
- Oxygen graphics library (included in `src/oxygen/`)

## Features

Both programs feature:
- Pixel-perfect rendering
- Pastel color palette
- Interactive controls
- Data randomization
- Dynamic scaling
