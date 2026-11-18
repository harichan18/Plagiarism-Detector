"""
tests/test_combined_analysis_api.py - Integration tests for POST /api/analyze-combined

Validates:
1. Successful TXT combined analysis.
2. Successful PDF combined analysis.
3. Successful DOCX combined analysis.
4. Successful PPTX combined analysis.
5. AI result and C11 result both appear in the response.
6. Empty reference corpus.
7. Missing reference corpus directory.
8. AI engine failure while plagiarism succeeds.
9. C11 engine failure while AI succeeds.
10. Unsupported file format.
11. Empty document.
12. Duplicate/self-reference exclusion.
13. No absolute server paths in public responses.
14. Existing /api/ai-detect behavior remains unchanged.
15. Existing plagiarism endpoints remain unchanged.
"""

import os
import sys
import json
import base64
import unittest
import io
import re
import tempfile
import shutil
from unittest.mock import patch

# Setup import path
_PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

import server
from server import PlagiarismRequestHandler


class EndpointTestClient(PlagiarismRequestHandler):
    """In-memory test client subclassing PlagiarismRequestHandler.
    Simulates HTTP request/response dispatching without physical sockets.
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


class TestCombinedAnalysisAPI(unittest.TestCase):
    """Test suite testing POST /api/analyze-combined and regression safety."""

    @classmethod
    def setUpClass(cls):
        server.ensure_c_executable()

    def _request(self, method: str, path: str, data: dict = None) -> tuple[int, dict]:
        client = EndpointTestClient(method, path, data)
        return client.execute()

    # 1. Successful TXT combined analysis
    def test_01_successful_txt_combined_analysis(self):
        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("filename"), "sample1.txt")
        self.assertEqual(body.get("format"), "TXT")
        self.assertIn("document", body)
        self.assertGreater(body["document"].get("word_count", 0), 100)

        # AI analysis check
        ai = body.get("analysis", {}).get("ai_detection", {})
        self.assertEqual(ai.get("status"), "completed")
        self.assertEqual(ai.get("engine"), "ADAFAI")
        self.assertIn("overall_assessment", ai.get("result", {}))

        # Plagiarism analysis check
        plag = body.get("analysis", {}).get("plagiarism_detection", {})
        self.assertEqual(plag.get("status"), "completed")
        self.assertEqual(plag.get("engine"), "C11")
        self.assertGreaterEqual(plag.get("reference_documents_checked", 0), 1)

    # 2. Successful PDF combined analysis
    def test_02_successful_pdf_combined_analysis(self):
        pdf_path = os.path.join(_PROJECT_ROOT, "test_corpus", "test.pdf")
        with open(pdf_path, "rb") as f:
            b64_content = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "test.pdf",
            "content": b64_content,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("format"), "PDF")
        self.assertIn(body.get("analysis", {}).get("ai_detection", {}).get("status"), ("completed", "insufficient_text"))
        self.assertEqual(body.get("analysis", {}).get("plagiarism_detection", {}).get("status"), "completed")

    # 3. Successful DOCX combined analysis
    def test_03_successful_docx_combined_analysis(self):
        docx_path = os.path.join(_PROJECT_ROOT, "test_corpus", "test.docx")
        with open(docx_path, "rb") as f:
            b64_content = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "test.docx",
            "content": b64_content,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("format"), "DOCX")
        self.assertIn(body.get("analysis", {}).get("ai_detection", {}).get("status"), ("completed", "insufficient_text"))
        self.assertEqual(body.get("analysis", {}).get("plagiarism_detection", {}).get("status"), "completed")

    # 4. Successful PPTX combined analysis
    def test_04_successful_pptx_combined_analysis(self):
        pptx_path = os.path.join(_PROJECT_ROOT, "test_corpus", "test.pptx")
        with open(pptx_path, "rb") as f:
            b64_content = base64.b64encode(f.read()).decode("utf-8")

        payload = {
            "filename": "test.pptx",
            "content": b64_content,
            "is_base64": True
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        self.assertEqual(body.get("format"), "PPTX")
        self.assertIn(body.get("analysis", {}).get("ai_detection", {}).get("status"), ("completed", "insufficient_text"))
        self.assertEqual(body.get("analysis", {}).get("plagiarism_detection", {}).get("status"), "completed")

    # 5. AI result and C11 result both appear with original details
    def test_05_ai_result_and_c11_result_both_appear(self):
        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)

        ai = body["analysis"]["ai_detection"]
        self.assertIn("signals", ai["result"])
        self.assertIn("stylometry", ai["result"]["signals"])
        self.assertIn("discourse", ai["result"]["signals"])
        self.assertIn("unicode", ai["result"]["signals"])
        self.assertIn("sentence_analysis", ai["result"])

        plag = body["analysis"]["plagiarism_detection"]
        self.assertIn("rankings", plag["result"])
        self.assertIn("statistics", plag["result"])
        self.assertIn("limitations", body)
        self.assertGreaterEqual(len(body["limitations"]), 2)

    # 6. Empty reference corpus handling
    def test_06_empty_reference_corpus(self):
        temp_dir = tempfile.mkdtemp(prefix="empty_corpus_")
        try:
            sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
            with open(sample_path, "r", encoding="utf-8") as f:
                content = f.read()

            payload = {
                "filename": "sample1.txt",
                "content": content,
                "is_base64": False,
                "corpus_dir": temp_dir
            }

            status, body = self._request("POST", "/api/analyze-combined", payload)
            self.assertEqual(status, 200)
            self.assertTrue(body.get("success"))
            # AI succeeds
            self.assertEqual(body["analysis"]["ai_detection"]["status"], "completed")
            # Plagiarism is unavailable with clear explanation
            plag = body["analysis"]["plagiarism_detection"]
            self.assertEqual(plag.get("status"), "unavailable")
            self.assertEqual(plag.get("reference_documents_checked"), 0)
            self.assertIn("No reference documents", plag.get("message", ""))
        finally:
            shutil.rmtree(temp_dir, ignore_errors=True)

    # 7. Missing reference corpus directory
    def test_07_missing_reference_corpus_directory(self):
        missing_dir = os.path.join(tempfile.gettempdir(), "non_existent_corpus_dir_12345")
        if os.path.exists(missing_dir):
            shutil.rmtree(missing_dir, ignore_errors=True)

        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False,
            "corpus_dir": missing_dir
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)
        self.assertTrue(body.get("success"))
        # AI succeeds
        self.assertEqual(body["analysis"]["ai_detection"]["status"], "completed")
        # Plagiarism reports unavailable
        plag = body["analysis"]["plagiarism_detection"]
        self.assertEqual(plag.get("status"), "unavailable")
        self.assertEqual(plag.get("reference_documents_checked"), 0)
        self.assertIn("not found", plag.get("message", "").lower())

    # 8. AI engine failure while plagiarism succeeds
    def test_08_ai_engine_failure_plagiarism_succeeds(self):
        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False
        }

        with patch.object(server._AI_ADAPTER, "analyze", side_effect=RuntimeError("Simulated AI model crash")):
            status, body = self._request("POST", "/api/analyze-combined", payload)
            self.assertEqual(status, 200)
            self.assertTrue(body.get("success"))

            ai = body["analysis"]["ai_detection"]
            self.assertEqual(ai.get("status"), "failed")
            self.assertIn("error", ai)
            self.assertIn("Simulated AI model crash", ai["error"])
            self.assertIsNone(ai.get("result"))

            plag = body["analysis"]["plagiarism_detection"]
            self.assertEqual(plag.get("status"), "completed")
            self.assertGreaterEqual(plag.get("reference_documents_checked", 0), 1)
            self.assertIsNotNone(plag.get("result"))

    # 9. C11 engine failure while AI succeeds
    def test_09_c11_engine_failure_ai_succeeds(self):
        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False
        }

        original_c_exe = server.C_EXECUTABLE
        try:
            server.C_EXECUTABLE = os.path.join(tempfile.gettempdir(), "non_existent_engine.exe")
            status, body = self._request("POST", "/api/analyze-combined", payload)
            self.assertEqual(status, 200)
            self.assertTrue(body.get("success"))

            # AI analysis succeeds
            ai = body["analysis"]["ai_detection"]
            self.assertEqual(ai.get("status"), "completed")
            self.assertIsNotNone(ai.get("result"))

            # Plagiarism analysis fails with explicit error
            plag = body["analysis"]["plagiarism_detection"]
            self.assertEqual(plag.get("status"), "failed")
            self.assertIn("error", plag)
            self.assertIsNone(plag.get("result"))
        finally:
            server.C_EXECUTABLE = original_c_exe

    # 10. Unsupported file format
    def test_10_unsupported_file_format(self):
        payload = {
            "filename": "malicious.exe",
            "content": "MZ\x90\x00\x03\x00\x00\x00",
            "is_base64": False
        }
        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))
        self.assertIn("Unsupported file format", body.get("error", ""))

    # 11. Empty document
    def test_11_empty_document(self):
        payload = {
            "filename": "empty.txt",
            "content": "",
            "is_base64": False
        }
        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 400)
        self.assertFalse(body.get("success"))
        self.assertIn("empty", body.get("error", "").lower())

    # 12. Duplicate / self-reference exclusion
    def test_12_duplicate_self_reference_exclusion(self):
        temp_dir = tempfile.mkdtemp(prefix="corpus_dedup_")
        try:
            # Place sample1.txt and sample2.txt into the test corpus
            s1_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
            s2_path = os.path.join(_PROJECT_ROOT, "documents", "sample2.txt")
            shutil.copy(s1_path, os.path.join(temp_dir, "sample1.txt"))
            shutil.copy(s2_path, os.path.join(temp_dir, "sample2.txt"))

            with open(s1_path, "r", encoding="utf-8") as f:
                content1 = f.read()

            payload = {
                "filename": "sample1.txt",
                "content": content1,
                "is_base64": False,
                "corpus_dir": temp_dir
            }

            status, body = self._request("POST", "/api/analyze-combined", payload)
            self.assertEqual(status, 200)

            plag = body["analysis"]["plagiarism_detection"]
            self.assertEqual(plag.get("status"), "completed")
            # Only sample2.txt should have been compared (sample1.txt was excluded as self-reference)
            self.assertEqual(plag.get("reference_documents_checked"), 1)

            # Ensure sample1.txt does NOT appear in rankings as a matched candidate
            rankings = plag.get("result", {}).get("rankings", [])
            doc_names = [r.get("document") for r in rankings]
            self.assertNotIn("sample1.txt", doc_names)
            self.assertIn("sample2.txt", doc_names)

            # Test 12b: Corpus containing ONLY sample1.txt uploaded as sample1.txt
            os.remove(os.path.join(temp_dir, "sample2.txt"))
            status2, body2 = self._request("POST", "/api/analyze-combined", payload)
            self.assertEqual(status2, 200)
            plag2 = body2["analysis"]["plagiarism_detection"]
            self.assertEqual(plag2.get("status"), "unavailable")
            self.assertEqual(plag2.get("reference_documents_checked"), 0)
        finally:
            shutil.rmtree(temp_dir, ignore_errors=True)

    # 13. No absolute server paths in public responses
    def test_13_no_absolute_server_paths_in_response(self):
        sample_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample_path, "r", encoding="utf-8") as f:
            content = f.read()

        payload = {
            "filename": "sample1.txt",
            "content": content,
            "is_base64": False
        }

        status, body = self._request("POST", "/api/analyze-combined", payload)
        self.assertEqual(status, 200)

        # Walk entire JSON tree and assert zero server drive/path leaks
        def check_no_paths(val, current_key=""):
            if isinstance(val, str):
                # Check for Windows absolute path like D:\... or C:\...
                win_match = re.search(r"[A-Za-z]:\\[a-zA-Z0-9_\\]+", val)
                self.assertIsNone(
                    win_match,
                    f"Found absolute Windows path in key '{current_key}': {val}"
                )
                # Check for Unix absolute paths like /home/..., /var/..., /Users/...
                if val.startswith("/") and len(val.split("/")) > 2:
                    self.fail(f"Found absolute Unix path in key '{current_key}': {val}")
            elif isinstance(val, dict):
                for k, v in val.items():
                    check_no_paths(v, f"{current_key}.{k}")
            elif isinstance(val, list):
                for idx, item in enumerate(val):
                    check_no_paths(item, f"{current_key}[{idx}]")

        check_no_paths(body)

    # 14. Existing /api/ai-detect behavior remains unchanged
    def test_14_existing_ai_detect_endpoint_preserved(self):
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
        self.assertIn("overall_assessment", body)
        self.assertIn("signals", body)

    # 15. Existing plagiarism endpoints remain unchanged
    def test_15_existing_plagiarism_endpoints_preserved(self):
        # 15a. GET /api/health
        status, health = self._request("GET", "/api/health")
        self.assertEqual(status, 200)
        self.assertEqual(health.get("status"), "healthy")
        self.assertEqual(health.get("engine"), "ready")
        self.assertEqual(health.get("ai_engine"), "ready")

        # 15b. GET /api/sample-docs
        status, samples = self._request("GET", "/api/sample-docs")
        self.assertEqual(status, 200)
        self.assertTrue(samples.get("success"))
        self.assertIsInstance(samples.get("samples"), list)

        # 15c. POST /api/upload
        sample1_path = os.path.join(_PROJECT_ROOT, "documents", "sample1.txt")
        with open(sample1_path, "r", encoding="utf-8") as f:
            txt1 = f.read()
        status, up1 = self._request("POST", "/api/upload", {"filename": "sample1.txt", "content": txt1})
        self.assertEqual(status, 200)
        self.assertTrue(up1.get("success"))

        # 15d. POST /api/compare
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

        # 15e. POST /api/batch-compare
        status, batch = self._request("POST", "/api/batch-compare", {
            "ref_file": up1["filepath"],
            "cand_files": [up2["filepath"]]
        })
        self.assertEqual(status, 200)
        self.assertTrue(batch.get("success"))
        self.assertIn("rankings", batch)
        self.assertIn("statistics", batch)


if __name__ == "__main__":
    unittest.main()
