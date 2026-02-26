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

### Check for errors

```bash
# Look for Quarto rendering errors
grep -i 'error\|Error\|ERROR' scripts/output.html | head -10

# Check browser console errors (if opened in browser)
# Look for JavaScript errors or missing resources
```

### Verify JSON data

```bash
# Check results.json exists and has content
ls -la scripts/results.json

# Validate JSON structure
python3 -c "import json; d=json.load(open('scripts/results.json')); print('Keys:', list(d.keys()))"
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

# Check for key sections
echo ""
echo "Sections found:"
grep -oE 'id="[^"]*"' scripts/output.html | grep -v '^id="$' | head -20

# Look for errors
ERROR_COUNT=$(grep -ci 'error' scripts/output.html 2>/dev/null || echo 0)
echo ""
echo "Error mentions: $ERROR_COUNT"
```

## Expected output

A successful visualization should have:

- `scripts/output.html` (20-50 KB)
- At least 5 embedded chart images
- Sections: Overview, I/O by Paradigm, Access Patterns, Top Files
- No rendering errors

## Troubleshooting

If verification fails:

1. **No HTML file**: Run Quarto manually to see errors:
   ```bash
   cd scripts && quarto render output.qmd --to html 2>&1
   ```

2. **No charts**: Check R/jsonlite installed:
   ```bash
   Rscript -e 'library(jsonlite); print("OK")'
   ```

3. **Empty charts**: Check results.json has data
   ```bash
   python3 -c "import json; d=json.load(open('scripts/results.json')); print('Files:', len(d.get('Files', [])))"
   ```

4. **Missing sections**: Check output.qmd syntax
   ```bash
   quarto render output.qmd --to html --verbose
   ```
