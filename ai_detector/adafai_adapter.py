"""adafai_adapter.py - Isolated adapter for the lightweight ADAFAI core engine.

This adapter bridges raw extracted document text to ADAFAI's standard-library
heuristics (stylometry, unicode forensics, discourse structure, and sentence-level
spans). It operates strictly in lightweight mode without neural models or GPU
dependencies, ensuring 100% compatibility with constrained deployment environments.
"""

from __future__ import annotations

import json
import os
import sys
from typing import Any, Dict, List, Optional

# Ensure project root is in sys.path so 'adafai' package can be imported reliably
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

_ADAFAI_AVAILABLE = False
_ADAFAI_IMPORT_ERROR: Optional[str] = None

try:
    import adafai
    from adafai.detector import analyze as adafai_analyze
    from adafai.spans import analyze_spans as adafai_analyze_spans
    _ADAFAI_AVAILABLE = True
except ImportError as err:
    _ADAFAI_AVAILABLE = False
    _ADAFAI_IMPORT_ERROR = str(err)


class ADAFAIImportError(ImportError):
    """Raised when ADAFAI modules cannot be imported."""
    pass


class ADAFAIAdapter:
    """Isolated, safe adapter wrapping ADAFAI's lightweight analysis capabilities."""

    ENGINE_NAME = "ADAFAI"
    MODE = "lightweight"
    MIN_WORDS_THRESHOLD = 150

    BASE_LIMITATIONS = [
        "Heuristic indicators do not prove AI authorship.",
        "Scores reflect statistical regularity, vocabulary patterns, and discourse heuristics, not mathematical certainty.",
        "Deterministic paraphrasing, human editing, and domain-specific vocabulary may alter heuristic signals.",
        "Never use this analysis as the sole basis for academic-integrity or disciplinary decisions."
    ]

    def __init__(self, raise_on_missing_import: bool = False):
        self.raise_on_missing_import = raise_on_missing_import

    @classmethod
    def is_available(cls) -> bool:
        """Returns True if the ADAFAI library is imported and ready for inference."""
        return _ADAFAI_AVAILABLE

    @classmethod
    def get_import_error(cls) -> Optional[str]:
        """Returns the error message if ADAFAI failed to import, or None."""
        return _ADAFAI_IMPORT_ERROR

    def analyze(self, text: Optional[str]) -> Dict[str, Any]:
        """Performs full lightweight AI detection analysis on the supplied text string.

        Args:
            text: Plain text extracted from a document.

        Returns:
            A normalized, JSON-serializable dictionary adhering to the system contract.
        """
        # 1. Dependency Check
        if not _ADAFAI_AVAILABLE:
            if self.raise_on_missing_import:
                raise ADAFAIImportError(
                    f"ADAFAI core package is not available: {_ADAFAI_IMPORT_ERROR}"
                )
            return self._build_error_response(
                status="error",
                error=f"ADAFAI engine is unavailable: {_ADAFAI_IMPORT_ERROR}"
            )

        # 2. Type validation
        if text is None or not isinstance(text, str):
            return self._build_error_response(
                status="invalid_input",
                error="Input text must be a non-null string."
            )

        # 3. Empty / Whitespace-only text handling
        cleaned = text.strip()
        if not cleaned:
            return {
                "engine": self.ENGINE_NAME,
                "mode": self.MODE,
                "status": "empty_input",
                "word_count": 0,
                "overall_assessment": None,
                "signals": {},
                "sentence_analysis": [],
                "sentence_count": 0,
                "flagged_sentences_count": 0,
                "limitations": self.BASE_LIMITATIONS + [
                    "Document contains no extractable text; analysis could not be conducted."
                ]
            }

        # 4. Invoke ADAFAI lightweight core functions
        # Strictly enforce use_models=False to prevent neural model downloads
        try:
            doc_result = adafai_analyze(text, use_models=False)
            spans_result = adafai_analyze_spans(text)
        except Exception as exc:
            return self._build_error_response(
                status="execution_error",
                error=f"ADAFAI analysis execution failed: {str(exc)}"
            )

        word_count = doc_result.get("word_count", 0)
        is_sufficient = word_count >= self.MIN_WORDS_THRESHOLD

        # 5. Format Sentence Analysis
        sentence_analysis: List[Dict[str, Any]] = []
        for i, s in enumerate(spans_result.get("sentences", [])):
            sentence_analysis.append({
                "sentence_index": i + 1,
                "start": s.get("start", 0),
                "end": s.get("end", 0),
                "text": s.get("text", ""),
                "word_count": s.get("word_count", 0),
                "score": s.get("score", 0.0),
                "low_confidence": s.get("low_confidence", False),
                "reasons": s.get("reasons", [])
            })

        # 6. Format Limitations
        limitations = list(self.BASE_LIMITATIONS)
        if not is_sufficient:
            limitations.append(
                f"{word_count} words is below the {self.MIN_WORDS_THRESHOLD}-word floor "
                "for a reliable statistical verdict; scores are reported as triage indications only."
            )

        # 7. Construct Overall Assessment
        overall_assessment = {
            "score": doc_result.get("score", 0.0),
            "verdict": doc_result.get("verdict", "unknown"),
            "is_sufficient_length": is_sufficient,
            "note": doc_result.get("note", "")
        }

        # 8. Assemble Normalized Response
        status = "completed" if is_sufficient else "insufficient_text"

        response: Dict[str, Any] = {
            "engine": self.ENGINE_NAME,
            "mode": self.MODE,
            "status": status,
            "word_count": word_count,
            "overall_assessment": overall_assessment,
            "signals": {
                "stylometry": doc_result.get("signals", {}).get("stylometry", {}),
                "unicode": doc_result.get("signals", {}).get("unicode", {}),
                "discourse": doc_result.get("signals", {}).get("discourse", {})
            },
            "sentence_analysis": sentence_analysis,
            "sentence_count": spans_result.get("sentence_count", len(sentence_analysis)),
            "flagged_sentences_count": spans_result.get("flagged", 0),
            "limitations": limitations
        }

        if "provenance_flags" in doc_result:
            response["provenance_flags"] = doc_result["provenance_flags"]

        # Ensure strict JSON serializability
        json.dumps(response)

        return response

    def _build_error_response(self, status: str, error: str) -> Dict[str, Any]:
        """Constructs a consistent error payload without fabricating scores."""
        return {
            "engine": self.ENGINE_NAME,
            "mode": self.MODE,
            "status": status,
            "error": error,
            "word_count": 0,
            "overall_assessment": None,
            "signals": {},
            "sentence_analysis": [],
            "sentence_count": 0,
            "flagged_sentences_count": 0,
            "limitations": list(self.BASE_LIMITATIONS)
        }


def analyze_text(text: Optional[str]) -> Dict[str, Any]:
    """Convenience functional wrapper around ADAFAIAdapter."""
    adapter = ADAFAIAdapter()
    return adapter.analyze(text)
