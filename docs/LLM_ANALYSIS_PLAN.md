# LLM API Configuration for Automatic Report Analysis

## Overview

This document outlines a plan to add LLM API configuration that can automatically analyze the `output.html` report and provide insights, recommendations, and anomaly detection.

## Architecture

```
┌─────────────────┐     ┌──────────────┐     ┌─────────────────┐
│ output.html     │────>│ analyze.py    │────>│ LLM API         │
│ (generated)     │     │ (extractor)  │     │ (OpenAI/Anthropic)│
└─────────────────┘     └──────────────┘     └─────────────────┘
                              │                      │
                              v                      │
                      ┌──────────────┐               │
                      │ insights.md  │<─────────────┘
                      │ (analysis)   │
                      └──────────────┘
```

## Implementation Plan

### 1. Configuration File (`llm_config.yaml`)

```yaml
provider: openai  # or "anthropic"
model: gpt-4o-mini
api_key: ${OPENAI_API_KEY}  # from environment

# Analysis options
analysis:
  include_summary: true
  include_recommendations: true
  include_anomaly_detection: true
  max_tokens: 2000

# Report sections to analyze
sections:
  - overview
  - io_by_paradigm
  - access_patterns
  - top_files
```

### 2. Analysis Script (`scripts/analyze_report.py`)

**Features:**
- Extract metrics from `output.html` using BeautifulSoup
- Send extracted data to LLM API
- Generate `insights.md` with analysis

**Key Functions:**
```python
def extract_metrics(html_path: str) -> dict:
    """Extract key metrics from HTML report"""
    
def call_llm_api(metrics: dict, config: dict) -> str:
    """Call LLM with extracted metrics"""
    
def generate_insights(analysis: str, output_path: str):
    """Write analysis to markdown file"""
```

### 3. Integration Points

- **Post-generation hook**: Run after `quarto render output.qmd`
- **CLI option**: `--analyze` flag for `otf-profiler`
- **Standalone**: Run manually with `python analyze_report.py`

### 4. Example Prompt

```
Analyze the following I/O performance data and provide:
1. Key insights (3-5 bullet points)
2. Performance bottlenecks (if any)
3. Recommendations for optimization
4. Anomalies or concerning patterns

Data:
- Total I/O: {total_bytes}
- I/O Operations: {total_ops}
- Paradigms: {paradigms}
- Access Patterns: {patterns}
- Top Files: {top_files}
```

### 5. Files to Create

| File | Description |
|------|-------------|
| `scripts/llm_config.yaml` | API configuration |
| `scripts/analyze_report.py` | Analysis script |
| `scripts/templates/prompt.txt` | LLM prompt template |

### 6. Environment Variables

```bash
# Set API key
export OPENAI_API_KEY="sk-..."
# or
export ANTHROPIC_API_KEY="sk-ant-..."
```

## Usage

```bash
# After generating report
cd scripts
python analyze_report.py --config llm_config.yaml

# Or with otf-profiler
./otf-profiler --json --analyze trace.otf2
```

## Security Considerations

- API keys via environment variables only
- No hardcoded credentials
- Config file in `.gitignore`

## Next Steps

1. Create `llm_config.yaml` template
2. Implement `analyze_report.py`
3. Add CLI integration to `otf-profiler`
4. Add tests for extraction logic
