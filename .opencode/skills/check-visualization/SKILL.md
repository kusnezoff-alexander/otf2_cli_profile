---
name: check-visualization
description: Verify and validate Quarto visualization outputs and their contents
license: MIT
compatibility: opencode
metadata:
  audience: developers
  workflow: testing
---

## What I do

This skill verifies that Quarto visualizations have been generated correctly and contain the expected content.

## When to use me

Use this when you need to:
- Verify HTML output was created
- Check that charts were rendered properly
- Validate the visualization contains expected sections
- Debug rendering issues

## How to use

### Basic checks

```bash
# Check HTML file exists
ls -la scripts/output.html

# Check file size (should be > 10KB)
stat -c %s scripts/output.html
```

### Verify charts are present

```bash
# Count embedded chart images
grep -c 'img.*figure-img' scripts/output.html

# Check for specific chart types
grep -l 'I/O by Paradigm' scripts/output.html  # Paradigm charts
grep -l 'Access Pattern' scripts/output.html   # Access pattern charts
```

### Verify content sections

```bash
# Check for expected sections
grep -E '## (Overview|I/O by Paradigm|Local Access Patterns|Global|Top Files)' scripts/output.html
```

### Check bar charts specifically

Bar charts in the HTML should show proper bars, not single rectangles. Check the generated PNG files:

```bash
# List chart files
ls -la scripts/output_files/figure-html/

# Check PNG dimensions (should have reasonable height for bars)
file scripts/output_files/figure-html/*.png

# Verify the PNG contains bar-like shapes (not flat rectangles)
# If bars look like single rectangles, the data vector may be named or structured incorrectly
```

### Verify JSON data

```bash
# Check results.json exists and has content
ls -la scripts/results.json

# Validate JSON structure
python3 -c "import json; d=json.load(open('scripts/results.json')); print('Keys:', list(d.keys()))"

# Check if data has multiple paradigms (bar chart requires >1 value)
python3 -c "import json; d=json.load(open('scripts/results.json')); print('Paradigms:', list(d.get('IOOperations', {}).keys()))"
```

### Full verification script

```bash
#!/bin/bash
echo "=== Visualization Verification ==="

# Check files exist
echo -n "HTML exists: "
[ -f scripts/output.html ] && echo "YES" || echo "NO"

echo -n "JSON exists: "
[ -f scripts/results.json ] && echo "YES" || echo "NO"

# Check sizes
echo "HTML size: $(stat -c %s scripts/output.html 2>/dev/null || echo 0) bytes"
echo "JSON size: $(stat -c %s scripts/results.json 2>/dev/null || echo 0) bytes"

# Count charts
CHART_COUNT=$(grep -c 'img.*figure-img' scripts/output.html 2>/dev/null || echo 0)
echo "Embedded charts: $CHART_COUNT"

# Check PNG files exist and have proper size
if [ -d "scripts/output_files/figure-html" ]; then
  echo ""
  echo "Chart files:"
  ls -la scripts/output_files/figure-html/*.png 2>/dev/null | head -10
fi

# Look for errors
ERROR_COUNT=$(grep -ci 'error' scripts/output.html 2>/dev/null || echo 0)
echo ""
echo "Error mentions: $ERROR_COUNT"
```

## Expected output

A successful visualization should have:

- `scripts/output.html` (20-50 KB)
- At least 4 embedded chart images with multiple bars each
- Sections: Overview, I/O by Paradigm, Access Patterns, Top Files
- No rendering errors

## Common Issues

### Single rectangle instead of bars

If bar charts show as single flat rectangles:
1. Check the data vector - may be a named vector that needs to be converted to numeric
2. Use `as.numeric()` on the vector explicitly
3. Ensure data is not a data.frame column

Example fix in R:
```r
# Wrong - creates named vector
bytes_data <- sapply(io_ops, function(x) x$Bytes)

# Correct - ensures numeric
bytes_vec <- as.numeric(sapply(io_ops, function(x) ifelse(is.null(x$Bytes), 0, x$Bytes)))
```

### Tables not rendering

Use `results="asis"` chunk option for markdown tables:
```{r, results="asis"}
cat("| Col1 | Col2 |\n|------|------|\n")
```
