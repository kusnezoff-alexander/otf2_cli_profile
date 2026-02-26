---
name: visualization
description: Generate visualizations from JSON profiling data using Quarto and R
license: MIT
compatibility: opencode
metadata:
  audience: developers
  workflow: profiling
---

## What I do

This skill generates HTML visualizations from OTF-Profiler JSON output using Quarto and R.

## When to use me

Use this when you need to:
- Render profiling results into an interactive HTML report
- Generate charts for I/O analysis
- Create shareable visualization reports

## Prerequisites

- R with jsonlite package installed
- Quarto CLI installed
- results.json file in the scripts/ directory

## How to use

### Option 1: Via profiler CLI (recommended)

```bash
# Run profiler with Quarto rendering
./build/otf-profiler --json --render-quarto -i <trace.otf2> -o <output_prefix>
```

This will:
1. Generate JSON profiling data
2. Copy results.json to scripts/ directory
3. Run Quarto to render output.qmd
4. Create output.html with embedded charts

### Option 2: Standalone Quarto rendering

```bash
cd scripts
quarto render output.qmd --to html
```

### Option 3: Check the output

After rendering, verify the output contains charts:

```bash
# Check for embedded chart images
grep -c 'img.*figure-img' scripts/output.html

# Open in browser
xdg-open scripts/output.html
```

## Output files

- `scripts/results.json` - Input data
- `scripts/output.html` - Rendered HTML with charts
- `scripts/output_files/figure-html/` - Chart PNG files

## Troubleshooting

If charts don't render:
1. Check R is installed: `which R`
2. Check jsonlite package: `Rscript -e 'library(jsonlite)'`
3. Check Quarto: `quarto --version`
4. Run Quarto manually with verbose output: `cd scripts && quarto render output.qmd --to html`
