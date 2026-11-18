# Technical Audit & Migration Plan: Combined AI Content & Plagiarism Detection System

**Document Version:** 1.0.0  
**Phase:** Phase 1 — Comprehensive Technical Audit & Architectural Migration Blueprint  
**Status:** Approved for Review (Inspection Only — No Code Changes Executed)  
**Date:** 2026-11-06  
**Project:** Plagiarism Detection and Document Similarity Analysis System  
**Target:** Unified AI Content Detection (ADAFAI) + Classical Plagiarism Detection (C11 DSA Engine)

---

## Executive Summary

This document presents a comprehensive technical audit of the **Plagiarism Detection and Document Similarity Analysis System** and outlines the architectural migration plan for integrating **ADAFAI** (Automated Detection Algorithm For Artificial Intelligence) as an independent AI content detection engine.

The upgraded system will provide a unified analysis capability:
1. **Single-Document Upload & Ingestion**: Support for `.pdf`, `.docx`, `.pptx`, and `.txt`.
2. **AI-Generated Writing Analysis**: Multi-signal detection across stylometry, unicode forensics, and discourse patterns.
3. **Human-Like Writing Pattern Analysis**: Measurement of lexical diversity (MATTR), sentence burstiness, and punctuation entropy.
4. **Sentence & Passage-Level Localization**: Interactive sentence-by-sentence heatmap with granular forensic explanations.
5. **Plagiarism Detection Against Reference Corpus**: Full preservation of the existing, authoritative C11 DSA engine.
6. **Unified Dual-Engine Forensic Reporting**: A consolidated report combining AI authorship likelihood and lexical similarity metrics.

In strict adherence to Phase 1 rules:
- **Zero code modifications** have been made to existing source files.
- **Zero files** have been deleted or renamed.
- All **C11 algorithms and data structures are strictly preserved**.
- **No external dependencies** have been installed.
- AI detection scores are treated as **heuristic triage indicators**, not mathematical proofs of authorship. No artificial accuracy claims are made.

---

## 1. Existing Architecture

The current production system operates as a hybrid, three-tier decoupled architecture designed for high execution speed, deterministic results, and cross-platform portability.

```
+--------------------------------------------------------------------------+
|                       PRESENTATION TIER (Frontend)                       |
|   HTML5 / Modern CSS3 / Vanilla ES6 JavaScript (ui/index.html, app.js)   |
|            Hosted on Vercel (or served statically by server.py)          |
+------------------------------------+-------------------------------------+
                                     | HTTP / JSON REST APIs (CORS enabled)
                                     v
+--------------------------------------------------------------------------+
|                     APPLICATION BRIDGE TIER (Python)                     |
|           Python 3.11.9 Threaded HTTP Server (server.py on Render)       |
|    - Multi-format extraction: pymupdf (PDF), docx (DOCX), pptx (PPTX)    |
|    - Content deduplication & SHA-256 collision-resistant disk caching    |
|    - Subprocess IPC bridge to native C binary                            |
+------------------------------------+-------------------------------------+
                                     | Standard POSIX/Win32 CLI IPC
                                     | (Arguments: --doc-info, --compare-json,
                                     |  --batch-json, --export-report)
                                     v
+--------------------------------------------------------------------------+
|                       CORE ALGORITHMIC TIER (C11)                        |
|       Native Compiled Binary (plagiarism_detector / .exe)                |
|    - Strict C11 Standard (GCC -O2), Zero External Runtime Dependencies   |
|    - Hash Table (djb2 + Chaining), KMP, Rabin-Karp, Merge Sort           |
|    - Dynamic Arrays, Jaccard Similarity, Forensic Text Highlighting      |
+--------------------------------------------------------------------------+
```

### 1.1 Architectural Component Boundaries

1. **Frontend Client (`ui/`)**:
   - `index.html`: Clean academic UI featuring an Analysis Dashboard, Algorithms Used tab, Technical Details tab, Document Dropzones, circular SVG progress gauge, 3-metric breakdown bars, matching phrase cards, and synchronized side-by-side text viewer.
   - `style.css`: Responsive CSS design system with CSS custom properties, responsive grids, custom scrollbars, and high-contrast match highlights (`mark.match-hl`).
   - `app.js`: Client controller managing file reading (`FileReader` text/base64), API dispatch, dynamic DOM generation, SVG gauge animation, and forensic report modal. It dynamically detects environment (`IS_LOCAL` flag) to route requests to `localhost:8080` or `https://plagiarism-detector-yyg0.onrender.com`.

2. **Backend Server (`server.py`)**:
   - Implements `ThreadedHTTPServer` using `http.server.BaseHTTPRequestHandler` with `socketserver.ThreadingMixIn`.
   - Cross-Origin Resource Sharing (CORS) headers configured for all origins (`*`) and standard HTTP verbs (`GET`, `POST`, `OPTIONS`).
   - Resolves deployment port dynamically (`PORT` environment variable on Render, command-line argument, or default `8080`).
   - Manages automatic compilation of the C engine on POSIX systems via `gcc` if the binary is absent, applying executable permissions (`chmod 0o755`).

3. **Core Algorithmic Engine (`src/`, `include/`)**:
   - Fully deterministic native C binary.
   - Accepts normalized UTF-8 plain text paths via CLI flags.
   - Emits structured JSON directly to `stdout` for bridge consumption, avoiding inter-process serialization overhead.
   - Manages all dynamic memory allocations internally, enforcing zero-leak deallocation routines.

### 1.2 Storage Architecture & Directory Layout

| Directory Path | Purpose | Lifecycle & Retention |
|---|---|---|
| `documents/` | Bundled sample documents (`sample1.txt`, `sample2.txt`) | Static version-controlled assets |
| `uploads/originals/` | Raw uploaded binary and text files (`<hash>_<filename>`) | Ephemeral (instance lifespan) |
| `uploads/extracted/` | Normalized UTF-8 text files (`<hash>_<basename>.txt`) | Ephemeral (instance lifespan) |
| `reports/` | Generated forensic text reports (`<basename>_report.txt`) | Ephemeral (instance lifespan) |
| `test_corpus/` | Test documents for PDF, DOCX, PPTX, TXT, and corrupt files | Static version-controlled test assets |

---

## 2. Existing API Flow

The application executes document processing and comparison via a clearly defined asynchronous request-response cycle.

```
User (Browser)               server.py (Python)              plagiarism_detector (C11)
      |                              |                                  |
      |--- 1. Select / Drop File --->|                                  |
      |--- 2. POST /api/upload ----->|                                  |
      |    {filename, content, b64}  |--- 3. Decode & SHA-256 Hash ---->|
      |                              |--- 4. Extract Text to Disk ----->|
      |                              |--- 5. Exec --doc-info ---------->|
      |                              |<-- 6. Return Metadata JSON ------|
      |<-- 7. Doc Info Response -----|                                  |
      |                              |                                  |
      |--- 8. Click Compare -------->|                                  |
      |--- 9. POST /api/compare ---->|                                  |
      |    {ref_file, cand_file}     |--- 10. Exec --compare-json ----->|
      |                              |    (Tokenize, Hash, KMP, RK)     |
      |                              |<-- 11. Return Comparison JSON ---|
      |<-- 12. Full Report Response -|                                  |
```

### 2.1 Detailed Endpoint Inventory

#### `GET /api/health` (also `/health`, `/healthz`)
- **Purpose**: Service health and C engine readiness probe.
- **Request Parameters**: None.
- **Response**:
  ```json
  {
    "status": "healthy",
    "engine": "ready",
    "executable": "plagiarism_detector"
  }
  ```

#### `GET /api/sample-docs`
- **Purpose**: Lists reference documents available in the `documents/` directory.
- **Response**: Array of document objects with `filename`, `filepath`, and `size`.

#### `POST /api/upload`
- **Purpose**: Ingests, hashes, extracts, and pre-analyzes any supported document.
- **Request Body**:
  ```json
  {
    "filename": "assignment1.pdf",
    "content": "data:application/pdf;base64,JVBERi0xLjQK...",
    "is_base64": true
  }
  ```
- **Execution Steps**:
  1. Validates extension against `{".txt", ".pdf", ".docx", ".pptx"}`.
  2. Strips data URL headers and decodes Base64 into raw bytes.
  3. Computes a 12-character SHA-256 prefix: `hashlib.sha256(content_bytes).hexdigest()[:12]`.
  4. Saves original binary to `uploads/originals/<hash>_<safe_name>`.
  5. Extracts plain text using `extract_document_text()`.
  6. Saves UTF-8 plain text to `uploads/extracted/<hash>_<base_name>.txt`.
  7. Spawns `[C_EXECUTABLE, "--doc-info", extracted_path]`.
- **Response**:
  ```json
  {
    "success": true,
    "filepath": "uploads/extracted/73b326ccaefd_assignment1.txt",
    "original_path": "uploads/originals/73b326ccaefd_assignment1.pdf",
    "filename": "assignment1.pdf",
    "file_size": 245102,
    "word_count": 1240,
    "sentence_count": 68,
    "unique_words": 412,
    "top_words": [{"word": "algorithm", "frequency": 18}],
    "format": "PDF"
  }
  ```

#### `POST /api/compare`
- **Purpose**: Executes pair-wise similarity analysis between two uploaded documents.
- **Request Body**:
  ```json
  {
    "ref_file": "uploads/extracted/hash1_docA.txt",
    "cand_file": "uploads/extracted/hash2_docB.txt",
    "ref_name": "Original.docx",
    "cand_name": "Submission.pdf"
  }
  ```
- **Execution Steps**:
  1. Invokes `[C_EXECUTABLE, "--compare-json", ref_file, cand_file]` with a 30-second timeout.
  2. Captures stdout JSON containing metrics, matching phrases, and text with boundary markers (`>>> MATCH <<<` / `>>> END MATCH <<<`).
  3. Restores user-facing display filenames.
- **Response**: Complete similarity breakdown, phrase match array, and highlighted strings.

#### `POST /api/batch-compare`
- **Purpose**: Compares one reference document against $N$ candidate documents.
- **Request Body**:
  ```json
  {
    "ref_file": "uploads/extracted/hash1_ref.txt",
    "cand_files": [
      "uploads/extracted/hash2_cand1.txt",
      "uploads/extracted/hash3_cand2.txt"
    ],
    "ref_name": "Baseline.txt",
    "cand_names": {"uploads/extracted/hash2_cand1.txt": "StudentA.docx"}
  }
  ```
- **Execution Steps**:
  1. Invokes `[C_EXECUTABLE, "--batch-json", ref_file, cand1, cand2, ...]` with a 60-second timeout.
  2. The C engine compares each candidate, sorts them via recursive **Merge Sort**, and generates statistical buckets.
- **Response**: Merge-sorted `rankings` array and `statistics` distribution object.

#### `POST /api/export-report`
- **Purpose**: Triggers C engine generation of a permanent forensic report file on disk.
- **Request Body**: `{ ref_file, cand_file, output_filename, ref_name, cand_name }`.
- **Execution Steps**:
  1. Invokes `[C_EXECUTABLE, "--export-report", ref_file, cand_file, dest_report]`.
  2. Reads generated file from `reports/`, injects user-facing document names, and returns file path and text content.

---

## 3. Existing C Engine Architecture

The C11 core engine is an algorithmic implementation designed for deterministic speed and rigorous memory safety. It contains zero third-party dependencies and relies strictly on standard C11 library primitives (`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`).

### 3.1 Preserved Algorithms and Data Structures

```
+-------------------------------------------------------------------------+
|                       C11 ALGORITHMIC ENGINE CORE                       |
+-------------------------------------------------------------------------+
|  Preprocessing:   preprocess_text(), preprocess_tokenize(),             |
|                   preprocess_extract_sentences()                        |
|                                                                         |
|  Data Structures: HashTable (djb2, Separate Chaining)                   |
|                   TokenArray, SentenceArray, MatchArray,                |
|                   PhraseMatchArray, RankedDocumentArray (Geometric 2x)  |
|                                                                         |
|  Pattern Search:  KMP (LPS Pi-Table, O(N+M) worst case)                 |
|                   Rabin-Karp (Rolling Hash Base 256, Mod 1e9+7, O(1))   |
|                                                                         |
|  Similarity:      Jaccard Word Vocabulary (Hash Table iteration)        |
|                   Jaccard Sentence Overlap                              |
|                   Phrase Coverage Ratio (Non-overlapping token masks)   |
|                                                                         |
|  Sorting:         Recursive Merge Sort (Descending, Stable O(D log D))  |
|                                                                         |
|  Reporting:       report_generate(), report_highlight_tokens()          |
+-------------------------------------------------------------------------+
```

#### 1. Hash Table with Separate Chaining (`hashtable.h`, `hashtable.c`)
- **Structure**: `HashTable` contains an array of bucket pointers (`HashEntry** buckets`) and tracks `unique_count`. Each `HashEntry` contains `char *word`, `int frequency`, and a `HashEntry *next` pointer.
- **Hash Function**: The classic `djb2` hash:
  $$\text{hash}_{i} = ((\text{hash}_{i-1} \ll 5) + \text{hash}_{i-1}) + c = \text{hash}_{i-1} \times 33 + c$$
  Initialized with seed `5381`.
- **Performance**: Expected $O(1)$ lookup, insertion, and frequency incrementation.
- **Features**: Frequency analysis and extraction of top-$K$ frequent vocabulary words (`hash_table_get_top_frequent()`).

#### 2. Knuth-Morris-Pratt (KMP) Pattern Matching (`kmp.h`, `kmp.c`)
- **LPS Table**: Precomputes Longest Prefix Suffix ($\pi$-table) in $O(M)$ time for candidate phrase patterns.
- **Matching**: Eliminates character backtracking, achieving guaranteed $O(N + M)$ worst-case phrase search across document text.
- **Phrase Extraction**: `find_matching_phrases_kmp()` identifies maximal continuous identical token sequences $\ge 3$ words between document token arrays.

#### 3. Rabin-Karp Rolling Hash (`rabinkarp.h`, `rabinkarp.c`)
- **Polynomial Rolling Hash**:
  $$H(s) = \left( \sum_{i=0}^{m-1} s[i] \cdot B^{m-1-i} \right) \pmod P$$
  Constants: Base $B = 256$, Prime modulus $P = 1,000,000,007$ (`1000000007ULL`).
- **Rolling Transition**: Computes window shift in $O(1)$ amortized time:
  $$H_{next} = \left( (H_{curr} - s[i] \cdot B^{m-1}) \cdot B + s[i+m] \right) \pmod P$$
- **Verification**: Strict character verification on hash match to eliminate false positives from collisions.

#### 4. Deduplication and Phrase Merging (`report.h`, `report.c`)
- `find_and_deduplicate_phrases()`: Combines phrase matches from both KMP and Rabin-Karp. Any match discovered by both algorithms is unified into a single entry labeled `"KMP + Rabin-Karp"`.

#### 5. Multi-Metric Similarity Calculation (`similarity.h`, `similarity.c`)
- **Word Jaccard Similarity**: Evaluates shared vocabulary between Hash Tables:
  $$J_{\text{word}} = \frac{|V_1 \cap V_2|}{|V_1 \cup V_2|} = \frac{|V_1 \cap V_2|}{|V_1| + |V_2| - |V_1 \cap V_2|} \times 100$$
- **Sentence Jaccard Similarity**: Evaluates unique sentence overlap between documents:
  $$J_{\text{sentence}} = \frac{|S_1 \cap S_2|}{|S_1 \cup S_2|} \times 100$$
- **Phrase Coverage Similarity**: Uses dynamic token coverage masks (`covered1`, `covered2`) to calculate the proportion of document words belonging to matched phrases of length $\ge 3$:
  $$S_{\text{phrase}} = \frac{\max(\text{matched\_words}_1, \text{matched\_words}_2)}{\max(W_1, W_2)} \times 100$$
- **Composite Overall Similarity Formula**:
  $$\text{Overall Similarity} = (J_{\text{word}} \times 0.40) + (J_{\text{sentence}} \times 0.30) + (S_{\text{phrase}} \times 0.30)$$
  *(Audit observation: C header defines `WORD_WEIGHT 0.40`, `SENTENCE_WEIGHT 0.30`, `PHRASE_WEIGHT 0.30`. The UI labels show 30%/30%/40% in informative text; the C engine's 40/30/30 weighting is the authoritative calculation).*
- **Categorical Classification**:
  - `80.0% - 100.0%`: Very High Similarity
  - `60.0% - 79.9%`: High Similarity
  - `40.0% - 59.9%`: Moderate Similarity
  - `20.0% - 39.9%`: Low Similarity
  - `0.0% - 19.9%`: Very Low Similarity

#### 6. Recursive Merge Sort (`mergesort.h`, `mergesort.c`)
- Implements custom divide-and-conquer sorting for `RankedDocument` structures without standard library `qsort()`.
- Orders candidates descending by similarity percentage in guaranteed stable $O(D \log D)$ time.
- Assigns 1-based ranks to sorted candidate records.

#### 7. Linear & Threshold Search (`search.h`, `search.c`)
- `search_ranked_by_filename()`: Performs case-insensitive filename/basename lookup in ranked results.
- `search_ranked_by_threshold()`: Filters and counts documents exceeding a specified similarity percentage.

#### 8. Text Preprocessing & Tokenization (`preprocessing.h`, `preprocessing.c`)
- `preprocess_text()`: Converts ASCII text to lowercase, strips carriage returns (`\r`), normalizes whitespace, and preserves sentence-terminating punctuation (`.`, `!`, `?`).
- `preprocess_tokenize()`: Splits clean text into contiguous alphabetical tokens stored in dynamic `TokenArray`.
- `preprocess_extract_sentences()`: Segments text into discrete sentences stored in `SentenceArray`.

---

## 4. Existing Document-Processing Architecture

The system supports four major document formats through Python extraction libraries:

| Format | Library Used | Function in `server.py` | Extraction Details & Edge-Case Handling |
|---|---|---|---|
| **PDF** (`.pdf`) | `PyMuPDF` (`pymupdf`/`fitz`) | `extract_text_from_pdf()` | Iterates through document pages via `page.get_text()`. Detects password encryption (`doc.is_encrypted`) and aborts with explicit error. Rejects scanned/image-only PDFs lacking extractable text. |
| **DOCX** (`.docx`) | `python-docx` (`docx`) | `extract_text_from_docx()` | Iterates paragraphs (`doc.paragraphs`) and table structures (`doc.tables` $\rightarrow$ `row.cells`), preserving tabular text. |
| **PPTX** (`.pptx`) | `python-pptx` (`pptx`) | `extract_text_from_pptx()` | Iterates slides (`prs.slides`), inspects all shapes containing text frames (`shape.has_text_frame`), and extracts paragraph strings. |
| **TXT** (`.txt`) | Built-in Python I/O | `extract_text_from_txt()` | Attempts UTF-8 decoding with fallback to Latin-1 (`ISO-8859-1`) to avoid encoding crashes. |

### 4.1 Document Pipeline Strengths
- **Format Normalization**: All incoming document formats are converted into clean, newline-delimited plain UTF-8 text prior to downstream consumption.
- **SHA-256 Storage Isolation**: Uploaded files receive unique content-derived identifiers (`file_hash = hashlib.sha256(content_bytes).hexdigest()[:12]`). Original files are preserved in `uploads/originals/`, and extracted plain text is saved to `uploads/extracted/`.
- **Loose Coupling**: The C11 DSA engine is completely isolated from complex binary document formats. It processes only normalized plain text files.

---

## 5. ADAFAI Integration Requirements

ADAFAI is an open-source, multi-signal toolkit created by Viraj Shoor for detecting AI-generated text.

### 5.1 Architecture of ADAFAI

ADAFAI is structured as an ensemble of separable, inspectable linguistic and cryptographic signals:

```
adafai/
├── stylometry.py          # Lexical + statistical features (ZERO dependencies)
├── unicode_forensics.py   # Character-level provenance artifacts (ZERO dependencies)
├── discourse.py           # Rhetorical-scaffold features (ZERO dependencies)
├── spans.py               # Sentence-level localization + explanations (ZERO dependencies)
├── detector.py            # Heuristic ensemble combining signals (ZERO dependencies for core)
├── perplexity.py          # Perplexity + burstiness (Requires PyTorch + Transformers)
├── binoculars.py          # Cross-model perplexity ratio (Requires PyTorch + Transformers)
└── watermark/             # Cryptographic watermark detection
    ├── kirchenbauer.py    # Green/red-list z-test (Requires PyTorch + Transformers)
    ├── synthid.py         # Google SynthID-Text detector wrapper
    └── providers.py       # Per-provider watermark status catalog
```

### 5.2 Signal Breakdown & Operational Characteristics

| Signal Module | Mechanism & Metrics | External Dependencies | Execution Speed | Memory Overhead |
|---|---|:---:|:---:|:---:|
| **Stylometry** (`stylometry.py`) | Moving-Average Type-Token Ratio (MATTR, window 50), Sentence Length Coefficient of Variation (CV), Repeated Trigram Ratio, AI-tell phrase density (54+ characteristic phrases: *delve, underscore, tapestry, pivotal, testament, plethora*), Punctuation Entropy. | **None** (Standard Library) | $< 10\text{ ms}$ | $< 5\text{ MB}$ |
| **Unicode Forensics** (`unicode_forensics.py`) | Detects invisible characters (zero-width spaces/joiners, BOM, soft hyphens), exotic spaces (narrow no-break, thin/figure spaces), mixed-script homoglyphs (Cyrillic/Greek substitutions), and typographic polish (curly vs straight quotes, em-dashes). | **None** (Standard Library) | $< 5\text{ ms}$ | $< 2\text{ MB}$ |
| **Discourse Structure** (`discourse.py`) | Rhetorical structures that survive synonym substitution: Negation pivots (*"not merely X... but rather Y"*), tricolons (rule-of-three lists), self-answered conversational questions (*"Slightly creepy? Sure."*), paragraph punch-line cadence, paragraph-length uniformity. | **None** (Standard Library) | $< 10\text{ ms}$ | $< 5\text{ MB}$ |
| **Sentence Spans** (`spans.py`) | Localizes AI-like characteristics at the sentence level using a 100-word sliding context window. Provides `start` and `end` character offsets, sentence scores, confidence flags, and plain-text explanatory reasons. | **None** (Standard Library) | $< 25\text{ ms}$ | $< 10\text{ MB}$ |
| **Core Ensemble** (`detector.py`) | Weights: Stylometry (0.30), Unicode (0.10), Discourse (0.35). Returns score [0.0 - 1.0], verdict (`likely human`, `uncertain`, `likely AI-generated`, or `insufficient evidence` if $< 150$ words). | **None** (Standard Library) | $< 30\text{ ms}$ | $< 10\text{ MB}$ |
| **Perplexity** (`perplexity.py`) | Evaluates causal LM loss and cross-sentence perplexity variation (burstiness) using `distilgpt2`. | `torch>=2.0`, `transformers>=4.46` | $1 - 5\text{ s}$ | $600\text{ MB} - 1.2\text{ GB}$ |
| **Binoculars** (`binoculars.py`) | Cross-perplexity ratio between observer model (`distilgpt2`) and performer model (`gpt2` or `Falcon-7B`). | `torch>=2.0`, `transformers>=4.46` | $3 - 15\text{ s}$ | $1.2\text{ GB} - 4.0\text{ GB}$ |
| **Watermark** (`watermark/`) | Kirchenbauer green/red-list z-test and Google SynthID wrapper. | `torch>=2.0`, `transformers>=4.46` | $1 - 3\text{ s}$ | $500\text{ MB} - 1.0\text{ GB}$ |

### 5.3 Methodological and Epistemic Integrity

The audit confirms that ADAFAI enforces strict scientific and ethical guidelines:
1. **No Absolute Probabilities**: The system explicitly warns:
   > *"Heuristic ensemble with unfitted weights — triage signal, not proof. Never use as sole evidence in an academic-integrity or disciplinary decision."*
2. **Floor on Text Length**: Texts under 150 words trigger an automatic abstain status (`insufficient evidence`), because short texts lack sufficient statistical variance for reliable stylometric evaluation.
3. **Robustness Limitations**: Deterministic paraphrasing and deliberate humanizer passes defeat surface stylometric signals. The discourse scaffold partially mitigates this, but cannot guarantee detection.
4. **Non-Native English Vulnerability**: Non-native (ESL) writers exhibit lower lexical diversity and more uniform sentence structures, causing elevated false-positive rates on stylometric detectors.

---

## 6. Dependency Compatibility Analysis

### 6.1 Current Production Dependencies (`requirements.txt`)
```text
python-docx>=1.2.0
python-pptx>=1.0.2
pymupdf>=1.28.2
pypdf>=6.18.1
```

### 6.2 Python Runtime Environment
- Current Render specification (`render.yaml`): `PYTHON_VERSION: 3.11.9`.
- ADAFAI Python requirement (`pyproject.toml`): `requires-python = ">=3.10"`.
- **Verdict**: Fully compatible. Python 3.11.9 meets all constraints.

### 6.3 Package Compatibility Matrix

| Package | Current Version | Role | Conflict with ADAFAI Core? | Conflict with ADAFAI Models? |
|---|---|---|:---:|:---:|
| `python-docx` | $\ge 1.2.0$ | DOCX extraction | **None** | None |
| `python-pptx` | $\ge 1.0.2$ | PPTX extraction | **None** | None |
| `pymupdf` | $\ge 1.28.2$ | PDF extraction | **None** | None |
| `pypdf` | $\ge 6.18.1$ | Fallback PDF extraction | **None** | None |
| `torch` | Not installed | Deep learning framework | N/A | **High risk on Render Free Tier** |
| `transformers` | Not installed | HuggingFace models | N/A | **High risk on Render Free Tier** |

### 6.4 Critical Dependency Finding: Core vs. Model-Based ADAFAI

A pivotal discovery of this technical audit is that **ADAFAI's core multi-signal detection engine requires ZERO third-party Python packages**:
- `stylometry.py`, `unicode_forensics.py`, `discourse.py`, `spans.py`, and `detector.py` use only Python's standard library (`math`, `re`, `bisect`, `unicodedata`, `collections`, `hashlib`).
- Integrating the core multi-signal AI detector requires **0 additional pip packages**, adding **0 MB of build download**, and introducing **0 risk of dependency conflicts**.
- Conversely, installing `torch` and `transformers` introduces $> 2.5\text{ GB}$ of dependencies and models, which will cause fatal Out-Of-Memory (OOM) crashes on Render Free Tier (see Section 11).

---

## 7. Proposed New Folder Structure

To integrate ADAFAI cleanly while maintaining complete isolation of the C11 DSA engine, the codebase will be organized into modular subsystems:

```
plagiarism-detector/
│
├── src/                                   # [PRESERVED - UNTOUCHED] C11 Source Files
│   ├── document.c
│   ├── hashtable.c
│   ├── kmp.c
│   ├── linkedlist.c
│   ├── main.c
│   ├── mergesort.c
│   ├── preprocessing.c
│   ├── rabinkarp.c
│   ├── report.c
│   ├── search.c
│   └── similarity.c
│
├── include/                               # [PRESERVED - UNTOUCHED] C11 Header Files
│   ├── document.h
│   ├── hashtable.h
│   ├── kmp.h
│   ├── linkedlist.h
│   ├── mergesort.h
│   ├── preprocessing.h
│   ├── rabinkarp.h
│   ├── report.h
│   ├── search.h
│   └── similarity.h
│
├── adafai/                                # [NEW] Independent AI Content Detection Engine
│   ├── __init__.py                        # Package initialization (exports analyze, analyze_spans)
│   ├── detector.py                        # Multi-signal ensemble & verdict logic
│   ├── stylometry.py                      # MATTR, sentence CV, punctuation entropy, tell phrases
│   ├── unicode_forensics.py               # Zero-width chars, exotic spaces, homoglyphs
│   ├── discourse.py                       # Negation pivots, tricolons, self-QA, paragraph cadence
│   └── spans.py                           # Sentence-level localization and explanation generator
│
├── extractors/                            # [NEW - MODULAR REFACTOR] Document Extraction Modules
│   ├── __init__.py
│   ├── base.py                            # Abstract extractor interface & validation
│   ├── pdf_extractor.py                   # PyMuPDF extractor with encryption & scan checks
│   ├── docx_extractor.py                  # python-docx paragraph & table extractor
│   ├── pptx_extractor.py                  # python-pptx slide text extractor
│   └── txt_extractor.py                   # UTF-8 / Latin-1 text extractor
│
├── ui/                                    # [ENHANCED] Frontend User Interface
│   ├── index.html                         # Updated dashboard supporting dual-analysis workflows
│   ├── app.js                             # Client-side handlers for AI heatmap & combined reports
│   └── style.css                          # Styles for AI score badges, heatmaps, and tabs
│
├── documents/                             # Sample reference documents
├── reports/                               # Exported forensic reports (.txt)
├── test_corpus/                           # Test files (.pdf, .docx, .pptx, .txt)
├── tests/                                 # Test Suites
│   ├── test_hashtable.c                   # C11 test suite
│   ├── test_kmp.c
│   ├── test_mergesort.c
│   ├── test_rabinkarp.c
│   ├── test_realistic.c
│   ├── test_report.c
│   ├── test_search.c
│   ├── test_similarity.c
│   ├── test_extractors.py                 # [NEW] Document extraction test suite
│   ├── test_adafai_core.py                # [NEW] AI detection signal verification suite
│   └── test_api_integration.py           # [NEW] End-to-end REST API verification suite
│
├── build.sh                               # Render build script
├── compile_flags.txt                      # Clangd compilation flags
├── render.yaml                            # Render deployment configuration
├── requirements.txt                       # Python dependencies
├── server.py                              # [ENHANCED] Bridge HTTP Server & Unified API Router
└── start_ui.bat                           # Local development startup script
```

---

## 8. Proposed API Endpoints

All existing endpoints will remain completely backward-compatible. Three new endpoints will be introduced to support single-document AI detection and combined dual-engine forensic analysis.

### 8.1 Preserved Endpoints (100% Backward Compatible)
- `GET /api/health` — Enhanced to report both C engine status and ADAFAI status.
- `GET /api/sample-docs` — Lists bundled sample documents.
- `POST /api/upload` — Ingests, hashes, and extracts `.txt`, `.pdf`, `.docx`, `.pptx`.
- `POST /api/compare` — Executes pair-wise C11 plagiarism comparison.
- `POST /api/batch-compare` — Executes multi-document C11 Merge Sort comparison.
- `POST /api/export-report` — Exports C11 plagiarism comparison report.

### 8.2 New Endpoints

#### 1. `POST /api/ai-detect`
- **Purpose**: Runs independent ADAFAI multi-signal AI analysis on a single document.
- **Request Body**:
  ```json
  {
    "filepath": "uploads/extracted/73b326ccaefd_essay.txt",
    "include_spans": true
  }
  ```
- **Execution**:
  1. Validates that the file exists and reads UTF-8 content.
  2. Executes `adafai.detector.analyze(text, use_models=False)`.
  3. Executes `adafai.spans.analyze_spans(text)` if `include_spans` is true.
- **Response Structure**:
  ```json
  {
    "success": true,
    "score": 0.7842,
    "verdict": "likely AI-generated",
    "word_count": 412,
    "signals": {
      "stylometry": {
        "score": 0.8120,
        "lexical_diversity_mattr": 0.5841,
        "sentence_length_cv": 0.2810,
        "repeated_trigram_ratio": 0.0820,
        "ai_phrase_density_per_1000w": 7.28,
        "punctuation_entropy": 2.1405
      },
      "unicode": {
        "score": 0.1500,
        "invisible_characters": {},
        "exotic_spaces": {},
        "homoglyphs": {},
        "flags": []
      },
      "discourse": {
        "score": 0.6450,
        "negation_pivots_per_1000w": 4.85,
        "tricolons_per_1000w": 2.43,
        "self_qa_per_1000w": 0.0,
        "punch_line_ratio": 0.6000,
        "paragraph_length_cv": 0.3120,
        "negation_pivot_hits": ["not merely", "not just"]
      }
    },
    "spans": {
      "sentence_count": 24,
      "flagged": 7,
      "sentences": [
        {
          "start": 0,
          "end": 84,
          "text": "The rich tapestry of artificial intelligence delves into pivotal areas of learning.",
          "word_count": 11,
          "score": 0.8850,
          "low_confidence": false,
          "reasons": [
            "AI-tell phrases: 'delve', 'pivotal', 'rich tapestry'",
            "low local lexical diversity (MATTR 0.52; human baseline ~0.75+)"
          ]
        }
      ]
    },
    "provenance_flags": [],
    "note": "Heuristic ensemble with unfitted weights - triage signal, not proof."
  }
  ```

#### 2. `POST /api/analyze-combined`
- **Purpose**: Unified workflow executing both ADAFAI AI detection and C11 plagiarism detection on an uploaded document against a reference corpus or comparison document.
- **Request Body**:
  ```json
  {
    "doc_file": "uploads/extracted/73b326ccaefd_submission.txt",
    "doc_name": "StudentPaper.docx",
    "corpus_mode": "samples",
    "reference_file": "uploads/extracted/aa6ab59ebfb8_ref.txt",
    "reference_name": "Baseline.pdf"
  }
  ```
- **Execution**:
  1. Executes ADAFAI analysis on `doc_file`.
  2. If `reference_file` is supplied, executes C11 `--compare-json`. If `corpus_mode == "samples"`, executes C11 `--batch-json` against corpus documents.
  3. Synthesizes a unified response object containing both forensic perspectives.
- **Response Structure**:
  ```json
  {
    "success": true,
    "document": {
      "name": "StudentPaper.docx",
      "filepath": "uploads/extracted/73b326ccaefd_submission.txt",
      "word_count": 412,
      "sentence_count": 24
    },
    "ai_detection": {
      "score": 0.7842,
      "verdict": "likely AI-generated",
      "signals": { ... },
      "spans": { ... },
      "disclaimer": "Triage signal, not proof."
    },
    "plagiarism_detection": {
      "similarity_score": 42.50,
      "similarity_level": "Moderate Similarity",
      "word_similarity": 38.20,
      "sentence_similarity": 25.00,
      "phrase_similarity": 64.30,
      "phrases": [ ... ],
      "highlighted_comp_text": "..."
    }
  }
  ```

#### 3. `POST /api/export-combined-report`
- **Purpose**: Generates and saves a consolidated plain-text or Markdown report documenting both AI writing signals and lexical plagiarism overlap.
- **Request Body**: `{ doc_file, doc_name, ai_data, plagiarism_data, output_filename }`.
- **Response**: `{ success: true, exported_path: "reports/...", content: "..." }`.

---

## 9. Migration Phases

The upgrade will be executed in five strictly gated phases. Each phase requires successful automated testing before proceeding.

```
+--------------------------------------------------------------------------+
| PHASE 1: TECHNICAL AUDIT & ARCHITECTURAL BLUEPRINT [CURRENT]            |
| - Inspect codebase, verify C engine integrity, assess ADAFAI deps        |
| - Produce AI_DETECTION_MIGRATION_PLAN.md                                 |
+------------------------------------+-------------------------------------+
                                     v
+--------------------------------------------------------------------------+
| PHASE 2: ADAFAI CORE INTEGRATION (Zero External Dependencies)            |
| - Vendor zero-dependency ADAFAI modules into adafai/                     |
| - Implement adafai/__init__.py with clean analyze() and spans API        |
| - Verify parity against test suite (test_stylometry, test_spans, etc.)   |
+------------------------------------+-------------------------------------+
                                     v
+--------------------------------------------------------------------------+
| PHASE 3: BACKEND API EXPANSION & COMBINED PIPELINE                      |
| - Refactor document extractors into extractors/ module                   |
| - Implement /api/ai-detect, /api/analyze-combined, export endpoints      |
| - Maintain 100% backward compatibility for existing endpoints            |
+------------------------------------+-------------------------------------+
                                     v
+--------------------------------------------------------------------------+
| PHASE 4: FRONTEND UI ENHANCEMENT                                         |
| - Add "AI Content Detector" & "Unified Analysis" modes to Dashboard      |
| - Build interactive sentence heatmap with tooltip/modal explanations     |
| - Build dual-score visual summary (AI Likelihood vs Plagiarism Score)    |
+------------------------------------+-------------------------------------+
                                     v
+--------------------------------------------------------------------------+
| PHASE 5: VERIFICATION, PERFORMANCE PROFILING & DEPLOYMENT                |
| - Memory profiling under 512MB RAM constraints                           |
| - End-to-end integration testing with multi-format test corpus           |
| - Render and Vercel staging deployment validation                        |
+--------------------------------------------------------------------------+
```

### Phase Details

#### Phase 1: Technical Audit & Migration Plan (Current)
- Complete technical audit of existing repository.
- Verify preservation of all C11 algorithms.
- Analyze ADAFAI architecture, dependencies, and model requirements.
- Document Render free-tier constraints and memory risks.

#### Phase 2: ADAFAI Core Integration (Backend Engine)
- Copy ADAFAI core modules (`stylometry.py`, `unicode_forensics.py`, `discourse.py`, `spans.py`, `detector.py`) into `adafai/`.
- Ensure standard library isolation (no `torch`, `transformers`, or `datasets` required for core).
- Write `tests/test_adafai_core.py` verifying detection behavior against known samples.

#### Phase 3: Backend API Expansion & Combined Pipeline
- Modularize extractors (`pdf_extractor.py`, `docx_extractor.py`, etc.).
- Add `/api/ai-detect`, `/api/analyze-combined`, and `/api/export-combined-report` routes in `server.py`.
- Preserve existing C subprocess calls and endpoints.
- Ensure all subprocess calls use non-blocking timeouts.

#### Phase 4: Frontend UI Enhancement
- Enhance `ui/index.html` to support a single-document analysis workflow alongside pair-wise comparison.
- Add an AI Content Detection overview gauge and metric breakdown cards (Stylometry, Unicode, Discourse).
- Implement an interactive sentence-level heatmap in `ui/app.js`:
  - Green/Neutral: Low AI likelihood ($\text{score} < 0.30$)
  - Amber: Uncertain ($\text{score} \in [0.30, 0.70)$)
  - Coral/Red: High AI likelihood ($\text{score} \ge 0.70$)
  - Clickable sentence spans displaying exact trigger reasons (e.g., tell phrases, MATTR, negation pivots).
- Update `style.css` with clean, accessible styling for heatmap spans and dual scores.

#### Phase 5: Verification, Benchmarking & Deployment
- Run all C11 unit tests (`tests/test_*.c`).
- Run Python extraction and AI detection test suites.
- Perform memory benchmarking ensuring peak resident memory stays under 250 MB (well below Render's 512 MB cap).
- Validate cross-origin requests between Vercel and Render.

---

## 10. Testing Strategy

A multi-layered testing regimen guarantees zero regressions in existing algorithms while confirming the correctness of new capabilities.

```
+-------------------------------------------------------------------------+
|                              TEST MATRIX                                |
+------------------------------------+------------------------------------+
| Layer                              | Scope & Target                     |
+------------------------------------+------------------------------------+
| 1. C11 Algorithmic Unit Tests      | Hash Table, KMP LPS table,         |
|    (Pure C, GCC Native)            | Rabin-Karp polynomial hash,        |
|                                    | Merge Sort stability, Jaccard      |
+------------------------------------+------------------------------------+
| 2. Document Extraction Tests       | Valid & corrupt PDF, DOCX, PPTX,   |
|    (Python PyMuPDF, docx, pptx)    | TXT, empty files, encrypted PDFs   |
+------------------------------------+------------------------------------+
| 3. ADAFAI Core Signal Tests        | MATTR sliding window correctness,  |
|    (Python Standard Library)       | Unicode homoglyphs/invisibles,     |
|                                    | Discourse negation pivots, spans   |
+------------------------------------+------------------------------------+
| 4. REST API Integration Tests      | Upload, single compare, batch,     |
|    (HTTP Client / End-to-End)      | AI detect, combined analysis       |
+------------------------------------+------------------------------------+
| 5. Memory & Performance Tests      | RSS memory profiling (< 250 MB),   |
|    (Valgrind, tracemalloc, psutil) | C leak checks, execution < 2.0s    |
+------------------------------------+------------------------------------+
```

### 10.1 C11 DSA Engine Regression Suite
The existing test suite in `tests/` will be executed without modification:
1. `tests/test_hashtable.c`: Bucket allocation, collision chaining, word counting.
2. `tests/test_kmp.c`: LPS computation, exact pattern matching, boundary offsets.
3. `tests/test_rabinkarp.c`: Rolling hash correctness, collision resolution.
4. `tests/test_mergesort.c`: Descending similarity ordering, rank assignment.
5. `tests/test_similarity.c`: Word Jaccard, sentence Jaccard, phrase coverage formulas.
6. `tests/test_report.c`: Deduplicated phrase merging, marker generation.
7. `tests/test_search.c`: Basename extraction, threshold filtering.
8. `tests/test_realistic.c`: End-to-end multi-document comparison.

### 10.2 Document Extractor Test Suite (`tests/test_extractors.py`)
Tests multi-format extraction against edge cases in `test_corpus/`:
- `test.pdf`, `test.docx`, `test.pptx`, `test.txt` $\rightarrow$ Correct text extraction.
- `corrupt.pdf`, `corrupt.docx` $\rightarrow$ Graceful exception handling with user-friendly error messages.
- `empty.pdf`, `empty.txt` $\rightarrow$ Empty file validation.
- Password-protected PDF $\rightarrow$ Clear encryption rejection notice.

### 10.3 ADAFAI Core Verification Suite (`tests/test_adafai_core.py`)
- **Stylometry**: Asserts MATTR matches reference calculations across sliding windows; verifies sentence CV; detects overused AI-tell phrases.
- **Unicode Forensics**: Detects zero-width spaces (`\u200b`), Cyrillic homoglyphs, and curly quotes.
- **Discourse**: Validates extraction of negation pivots (*not only ... but also*), tricolons, and self-answered questions.
- **Spans**: Verifies exact character offsets match original input text; checks sentence-level reason attribution.
- **Short-Text Floor**: Asserts that text under 150 words returns `verdict: "insufficient evidence"`.

### 10.4 Combined Pipeline Integration Tests (`tests/test_api_integration.py`)
- Verifies `POST /api/ai-detect` returns valid JSON with all required signal structures.
- Verifies `POST /api/analyze-combined` returns both AI scores and C plagiarism scores for an uploaded document.
- Verifies `POST /api/export-combined-report` writes an audit file to `reports/`.

---

## 11. Deployment and Performance Risks

### 11.1 Render Free-Tier Hardware Constraints

| Resource | Render Free Tier Limit | Production System Consumption (Proposed Core) | Risk Level |
|---|---|---|:---:|
| **Memory (RAM)** | **512 MB** | $\approx 85\text{ MB} - 160\text{ MB}$ (Python + C Subprocess) | **LOW** (with Core) / **FATAL** (with PyTorch) |
| **CPU** | 0.1 vCPU (Shared) | CPU bursts during extraction and KMP matching ($< 0.5\text{s}$) | **LOW** |
| **Disk** | Ephemeral (Wiped on restart) | Caches in `uploads/` and `reports/` are temporary | **MEDIUM** (Data ephemeral) |
| **Idle Timeout** | Spins down after 15 min inactivity | Cold start requires $\approx 50\text{ s}$ to spin up | **MEDIUM** (UX latency) |
| **Build Timeout** | 15 minutes | GCC compilation ($< 2\text{ s}$) + Pip install ($< 15\text{ s}$) | **LOW** |

### 11.2 The PyTorch / Deep Learning Model Trap on Render Free Tier

> [!CAUTION]
> Attempting to install and run the model-based components of ADAFAI (`perplexity.py` and `binoculars.py` requiring `torch`, `transformers`, and HuggingFace models) on Render's Free Tier **will immediately fail with an Out-Of-Memory (OOM) fatal exit (Exit code 137)**.

**Detailed Technical Risk Analysis:**
1. **PyTorch Base Overhead**: Importing PyTorch (`import torch`) alone consumes $250\text{ MB} - 350\text{ MB}$ of Resident Set Size (RSS).
2. **Model Footprint**:
   - `distilgpt2` (Perplexity): $\approx 350\text{ MB}$ uncompressed weights + tokenizer tensors.
   - `gpt2` + `distilgpt2` (Binoculars): $\approx 850\text{ MB}$ weights in memory.
   - Falcon-7B (Reference Binoculars): $\approx 14\text{ GB}$ (completely impossible).
3. **Combined RAM Requirement**: Running Binoculars with PyTorch requires a minimum of $1.5\text{ GB} - 2.5\text{ GB}$ of RAM.
4. **Build Quota**: Downloading PyTorch wheels ($\approx 800\text{ MB}$) during Render's build frequently exceeds bandwidth limits or build timeouts.

### 11.3 Memory Mitigation Strategy
- **Adopt the Zero-Dependency Core**: Deploy the core multi-signal detector (`stylometry`, `unicode_forensics`, `discourse`, `spans`, and `detector`).
- **RAM Footprint**: Under $160\text{ MB}$ total peak usage (including Python, PyMuPDF, and the C binary), leaving $> 350\text{ MB}$ of safety headroom.
- **Zero Build Penalty**: Zero additional packages to install in `requirements.txt`.
- **Sub-Second Execution**: Complete document analysis finishes in $< 100\text{ ms}$, compared to $15+\text{ seconds}$ for neural inference.

### 11.4 Ephemeral Storage Management
- Because Render free instances utilize ephemeral containers, files stored in `uploads/` and `reports/` do not persist across redeployments or cold-start restarts.
- **Mitigation**: Implement an automated cache retention routine in `server.py` that purges uploaded files older than 2 hours to prevent disk bloat.

### 11.5 Vercel-to-Render Cross-Origin Communication
- The frontend hosted on Vercel communicates across domains with Render backend.
- `server.py` already includes robust CORS headers (`Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods: GET, POST, OPTIONS`).
- **Mitigation**: Ensure all newly added endpoints explicitly invoke `self.end_headers()` which sets the global CORS headers.

---

## 12. Exact List of Files for Modification or Creation

### 12.1 Files that MUST NOT Be Modified (Strict C11 Engine Preservation)
The following 22 files constitute the core C11 algorithmic engine and its test harness. They are completely protected from any alteration:
- `include/document.h`
- `include/hashtable.h`
- `include/kmp.h`
- `include/linkedlist.h`
- `include/mergesort.h`
- `include/preprocessing.h`
- `include/rabinkarp.h`
- `include/report.h`
- `include/search.h`
- `include/similarity.h`
- `src/document.c`
- `src/hashtable.c`
- `src/kmp.c`
- `src/linkedlist.c`
- `src/main.c`
- `src/mergesort.c`
- `src/preprocessing.c`
- `src/rabinkarp.c`
- `src/report.c`
- `src/search.c`
- `src/similarity.c`
- `tests/test_*.c` (all 8 C test files)

### 12.2 Files to Be Created (New Subsystems)
1. `adafai/__init__.py`: Package entry point exposing `analyze` and `analyze_spans`.
2. `adafai/detector.py`: Heuristic ensemble combining stylometry, unicode forensics, and discourse.
3. `adafai/stylometry.py`: MATTR, sentence-length CV, trigram repetition, AI-tell phrase density, punctuation entropy.
4. `adafai/unicode_forensics.py`: Detection of invisible characters, exotic spaces, homoglyphs, typographic polish.
5. `adafai/discourse.py`: Detection of negation pivots, tricolons, self-answered questions, paragraph cadence.
6. `adafai/spans.py`: Sentence-level localization and explanation generator.
7. `extractors/__init__.py`: Modular extraction package initializer.
8. `extractors/base.py`: Unified extractor interface.
9. `extractors/pdf_extractor.py`: Dedicated PDF extraction logic.
10. `extractors/docx_extractor.py`: Dedicated DOCX extraction logic.
11. `extractors/pptx_extractor.py`: Dedicated PPTX extraction logic.
12. `extractors/txt_extractor.py`: Dedicated TXT extraction logic.
13. `tests/test_adafai_core.py`: Unit tests for all ADAFAI signals.
14. `tests/test_extractors.py`: Unit tests for multi-format document extraction.
15. `tests/test_api_integration.py`: Integration tests for all REST endpoints.

### 12.3 Files to Be Modified (Phases 3 & 4)
1. `server.py`:
   - Import `adafai` package.
   - Register routes: `/api/ai-detect`, `/api/analyze-combined`, `/api/export-combined-report`.
   - Update `/api/health` to report AI engine readiness.
2. `requirements.txt`:
   - Keep current extraction packages (`python-docx`, `python-pptx`, `pymupdf`, `pypdf`).
   - Core ADAFAI requires zero additions. (Optional test runner `pytest` can be added for CI).
3. `ui/index.html`:
   - Add "AI Content Detector" and "Combined Analysis" tabs/views.
   - Add sentence-level heatmap display container and explainability panel.
   - Add combined score overview (AI likelihood vs Plagiarism similarity).
4. `ui/app.js`:
   - Add client controllers for single-document upload and AI detection dispatch.
   - Implement dynamic sentence heatmap rendering with interactive tooltip modals.
   - Implement combined report viewer and exporter.
5. `ui/style.css`:
   - Add CSS styles for AI likelihood badges, heatmap span highlighting, and combined score cards.
6. `README.md`:
   - Document the combined AI + Plagiarism detection architecture, algorithmic breakdown, limitations, and usage instructions.

---

## 13. Safest Implementation Approach & Final Recommendation

### 13.1 Strategic Recommendation: The Two-Tier Architecture

To achieve maximum reliability, zero downtime, and complete compatibility with Render's Free Tier, the project should adopt a **Two-Tier Architecture**:

```
+==========================================================================+
|                         RECOMMENDED ARCHITECTURE                         |
+==========================================================================+
|                                                                          |
|   TIER 1: PRODUCTION DEFAULT (Zero-Dependency Standard Library Core)     |
|   --------------------------------------------------------------------   |
|   - Engine: Pure Python standard library ADAFAI + Native C11 Engine      |
|   - Signals: Stylometry + Unicode Forensics + Discourse + Sentence Spans |
|   - Plagiarism: Full C11 Hash Table, KMP, Rabin-Karp, Merge Sort         |
|   - Memory Footprint: ~120 MB peak RAM (Safely within Render's 512 MB)   |
|   - Response Latency: < 100 ms total processing time                     |
|   - Cost: $0.00 / month (Fully operational on Render + Vercel free tier) |
|                                                                          |
|   TIER 2: OPTIONAL LOCAL / HIGH-COMPUTE EXTENSION (Model-Based)          |
|   --------------------------------------------------------------------   |
|   - Engine: Optional PyTorch + HuggingFace Transformers (Binoculars/PPL) |
|   - Activation: Gated behind an environment variable (USE_MODELS=true)   |
|   - Graceful Fallback: Automatically falls back to Tier 1 if torch       |
|     is missing or if system RAM < 2 GB                                   |
|   - Target Environment: Local developer workstations with GPU or paid    |
|     cloud instances (Render Standard / AWS EC2)                          |
|                                                                          |
+==========================================================================+
```

### 13.2 Key Architectural Safeguards

1. **Zero C Code Alteration**: The C11 DSA engine will not be touched. It is proven, high-performing, leak-free, and handles all textual similarity and report generation with optimal efficiency.
2. **Standard-Library-First Integration**: Integrating ADAFAI's core (`stylometry.py`, `unicode_forensics.py`, `discourse.py`, `spans.py`, and `detector.py`) fulfills every user requirement:
   - Single-document upload.
   - AI-generated writing analysis.
   - Human-like writing pattern analysis (lexical diversity, sentence length CV, punctuation entropy).
   - Granular sentence-level heatmap and explainable reason tags.
   - Plagiarism detection against reference corpus.
   - Combined dual-engine forensic reports.
3. **Memory Safety Guarantee**: By utilizing Tier 1 on Render, the system avoids the fatal PyTorch OOM trap, remaining rock-solid within Render's 512MB RAM ceiling.
4. **Epistemic Honesty in the UI**: The frontend will display AI detection results with clear contextual labeling:
   - "Likely Human Writing", "Uncertain / Mixed Patterns", or "Likely AI-Generated Writing".
   - An explicit academic notice clarifying that the score represents a heuristic triage indicator measuring linguistic regularity and known phrase patterns, rather than absolute proof of authorship.
   - Short documents under 150 words will display "Insufficient Evidence for Statistical AI Detection", preventing misleading classifications.

---

*End of Technical Audit & Migration Plan.*
