# Plagiarism Detection and Document Similarity Analysis System

## 1. Project Title
**Plagiarism Detection and Document Similarity Analysis Engine in Pure C (C11)**  
A high-performance, memory-safe data structures and algorithms project designed to analyze textual overlap, identify plagiarized content, rank candidate documents, and generate forensic forensic comparison reports without external dependencies.

---

## 2. Problem Statement
Academic institutions and software organizations face significant challenges in detecting unauthorized content duplication, text recycling, and academic dishonesty. Existing solutions frequently rely on proprietary cloud APIs, heavyweight machine learning models, or external libraries. There is a need for an efficient, transparent, and deterministic plagiarism detection engine written in standard C that demonstrates fundamental Computer Science principles, optimal algorithmic complexity, and safe low-level memory management.

---

## 3. Objectives
- Implement foundational Computer Science data structures (Hash Tables, Dynamic Arrays, Linked Lists) completely from scratch.
- Apply high-efficiency string matching algorithms (Knuth-Morris-Pratt and Rabin-Karp) to detect continuous and fragmented phrase overlap.
- Design a mathematically rigorous multi-metric similarity model combining word-level vocabulary overlap, sentence-level matching, and longest common phrase coverage.
- Implement a stable $O(N \log N)$ sorting algorithm (Merge Sort) to rank corpus documents by similarity without using C standard library `qsort()`.
- Implement fast linear and threshold searching across ranked document arrays.
- Provide comprehensive forensic reporting with match boundary markers (`>>> MATCH <<< ... >>> END MATCH <<<`) and file export capabilities.
- Guarantee memory safety (zero memory leaks, safe pointer reallocations, rigorous NULL checks) and adherence to strict compiler standards.

---

## 4. Features
- **Multi-Phase Text Preprocessing:** Case folding, punctuation normalization, sentence boundary detection, and dynamic tokenization.
- **Hash Table Word Frequency Engine:** Separate chaining collision resolution with the `djb2` hash algorithm and frequency-sorted top-word extraction.
- **Dual String Matching Suite:**
  - **Knuth-Morris-Pratt (KMP):** Preprocesses patterns into a Longest Prefix Suffix (LPS) array for $O(N + M)$ guaranteed worst-case search.
  - **Rabin-Karp (RK):** Employs a polynomial rolling hash with modular arithmetic for efficient substring searching.
- **Hybrid Similarity Metric:**
  - Word Similarity (Jaccard Index): 30% weight
  - Sentence Similarity (Exact Sentence Jaccard Index): 30% weight
  - Phrase Similarity (Coverage of matched phrases $\ge 3$ words): 40% weight
- **Batch Corpus Comparison & Ranking:** Analyzes a reference document against an entire directory of documents, sorting results via custom Merge Sort.
- **Result Querying & Filtering:** Search ranked candidates by document filename or similarity threshold percentage ($\ge X\%$).
- **Exportable Plagiarism Reports:** Generates structured textual reports complete with similarity classification, document statistics, and matched phrase highlights.
- **Overwrite Protection:** Interactive prompt before overwriting existing report files to prevent accidental data loss.

---

## 5. Technologies
- **Language:** Pure Standard C (C11 standard)
- **Compiler:** GCC 16.1.0 / MinGW-w64 with strict flags (`-O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -std=c11`)
- **Libraries:** Standard C Library only (`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`, `math.h`, `dirent.h`, `assert.h`)
- **Platform:** Cross-Platform (Tested and validated on Windows and Linux)

---

## 6. Data Structures Used

### 1. Dynamic Arrays (`TokenArray`, `SentenceArray`, `MatchArray`, `PhraseMatchArray`, `RankedDocumentArray`)
- **Description:** Dynamically allocated contiguously indexed arrays with automated geometric capacity resizing ($2 \times$ scale factor).
- **Memory Management:** Employs temporary pointer verification (`void *tmp = realloc(...)`) to guarantee pointer preservation upon allocation failure.

### 2. Singly Linked Lists (`HashEntry`)
- **Description:** Form the collision buckets of the hash table.
- **Structure:** Each node dynamically allocates a word string, an integer occurrence counter, and a pointer to the subsequent node (`next`).

### 3. Hash Table with Separate Chaining (`HashTable`)
- **Description:** Fixed-bucket array of linked list heads for $O(1)$ expected word insertion, search, and frequency incrementation.
- **Hashing:** Implements Daniel J. Bernstein's `djb2` hash function.

### 4. Merge Sort Auxiliary Buffer
- **Description:** Contiguous heap buffer allocated during the merge step to preserve stability and avoid stack overflows on large datasets.

---

## 7. Algorithms Used

### 1. `djb2` String Hashing
- **Equation:** $hash_{i} = (hash_{i-1} \times 33) \oplus c$
- **Properties:** Exceptional distribution properties and minimal collision rates for natural language vocabularies.

### 2. Knuth-Morris-Pratt (KMP) Algorithm
- **Mechanism:** Builds a $\pi$ (LPS) lookup table representing the length of the longest proper prefix that is also a suffix. Bypasses redundant character comparisons when mismatches occur.
- **Usage:** Identifies verbatim phrase occurrences between candidate token streams.

### 3. Rabin-Karp (RK) Algorithm
- **Mechanism:** Computes a polynomial rolling hash:
  $$H(s) = \left( \sum_{i=0}^{M-1} s[i] \cdot B^{M - 1 - i} \right) \pmod Q$$
  where $B = 256$ and $Q = 1000000007$. Shifts window in $O(1)$ time and validates matches to handle spurious hash collisions.

### 4. Merge Sort (Recursive Divide and Conquer)
- **Mechanism:** Recursively subdivides document similarity records into halves, sorts sub-lists, and merges them in descending similarity order.
- **Properties:** Stable sort with guaranteed $O(N \log N)$ worst-case time complexity.

### 5. Jaccard Similarity Index
- **Formulas:**
  $$J_{word}(D_1, D_2) = \frac{|V(D_1) \cap V(D_2)|}{|V(D_1) \cup V(D_2)|} \times 100$$
  $$J_{sentence}(D_1, D_2) = \frac{|S(D_1) \cap S(D_2)|}{|S(D_1) \cup S(D_2)|} \times 100$$

### 6. Phrase Coverage Calculation
- **Formula:**
  $$Coverage(D_1, D_2) = \frac{\max(|CoveredWords(D_1)|, |CoveredWords(D_2)|)}{\max(|Words(D_1)|, |Words(D_2)|)} \times 100$$
  Overlapping phrase matches are flattened into a unique token coverage bitmap to prevent duplicate counting.

---

## 8. System Workflow

```
[Raw Document File] 
       │
       ▼
[Document Reader] ──> Dynamic Buffer Allocation & Length Calculation
       │
       ▼
[Text Preprocessor] ──> Normalization (Lowercase, Punctuation Removal, Whitespace Squashing)
       │
       ├─► [Tokenization Engine] ──> TokenArray (Word Tokens)
       └─► [Sentence Splitter]   ──> SentenceArray (Normalized Sentences)
               │
               ▼
       [djb2 Hash Table] ──> Word Frequency Table & Unique Vocabulary
               │
               ▼
[Plagiarism Engine] ──> KMP & Rabin-Karp Phrase Extraction (Min 3 Words)
       │
       ├─► Word Jaccard (30%)
       ├─► Sentence Jaccard (30%)
       └─► Phrase Coverage (40%)
               │
               ▼
       [Composite Similarity Score] ──> Overall Plagiarism % & Level Classification
               │
               ▼
       [Corpus Analyzer] ──> Merge Sort (Descending Ranking)
               │
               ├─► [Search Module] (Threshold / Filename Querying)
               └─► [Report Generator] (Highlighting & File Export)
```

---

## 9. Similarity Calculation
The overall plagiarism percentage is computed through a calibrated weighted sum of three distinct linguistic layers:

$$\text{Overall Similarity} = 0.30 \times \text{Word Sim} + 0.30 \times \text{Sentence Sim} + 0.40 \times \text{Phrase Sim}$$

### Classification Thresholds:
- **80.0% – 100.0%:** Very High Similarity (Extensive direct copying / identical submission)
- **60.0% – 79.99%:** High Similarity (Substantial plagiarism or verbatim paragraph reuse)
- **40.0% – 59.99%:** Moderate Similarity (Noticeable overlap, shared quotations, partial revision)
- **20.0% – 39.99%:** Low Similarity (Minor topical vocabulary overlap)
- **0.0% – 19.99%:** Very Low Similarity (Independent original content)

---

## 10. Complexity Analysis

| Component / Operation | Data Structure / Algorithm | Time Complexity (Best / Worst) | Space Complexity |
|:---|:---|:---:|:---:|
| Document Ingestion | Dynamic Buffer | $O(N)$ / $O(N)$ | $O(N)$ |
| Text Preprocessing | Array In-Place Filter | $O(N)$ / $O(N)$ | $O(N)$ |
| Vocabulary Ingestion | Hash Table (`djb2`) | $O(W)$ / $O(W \cdot K)$ | $O(W)$ |
| Word Lookup | Hash Table | $O(1)$ average / $O(K)$ worst | $O(1)$ |
| LPS Precomputation | KMP Array | $O(M)$ / $O(M)$ | $O(M)$ |
| Pattern Matching | KMP Search | $O(N + M)$ / $O(N + M)$ | $O(M)$ |
| Pattern Matching | Rabin-Karp Search | $O(N + M)$ / $O(N \cdot M)$ | $O(1)$ |
| Document Ranking | Custom Merge Sort | $O(D \log D)$ / $O(D \log D)$ | $O(D)$ auxiliary |
| Linear Search | Sequential Scan | $O(D)$ / $O(D)$ | $O(1)$ |
| Report Generation | Token Highlighting Bitmap | $O(T)$ / $O(T)$ | $O(T)$ |

*Legend: $N$ = Document text length, $M$ = Pattern length, $W$ = Number of tokens, $K$ = Maximum bucket chain depth, $D$ = Number of documents in directory, $T$ = Total tokens in document.*

---

## 11. Project Structure

```
plagiarism-detector/
├── documents/                     # Sample and realistic evaluation datasets
│   ├── original.txt
│   ├── student1.txt ... student5.txt
│   ├── test_reference.txt         # 3-paragraph reference benchmark
│   ├── test_identical.txt         # Exact duplicate
│   ├── test_paraphrased.txt       # Concept-identical, reworded sentences
│   ├── test_partial.txt           # Verbatim paragraph copy + unrelated topic
│   ├── test_unrelated.txt         # Gastronomy corpus (unrelated baseline)
│   ├── test_repeated.txt          # Boundary condition duplicate phrases
│   └── test_long.txt              # Multi-paragraph systems architecture paper
├── include/                       # Modular C header definitions
│   ├── document.h                 # File reading and Document structure
│   ├── preprocessing.h            # Sanitization, tokenization, and sentence extraction
│   ├── hashtable.h                # djb2 Hash Table with separate chaining
│   ├── kmp.h                      # Knuth-Morris-Pratt pattern matching
│   ├── rabinkarp.h                # Rabin-Karp polynomial rolling hash search
│   ├── similarity.h               # Multi-metric similarity scoring engine
│   ├── mergesort.h                # Recursive Merge Sort and RankedDocumentArray
│   ├── search.h                   # Linear search and threshold filtering
│   └── report.h                   # Plagiarism report formatting and text highlighting
├── reports/                       # Generated forensic report exports
│   └── *.txt
├── src/                           # Complete C source implementations
│   ├── document.c
│   ├── preprocessing.c
│   ├── hashtable.c
│   ├── kmp.c
│   ├── rabinkarp.c
│   ├── similarity.c
│   ├── mergesort.c
│   ├── search.c
│   ├── report.c
│   └── main.c                     # Interactive CLI user application
├── tests/                         # Full automated unit test suite
│   ├── test_hashtable.c
│   ├── test_kmp.c
│   ├── test_rabinkarp.c
│   ├── test_similarity.c
│   ├── test_mergesort.c
│   ├── test_search.c
│   ├── test_report.c
│   └── test_realistic.c           # Full paragraph-scale realistic test suite
├── plagiarism_detector.exe        # Production binary compiled with strict flags
└── README.md                      # Comprehensive technical documentation
```

---

## 12. Installation / Compilation

### Prerequisites
- GCC Compiler (v7.0 or newer supporting `-std=c11`)
- Make or standard terminal shell (PowerShell / Command Prompt / Bash)

### Compilation Command
Compile the complete application using strict optimization and warning flags:
```bash
gcc -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -std=c11 -Iinclude -o plagiarism_detector.exe src/*.c
```

All source files compile cleanly with **0 errors and 0 warnings**.

---

## 13. How to Run
Execute the compiled binary from the project directory:
```bash
./plagiarism_detector.exe
```

### Main Application Menu
```
============================================================
       PLAGIARISM DETECTION & SIMILARITY ANALYSIS
============================================================
1. Analyze Document Word Frequency (Phase 1 & 2)
2. Search Phrase using KMP Algorithm (Phase 3)
3. Search Phrase using Rabin-Karp Algorithm (Phase 3)
4. Compare Two Documents (Phase 4 & 6 Report)
5. Compare Document Against Directory & Rank (Phase 5)
6. Search in Ranked Results (Phase 5)
7. View Last Generated Plagiarism Report (Phase 6)
8. Exit
============================================================
Enter choice (1-8):
```

---

## 14. Example Usage

### Comparing Two Documents
```
Enter choice (1-8): 4

Enter Document 1 path:
documents/test_reference.txt
Enter Document 2 path:
documents/test_partial.txt

============================================================
PLAGIARISM DETECTION REPORT
============================================================

Reference Document:
test_reference.txt

Compared Document:
test_partial.txt

---

## OVERALL RESULT

Similarity Score:  23.52%

Similarity Level: Low Similarity

---

## ANALYSIS BREAKDOWN

Word Similarity:        20.93%
Sentence Similarity:    20.00%
Phrase Similarity:      28.92%

---

## DOCUMENT STATISTICS

Reference Words:       95
Compared Words:        83

Reference Sentences:   6
Compared Sentences:    6

Common Unique Words:   18
Matching Sentences:    2
Matching Phrases:      1
Matching Phrase Words: 24

---

## DETECTED MATCHING PHRASES

1. [KMP] Length: 24 words
   Ref Pos: 0 | Comp Pos: 0
   Phrase: "data structures and algorithms form the essential foundation of modern computer science and software development efficient algorithms optimize computational runtime and minimize physical memory consumption across distributed systems"

---

## HIGHLIGHTED COMPARED TEXT

>>> MATCH <<< data structures and algorithms form the essential foundation of modern computer science and software development efficient algorithms optimize computational runtime and minimize physical memory consumption across distributed systems >>> END MATCH <<< planetary orbits follow elliptical paths around celestial focal points according to classical gravitational mechanics spectroscopic observation of distant stellar atmospheres reveals hydrogen absorption lines and chemical compositions astronomers calibrate optical telescopes using adaptive mirrors to compensate for turbulent atmospheric distortion cosmic microwave background radiation provides empirical evidence regarding the thermal expansion of the early universe 

============================================================

Do you want to save this report to a file? (y/n): y
Report saved successfully to 'reports/test_partial_report.txt'.
```

---

## 15. Testing

The project includes eight independent unit test suites verifying every component from hash collisions to mathematical symmetry.

### Compiling and Running Unit Tests

#### 1. Hash Table Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_ht.exe tests/test_hashtable.c src/hashtable.c src/preprocessing.c
./test_ht.exe
```

#### 2. KMP String Matching Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_kmp.exe tests/test_kmp.c src/kmp.c src/preprocessing.c
./test_kmp.exe
```

#### 3. Rabin-Karp String Matching Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_rk.exe tests/test_rabinkarp.c src/rabinkarp.c src/kmp.c src/preprocessing.c
./test_rk.exe
```

#### 4. Similarity Engine Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_sim.exe tests/test_similarity.c src/similarity.c src/hashtable.c src/kmp.c src/preprocessing.c
./test_sim.exe
```

#### 5. Merge Sort Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_ms.exe tests/test_mergesort.c src/mergesort.c
./test_ms.exe
```

#### 6. Search & Filter Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_search.exe tests/test_search.c src/search.c src/mergesort.c
./test_search.exe
```

#### 7. Report Generation Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_rep.exe tests/test_report.c src/report.c src/similarity.c src/hashtable.c src/kmp.c src/rabinkarp.c src/search.c src/mergesort.c src/document.c src/preprocessing.c
./test_rep.exe
```

#### 8. Realistic Multi-Paragraph Document Test Suite
```bash
gcc -O2 -Wall -Wextra -std=c11 -Iinclude -o test_real.exe tests/test_realistic.c src/document.c src/preprocessing.c src/hashtable.c src/kmp.c src/rabinkarp.c src/similarity.c src/mergesort.c src/search.c src/report.c
./test_real.exe
```

---

## 16. Limitations
- **Plaintext Ingestion:** The current implementation reads plain ASCII and UTF-8 encoded text files; proprietary document formats (`.pdf`, `.docx`) require external text extraction before ingestion.
- **Exact & Substring Matching:** Sentence and phrase matching algorithms prioritize verbatim and partial phrase overlap; heavy synonym substitution and structural reorganization without common n-grams may yield lower phrase scores despite semantic similarity.
- **In-Memory Operation:** Candidate document sets are maintained in heap memory during corpus comparison, which is optimal for small to medium repositories (thousands of documents) but requires chunked disk streaming for web-scale datasets.

---

## 17. Future Scope
- **Vector Embeddings & Semantic Similarity:** Integrate cosine similarity based on TF-IDF vectors or word embeddings to detect synonym-heavy paraphrasing.
- **File Format Parsing:** Native parsing support for PDF, Microsoft Word (`.docx`), and Markdown AST representations.
- **Parallel Multi-Core Execution:** Utilize OpenMP or POSIX threads (`pthreads`) to execute parallel KMP and Rabin-Karp searches across large document directories.
- **Persistent Database Backend:** SQLite / key-value document store indexing for persistent fingerprint storage across large institutional archives.

---

## 18. Conclusion
The **Plagiarism Detection and Document Similarity Analysis System** showcases how core Computer Science data structures and classical string algorithms can be synthesized into a practical, industrial-grade analysis tool. By combining custom hash tables, the Knuth-Morris-Pratt and Rabin-Karp algorithms, recursive Merge Sort, and a balanced multi-metric similarity model, the engine delivers deterministic, explainable, and performant plagiarism analysis in pure C with zero memory leaks and strict compiler compliance.
