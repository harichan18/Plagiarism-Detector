"""test_ai_detect_api.py - Direct integration test suite for POST /api/ai-detect.

Validates the complete endpoint lifecycle in server.py:
1. Successful TXT document analysis.
2. Successful DOCX document analysis.
3. Successful PDF document analysis.
4. Successful PPTX document analysis.
5. Unsupported file extension rejection.
6. Empty document rejection (empty string and extractable text checks).
7. Document with fewer than 150 words (insufficient text threshold).
8. Missing upload parameters (missing filename or content).
9. Corrupt or unreadable document handling (corrupt PDF, corrupt DOCX).
10. ADAFAI adapter error handling (graceful 500/503 without leaking paths).
11. Preservation of existing plagiarism routes (GET /api/health, GET /api/sample-docs, POST /api/upload, POST /api/compare).
"""

import base64
import io
import json
import os
import sys
import unittest
from unittest.mock import patch

# Ensure project root is in sys.path
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

import server
from server import PlagiarismRequestHandler


class EndpointTestClient(PlagiarismRequestHandler):
    """Direct, socket-free test harness for PlagiarismRequestHandler.
    Simulates real HTTP requests and captures response codes and JSON bodies
    without opening network ports, guaranteeing fast and sandboxed execution.
    """

    def __init__(self, method: str, path: str, data: dict = None):
        raw_body = json.dumps(data).encode("utf-8") if data is not None else b""
        self.rfile = io.BytesIO(raw_body)
        self.wfile = io.BytesIO()
        self.headers = {"Content-Length": str(len(raw_body))} if data is not None else {}
        self.path = path
        self.command = method
        self.status_code = None
        self.response_data = None
        self.raw_response = None

    def send_response(self, code, message=None):
        self.status_code = code

    def send_header(self, k, v):
        pass

    def end_headers(self):
        pass

    def send_json(self, data, status=200):
        self.status_code = status
        self.response_data = data
        body = json.dumps(data).encode("utf-8")
        self.wfile.write(body)

    def execute(self) -> tuple[int, dict]:
        """Dispatch request and return (status_code, response_dict)."""
        if self.command == "GET":
            self.do_GET()
        elif self.command == "POST":
            self.do_POST()
        elif self.command == "OPTIONS":
            self.do_OPTIONS()

        raw_output = self.wfile.getvalue().decode("utf-8")
        if self.response_data is not None:
            return self.status_code, self.response_data
        try:
            return self.status_code, json.loads(raw_output) if raw_output else {}
        except Exception:
            return self.status_code, {"raw": raw_output}


class TestAIDetectAPI(unittest.TestCase):
    """Test suite testing POST /api/ai-detect and route preservation."""

    @classmethod
    def setUpClass(cls):
        server.ensure_c_executable()

    def _request(self, method: str, path: str, data: dict = None) -> tuple[int, dict]:
        client = EndpointTestClient(method, path, data)
        return client.execute()

    # 1. Successful TXT analysis (> 150 words)
    def test_01_successful_txt_analysis(self):
        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("engine"), "ADAFAI")
        self.assertEqual(body.get("mode"), "lightweight")
        self.assertEqual(body.get("format"), "TXT")
        self.assertEqual(body.get("filename"), "sample1.txt")
        self.assertEqual(body.get("status"), "completed")
        self.assertGreaterEqual(body.get("word_count", 0), 150)
        self.assertIn("overall_assessment", body)
        self.assertIn("signals", body)
        self.assertIn("sentence_analysis", body)
        self.assertGreater(len(body["sentence_analysis"]), 0)

    # 2. Successful DOCX analysis
    def test_02_successful_docx_analysis(self):
        docx_path = os.path.join(_PROJECT_ROOT, "test_corpus", "test.docx")
        with open(docx_path, "rb") as f:
            b64_content = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "test.docx",
            "content": b64_content,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("format"), "DOCX")
        self.assertIn("overall_assessment", body)
        self.assertIn("signals", body)
        self.assertGreater(body.get("word_count", 0), 0)

    # 3. Successful PDF analysis
    def test_03_successful_pdf_analysis(self):
        pdf_path = os.path.join(_PROJECT_ROOT, "test_corpus", "test.pdf")
        with open(pdf_path, "rb") as f:
            b64_content = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "test.pdf",
            "content": b64_content,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("format"), "PDF")
        self.assertIn("overall_assessment", body)
        self.assertIn("signals", body)
        self.assertGreater(body.get("word_count", 0), 0)

    # 4. Successful PPTX analysis
    def test_04_successful_pptx_analysis(self):
        pptx_path = os.path.join(_PROJECT_ROOT, "test_corpus", "test.pptx")
        with open(pptx_path, "rb") as f:
            b64_content = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "test.pptx",
            "content": b64_content,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("format"), "PPTX")
        self.assertIn("overall_assessment", body)
        self.assertIn("signals", body)
        self.assertGreater(body.get("word_count", 0), 0)

    # 5. Unsupported file extension
    def test_05_unsupported_file_extension(self):
        payload = {
            "filename": "document.xyz",
            "content": "This is sample text for an invalid file type.",
            "is_base64": False
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))
        self.assertIn("Unsupported file format", body.get("error", ""))

    # 6. Empty document
    def test_06_empty_document(self):
        # 6a. Empty content string
        payload = {
            "filename": "empty_file.txt",
            "content": "",
            "is_base64": False
        }
        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))
        self.assertIn("empty", body.get("error", "").lower())

        # 6b. File that extracts to empty text
        empty_txt = os.path.join(_PROJECT_ROOT, "test_corpus", "empty.txt")
        with open(empty_txt, "rb") as f:
            b64_empty = base64.b64encode(f.read()).decode("utf-8")

        payload_empty = {
            "filename": "empty.txt",
            "content": b64_empty,
            "is_base64": True
        }
        status, body = self._request("POST", "/api/ai-detect", payload_empty)
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))

    # 7. Document with fewer than 150 words
    def test_07_document_fewer_than_150_words(self):
        short_prose = (
            "This is a concise paragraph about machine learning concepts. "
            "It deliberately contains fewer than one hundred and fifty words "
            "so we can verify the statistical floor behavior."
        )
        payload = {
            "filename": "short_document.txt",
            "content": short_prose,
            "is_base64": False
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("status"), "insufficient_text")
        self.assertLess(body.get("word_count", 0), 150)
        self.assertEqual(body["overall_assessment"]["verdict"], "insufficient evidence")
        self.assertFalse(body["overall_assessment"]["is_sufficient_length"])
        self.assertIn("signals", body)
        self.assertIn("sentence_analysis", body)

    # 8. Missing upload parameters
    def test_08_missing_upload_parameters(self):
        # Empty payload
        status, body = self._request("POST", "/api/ai-detect", {})
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))

        # Missing content
        status, body = self._request("POST", "/api/ai-detect", {"filename": "test.txt"})
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))

        # Missing filename
        status, body = self._request("POST", "/api/ai-detect", {"content": "text content here"})
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))

    # 9. Corrupt or unreadable document
    def test_09_corrupt_document(self):
        corrupt_pdf = os.path.join(_PROJECT_ROOT, "test_corpus", "corrupt.pdf")
        with open(corrupt_pdf, "rb") as f:
            b64_corrupt = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "corrupt.pdf",
            "content": b64_corrupt,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/ai-detect", payload)
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))
        # Verify no raw filesystem path is leaked in public error
        err_msg = body.get("error", "")
        self.assertNotIn("D:\\", err_msg)
        self.assertNotIn("C:\\", err_msg)

    # 10. ADAFAI adapter error handling
    def test_10_adafai_adapter_error_handling(self):
        with patch.object(server._AI_ADAPTER, "analyze", side_effect=RuntimeError("Simulated calculation error")):
            payload = {
                "filename": "test_error.txt",
                "content": "Valid prose for testing adapter failure handling in the backend.",
                "is_base64": False
            }
            status, body = self._request("POST", "/api/ai-detect", payload)
            self.assertEqual(status, 500)
            self.assertFalse(body.get("success"))
            self.assertIn("AI detection analysis failed", body.get("error", ""))
            # Verify no paths leaked
            err_msg = body.get("error", "")
            self.assertNotIn("D:\\", err_msg)
            self.assertNotIn("C:\\", err_msg)

    # 11. Confirm existing plagiarism API routes remain operational
    def test_11_existing_plagiarism_routes_preserved(self):
        # 11a. GET /api/health
        status, health = self._request("GET", "/api/health")
        self.assertEqual(status, 200)
        self.assertEqual(health.get("status"), "healthy")
        self.assertEqual(health.get("engine"), "ready")
        self.assertEqual(health.get("ai_engine"), "ready")

        # 11b. GET /api/sample-docs
        status, samples = self._request("GET", "/api/sample-docs")
        self.assertEqual(status, 200)
        self.assertTrue(samples.get("success"))
        self.assertIsInstance(samples.get("samples"), list)

        # 11c. POST /api/upload
        sample1_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample1_path, "r", encoding="utf-8") as f:
            txt1 = f.read()
        status, up1 = self._request("POST", "/api/upload", {"filename": "sample1.txt", "content": txt1})
        self.assertEqual(status, 200)
        self.assertTrue(up1.get("success"))
        self.assertIn("filepath", up1)

        # 11d. POST /api/compare
        sample2_path = os.path.join(_PROJECT_ROOT, "documents", "sample2.txt")
        with open(sample2_path, "r", encoding="utf-8") as f:
            txt2 = f.read()
        status, up2 = self._request("POST", "/api/upload", {"filename": "sample2.txt", "content": txt2})
        self.assertEqual(status, 200)

        status, comp = self._request("POST", "/api/compare", {
            "ref_file": up1["filepath"],
            "cand_file": up2["filepath"]
        })
        self.assertEqual(status, 200)
        self.assertTrue(comp.get("success"))
        self.assertIn("similarity_score", comp)
        self.assertIn("word_similarity", comp)
        self.assertIn("sentence_similarity", comp)
        self.assertIn("phrase_similarity", comp)


if __name__ == "__main__":
    unittest.main()
