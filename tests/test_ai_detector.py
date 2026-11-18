"""test_ai_detector.py - Unit test suite for the isolated ADAFAI lightweight AI detection adapter.

Tests cover:
- Empty and whitespace-only text
- Non-string input
- Short text below the 150-word floor
- Longer human-style sample execution
- Longer AI-style sample execution
- Sentence-level character offsets, spans, and explainable reason tags
- Strict JSON serializability
- Missing dependency graceful error handling
"""

import json
import os
import sys
import unittest
from unittest.mock import patch

# Ensure project root is in sys.path
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

from ai_detector.adafai_adapter import (
    ADAFAIAdapter,
    ADAFAIImportError,
    analyze_text,
)


class TestADAFAIAIAdapter(unittest.TestCase):
    """Test cases for the isolated ADAFAI AI detector adapter."""

    def setUp(self):
        self.adapter = ADAFAIAdapter()

        # Sample human-written prose (> 150 words)
        self.human_sample = (
            "So I decided to spend last weekend trying to fix the plumbing under my kitchen sink. "
            "It was leaking for weeks and making this annoying dripping sound every night. "
            "I went down to the local hardware store, bought a wrench and some replacement pipes, "
            "and thought it would take maybe twenty minutes tops. Boy was I wrong about that! "
            "As soon as I unscrewed the main joint, dirty water sprayed everywhere across the cabinets. "
            "I spent two hours cleaning up the mess with old bath towels and cursing my overconfidence. "
            "Eventually my neighbour came over, saw the catastrophe, and lent me some plumber's tape. "
            "We finally managed to get the seal tight by Sunday afternoon. "
            "After that ordeal, we celebrated with some homemade burgers and watched the football match. "
            "My hands were sore for two days straight, but at least the sink is dry now. "
            "Next time something breaks in the kitchen, I am definitely calling a professional instead."
        )

        # Sample AI-style prose (> 150 words)
        self.ai_sample = (
            "In today's fast-paced digital era, it is important to note that artificial intelligence "
            "plays a pivotal role across modern society. Furthermore, this cutting-edge technology "
            "will delve into the intricate complexities of global industries. Moreover, researchers "
            "consistently underscore the robust benefits of holistic automation. "
            "At its core, this paradigm shift stands as a testament to human ingenuity and relentless progress. "
            "To navigate the complexities of machine learning, organizations must leverage innovative tools "
            "and facilitate seamless data integration. Notably, the rich tapestry of neural computing offers "
            "a plethora of opportunities for forward-thinking leaders. In conclusion, the interplay between "
            "human talent and technological prowess will undoubtedly illuminate new horizons. "
            "In essence, artificial intelligence is not merely a tool for efficiency, but rather a catalyst "
            "for profound societal transformation. It is imperative that we unlock its full potential responsibly. "
            "By fostering collaborative ecosystems, modern institutions can successfully harness the transformative "
            "capabilities of algorithmic systems to build a more equitable and dynamic technological landscape."
        )

    def test_engine_availability(self):
        """Verify that ADAFAI core is successfully located and available."""
        self.assertTrue(
            self.adapter.is_available(),
            f"ADAFAI should be available, error was: {self.adapter.get_import_error()}"
        )

    def test_empty_text(self):
        """Verify handling of completely empty text string."""
        res = analyze_text("")
        self.assertEqual(res["engine"], "ADAFAI")
        self.assertEqual(res["mode"], "lightweight")
        self.assertEqual(res["status"], "empty_input")
        self.assertEqual(res["word_count"], 0)
        self.assertIsNone(res["overall_assessment"])
        self.assertEqual(res["signals"], {})
        self.assertEqual(res["sentence_analysis"], [])
        self.assertEqual(res["sentence_count"], 0)
        self.assertIsInstance(res["limitations"], list)
        self.assertTrue(len(res["limitations"]) > 0)

    def test_whitespace_only_text(self):
        """Verify handling of whitespace-only input (spaces, tabs, newlines)."""
        res = analyze_text("   \n\t   \r\n   ")
        self.assertEqual(res["status"], "empty_input")
        self.assertEqual(res["word_count"], 0)
        self.assertIsNone(res["overall_assessment"])
        self.assertEqual(res["signals"], {})
        self.assertEqual(res["sentence_analysis"], [])

    def test_none_input(self):
        """Verify non-string input handling."""
        res = analyze_text(None)
        self.assertEqual(res["status"], "invalid_input")
        self.assertIn("error", res)
        self.assertIsNone(res["overall_assessment"])

    def test_below_minimum_word_threshold(self):
        """Verify that text under 150 words receives an 'insufficient_text' status."""
        short_text = "This is a short sample text with only a few words to test the threshold."
        res = analyze_text(short_text)
        
        self.assertEqual(res["status"], "insufficient_text")
        self.assertLess(res["word_count"], 150)
        self.assertIsNotNone(res["overall_assessment"])
        self.assertFalse(res["overall_assessment"]["is_sufficient_length"])
        self.assertEqual(res["overall_assessment"]["verdict"], "insufficient evidence")
        self.assertIsInstance(res["overall_assessment"]["score"], float)
        # Even with short text, signals and sentence spans should still be present
        self.assertIn("stylometry", res["signals"])
        self.assertIn("unicode", res["signals"])
        self.assertIn("discourse", res["signals"])
        self.assertGreater(len(res["sentence_analysis"]), 0)

    def test_longer_human_sample_execution(self):
        """Verify execution and structure for a human-written passage > 150 words."""
        res = analyze_text(self.human_sample)
        
        self.assertEqual(res["engine"], "ADAFAI")
        self.assertEqual(res["mode"], "lightweight")
        self.assertEqual(res["status"], "completed")
        self.assertGreaterEqual(res["word_count"], 150)
        
        assessment = res["overall_assessment"]
        self.assertIsNotNone(assessment)
        self.assertTrue(assessment["is_sufficient_length"])
        self.assertIn(assessment["verdict"], ["likely human", "uncertain", "likely AI-generated"])
        self.assertGreaterEqual(assessment["score"], 0.0)
        self.assertLessEqual(assessment["score"], 1.0)
        
        signals = res["signals"]
        self.assertIn("stylometry", signals)
        self.assertIn("unicode", signals)
        self.assertIn("discourse", signals)
        
        sty = signals["stylometry"]
        self.assertIn("lexical_diversity_mattr", sty)
        self.assertIn("sentence_length_cv", sty)
        self.assertIn("punctuation_entropy", sty)
        self.assertIn("ai_phrase_density_per_1000w", sty)

    def test_longer_ai_sample_execution(self):
        """Verify execution and structure for an AI-patterned passage > 150 words."""
        res = analyze_text(self.ai_sample)
        
        self.assertEqual(res["status"], "completed")
        self.assertGreaterEqual(res["word_count"], 150)
        
        assessment = res["overall_assessment"]
        self.assertIsNotNone(assessment)
        self.assertTrue(assessment["is_sufficient_length"])
        self.assertGreaterEqual(assessment["score"], 0.0)
        self.assertLessEqual(assessment["score"], 1.0)
        
        # Verify AI tell phrase density is captured
        phrase_density = res["signals"]["stylometry"]["ai_phrase_density_per_1000w"]
        self.assertGreater(phrase_density, 0.0)

    def test_sentence_offsets_and_reasons(self):
        """Verify character offsets map to original text and reasons are populated."""
        sample = (
            "First sentence is completely normal. "
            "Furthermore, we must delve into the intricate tapestry of modern learning. "
            "Third sentence concludes the experiment."
        )
        res = analyze_text(sample)
        
        sentences = res["sentence_analysis"]
        self.assertEqual(len(sentences), 3)
        
        for s in sentences:
            start = s["start"]
            end = s["end"]
            self.assertIsInstance(start, int)
            self.assertIsInstance(end, int)
            self.assertGreater(end, start)
            
            # Verify slice corresponds to the sentence text
            sliced = sample[start:end].strip()
            self.assertTrue(sliced.startswith(s["text"][:15]))
            
            # Check reasons list
            self.assertIsInstance(s["reasons"], list)
            self.assertGreater(len(s["reasons"]), 0)

        # Check that the sentence containing 'delve' has reasons tagging it
        delve_sentence = sentences[1]
        self.assertTrue(
            any("delve" in r or "tapestry" in r for r in delve_sentence["reasons"]),
            f"Reasons should identify tell phrases: {delve_sentence['reasons']}"
        )

    def test_json_serialization(self):
        """Ensure all outputs are 100% JSON-serializable."""
        samples = ["", "Short snippet.", self.human_sample, self.ai_sample]
        for text in samples:
            res = analyze_text(text)
            try:
                serialized = json.dumps(res)
                deserialized = json.loads(serialized)
                self.assertEqual(deserialized["status"], res["status"])
                self.assertEqual(deserialized["word_count"], res["word_count"])
            except (TypeError, ValueError) as err:
                self.fail(f"JSON serialization failed: {err}")

    def test_missing_dependency_graceful_handling(self):
        """Verify graceful error reporting when ADAFAI is not importable."""
        with patch("ai_detector.adafai_adapter._ADAFAI_AVAILABLE", False), \
             patch("ai_detector.adafai_adapter._ADAFAI_IMPORT_ERROR", "ModuleNotFoundError: No module named 'adafai'"):
            
            # Default behavior: returns error status without throwing
            res = self.adapter.analyze("Some valid document text.")
            self.assertEqual(res["status"], "error")
            self.assertIn("error", res)
            self.assertIn("unavailable", res["error"])
            self.assertIsNone(res["overall_assessment"])
            self.assertEqual(res["signals"], {})
            self.assertEqual(res["sentence_analysis"], [])
            
            # With raise_on_missing_import=True: raises ADAFAIImportError
            strict_adapter = ADAFAIAdapter(raise_on_missing_import=True)
            with self.assertRaises(ADAFAIImportError):
                strict_adapter.analyze("Some valid document text.")


if __name__ == "__main__":
    unittest.main()
