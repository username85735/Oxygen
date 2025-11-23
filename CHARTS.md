# TI-84 Plus CE Programs

Comprehensive suite of programs for the TI-84 Plus CE calculator.

## Programs

### 1. MAT142 Exam 2 Suite (Current)
- **File**: `src/mathsuite.c`
- **Purpose**: Complete statistics and probability calculator for MAT142 exams
- **Modules**:
  - **Normal Distribution Solver**: Z-scores, CDF, inverse CDF, empirical rule
  - **Probability & Set Logic**: Union, intersection, conditionals, independence tests
  - **Counting & Combinatorics**: Factorials, permutations, combinations
- **Features**:
  - Overflow protection for large factorials
  - High-precision normal distribution approximation (Abramowitz & Stegun)
  - Binary search for inverse CDF
  - Interactive menu-driven interface
- **Controls**:
  - Arrow keys: Navigate menus
  - 1-3: Edit input values
  - ENTER: Confirm/Select
  - GRAPH: Special functions (inverse CDF)
  - 2nd: Return to main menu
  - CLEAR: Exit

### 2. Pie Chart
- **File**: `demos/piechart.c`
- **Features**:
  - Beautiful pastel colored slices
  - Smart label positioning (inside for large slices, outside with connecting lines for small slices)
  - Smooth circle rendering with 1-degree increments
  - ENTER: Randomize data
  - +/-: Add/remove slices (3-8 slices)
  - CLEAR: Exit

### 3. Bar Chart
- **File**: `demos/barchart.c`
- **Features**:
  - Rounded-top bars with pastel colors
  - Smart label positioning with staggering
  - ENTER: Randomize data
  - +/-: Add/remove bars
  - GRAPH: Toggle axis break with zigzag indicator
  - CLEAR: Exit

## Building

To build the **MAT142 Suite** (current):
```bash
make clean && make
```

To build the **Pie Chart** instead:
1. Move files: `mv src/mathsuite.c demos/ && mv demos/piechart.c src/`
2. Update Makefile: Change `NAME ?= MATHSUIT` to `NAME ?= PIECHART`
3. Update Makefile: Change description to `"Pie Chart Demo"`
4. Build: `make clean && make`

To build the **Bar Chart**:
1. Move files: `mv src/mathsuite.c demos/ && mv demos/barchart.c src/`
2. Update Makefile: Change `NAME ?= MATHSUIT` to `NAME ?= BARCHART`
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
