#!/usr/bin/env python3
"""
server.py - Bridge server for Plagiarism Detection & Document Similarity Analysis.
Handles HTTP REST API, multi-format text extraction (.txt, .pdf, .docx, .pptx),
and delegates core algorithmic analysis to the authoritative C11 DSA engine.
Standard C engine receives normalized UTF-8 plain text.
"""

import http.server
import socketserver
import subprocess
import json
import os
import sys
import mimetypes
import base64
import uuid
import hashlib
import re
from urllib.parse import urlparse

# Document parsing libraries
import pymupdf
import docx
import pptx

# AI Detection Adapter (lightweight core)
try:
    from ai_detector.adafai_adapter import ADAFAIAdapter
    _AI_ADAPTER = ADAFAIAdapter()
    _AI_IMPORT_ERROR = None
except ImportError as _ai_err:
    _AI_ADAPTER = None
    _AI_IMPORT_ERROR = str(_ai_err)

HOST = "0.0.0.0"
DEFAULT_PORT = 8080
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UI_DIR = os.path.join(BASE_DIR, "ui")
UPLOADS_DIR = os.path.join(BASE_DIR, "uploads")
ORIGINALS_DIR = os.path.join(UPLOADS_DIR, "originals")
EXTRACTED_DIR = os.path.join(UPLOADS_DIR, "extracted")
REPORTS_DIR = os.path.join(BASE_DIR, "reports")
DOCS_DIR = os.path.join(BASE_DIR, "documents")

# Locate C executable relative to project root
EXECUTABLE_NAME = "plagiarism_detector.exe" if sys.platform.startswith("win") else "plagiarism_detector"
C_EXECUTABLE = os.path.join(BASE_DIR, EXECUTABLE_NAME)

# Ensure writable directories exist
os.makedirs(ORIGINALS_DIR, exist_ok=True)
os.makedirs(EXTRACTED_DIR, exist_ok=True)
os.makedirs(REPORTS_DIR, exist_ok=True)
os.makedirs(DOCS_DIR, exist_ok=True)

MAX_UPLOAD_SIZE = 50 * 1024 * 1024  # 50 MB upload limit
SUPPORTED_EXTENSIONS = {".txt", ".pdf", ".docx", ".pptx"}


def sanitize_error_message(msg: str) -> str:
    """Remove local filesystem paths and trace details from public error messages."""
    if not msg:
        return "An unexpected error occurred."
    clean = str(msg)
    # Remove Windows drive paths (e.g., C:\... or D:\...)
    clean = re.sub(r"[A-Za-z]:\\[^:\n\r\t\"\'<>]+", "[path]", clean)
    # Remove Unix paths (/foo/bar/...)
    clean = re.sub(r"/(?:[a-zA-Z0-9._-]+/)+[a-zA-Z0-9._-]+", "[path]", clean)
    return clean


def scrub_server_paths(obj):
    """Recursively scrub all absolute Windows or Unix server paths from nested structures."""
    if isinstance(obj, str):
        clean = re.sub(r"[A-Za-z]:\\[^:\n\r\t\"\'<>]+", "[path]", obj)
        clean = re.sub(r"/(?:[a-zA-Z0-9._-]+/)+[a-zA-Z0-9._-]+", "[path]", clean)
        return clean
    elif isinstance(obj, dict):
        return {k: scrub_server_paths(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [scrub_server_paths(elem) for elem in obj]
    else:
        return obj


def ensure_c_executable():
    """Ensure C executable exists and has execute permissions. Compiles if needed on POSIX/Linux."""
    if os.path.isfile(C_EXECUTABLE):
        if not sys.platform.startswith("win"):
            try:
                st = os.stat(C_EXECUTABLE)
                os.chmod(C_EXECUTABLE, st.st_mode | 0o755)
            except Exception as e:
                print(f"Warning: Could not set executable permissions on {C_EXECUTABLE}: {e}")
        return True

    src_dir = os.path.join(BASE_DIR, "src")
    include_dir = os.path.join(BASE_DIR, "include")
    if os.path.isdir(src_dir) and os.path.isdir(include_dir):
        print(f"C engine executable '{C_EXECUTABLE}' not found. Compiling from C source files...")
        import glob
        c_files = glob.glob(os.path.join(src_dir, "*.c"))
        if c_files:
            cmd = [
                "gcc", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Wconversion",
                "-std=c11", f"-I{include_dir}", "-o", C_EXECUTABLE
            ] + c_files
            try:
                subprocess.run(cmd, capture_output=True, text=True, check=True)
                if not sys.platform.startswith("win"):
                    st = os.stat(C_EXECUTABLE)
                    os.chmod(C_EXECUTABLE, st.st_mode | 0o755)
                print(f"Successfully compiled C engine: {C_EXECUTABLE}")
                return True
            except Exception as err:
                print(f"Automatic C engine compilation failed: {err}")
    print(f"Warning: C executable '{C_EXECUTABLE}' not found. Compilation with gcc may be required.")
    return False


def extract_text_from_pdf(filepath):
    try:
        doc = pymupdf.open(filepath)
    except Exception as e:
        raise ValueError("The PDF file appears to be corrupted or invalid.") from e

    if doc.is_encrypted:
        doc.close()
        raise ValueError("This PDF is password-protected and cannot be analyzed without a password.")

    text_parts = []
    try:
        for page_num in range(len(doc)):
            page = doc[page_num]
            text = page.get_text()
            if text and text.strip():
                text_parts.append(text.strip())
    finally:
        doc.close()

    full_text = "\n\n".join(text_parts).strip()
    if not full_text:
        raise ValueError("This PDF does not contain extractable text. Scanned/image-only PDFs are not supported.")
    return full_text


def extract_text_from_docx(filepath):
    try:
        doc = docx.Document(filepath)
    except Exception as e:
        raise ValueError("The DOCX file appears to be corrupted or invalid.") from e

    text_parts = []
    for para in doc.paragraphs:
        t = para.text.strip()
        if t:
            text_parts.append(t)

    for table in doc.tables:
        for row in table.rows:
            row_items = [cell.text.strip() for cell in row.cells if cell.text.strip()]
            if row_items:
                text_parts.append(" ".join(row_items))

    full_text = "\n\n".join(text_parts).strip()
    if not full_text:
        raise ValueError("This document does not contain extractable text.")
    return full_text


def extract_text_from_pptx(filepath):
    try:
        prs = pptx.Presentation(filepath)
    except Exception as e:
        raise ValueError("The PPTX file appears to be corrupted or invalid.") from e

    text_parts = []
    for slide_idx, slide in enumerate(prs.slides):
        slide_lines = []
        for shape in slide.shapes:
            if shape.has_text_frame:
                for para in shape.text_frame.paragraphs:
                    t = para.text.strip()
                    if t:
                        slide_lines.append(t)
        if slide_lines:
            text_parts.append("\n".join(slide_lines))

    full_text = "\n\n".join(text_parts).strip()
    if not full_text:
        raise ValueError("This document does not contain extractable text.")
    return full_text


def extract_text_from_txt(filepath):
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            text = f.read().strip()
    except UnicodeDecodeError:
        with open(filepath, "r", encoding="latin-1") as f:
            text = f.read().strip()

    if not text:
        raise ValueError("This document does not contain extractable text.")
    return text


def extract_document_text(filepath, ext):
    ext = ext.lower()
    if ext == ".pdf":
        return extract_text_from_pdf(filepath)
    elif ext == ".docx":
        return extract_text_from_docx(filepath)
    elif ext == ".pptx":
        return extract_text_from_pptx(filepath)
    elif ext == ".txt":
        return extract_text_from_txt(filepath)
    else:
        raise ValueError("Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX.")


class PlagiarismRequestHandler(http.server.BaseHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With, Accept")
        self.send_header("Access-Control-Max-Age", "86400")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def send_json(self, data, status=200):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path in ("/api/health", "/health", "/healthz"):
            self.send_json({
                "status": "healthy",
                "engine": "ready" if os.path.isfile(C_EXECUTABLE) else "missing",
                "executable": os.path.basename(C_EXECUTABLE),
                "ai_engine": "ready" if (_AI_ADAPTER and _AI_ADAPTER.is_available()) else "unavailable"
            })
            return

        if path == "/api/sample-docs":
            self.handle_sample_docs()
            return

        if path.startswith("/documents/"):
            doc_fname = os.path.basename(path)
            doc_fpath = os.path.join(DOCS_DIR, doc_fname)
            if os.path.isfile(doc_fpath):
                with open(doc_fpath, "rb") as df:
                    c_data = df.read()
                self.send_response(200)
                self.send_header("Content-Type", "text/plain; charset=utf-8")
                self.send_header("Content-Length", str(len(c_data)))
                self.end_headers()
                self.wfile.write(c_data)
                return

        if path == "/" or path == "":
            path = "/index.html"

        rel_path = os.path.normpath(path.lstrip("/"))
        file_path = os.path.join(UI_DIR, rel_path)

        if os.path.commonpath([UI_DIR, file_path]) == UI_DIR and os.path.isfile(file_path):
            content_type, _ = mimetypes.guess_type(file_path)
            if not content_type:
                content_type = "application/octet-stream"

            with open(file_path, "rb") as f:
                content = f.read()

            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content)
        else:
            self.send_response(404)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(b"404 Not Found")

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path

        content_length = int(self.headers.get("Content-Length", 0))
        if content_length > MAX_UPLOAD_SIZE:
            self.send_json({"success": False, "error": "File size exceeds the 50 MB limit."}, 413)
            return

        post_body = self.rfile.read(content_length)

        try:
            data = json.loads(post_body.decode("utf-8")) if post_body else {}
        except Exception as e:
            self.send_json({"success": False, "error": f"Invalid JSON body: {str(e)}"}, 400)
            return

        if path == "/api/upload":
            self.handle_upload(data)
        elif path == "/api/compare":
            self.handle_compare(data)
        elif path == "/api/batch-compare":
            self.handle_batch_compare(data)
        elif path == "/api/export-report":
            self.handle_export_report(data)
        elif path == "/api/ai-detect":
            self.handle_ai_detect(data)
        elif path == "/api/analyze-combined":
            self.handle_analyze_combined(data)
        else:
            self.send_json({"success": False, "error": "Endpoint not found"}, 404)

    def handle_sample_docs(self):
        samples = []
        if os.path.isdir(DOCS_DIR):
            for fname in sorted(os.listdir(DOCS_DIR)):
                ext = os.path.splitext(fname)[1].lower()
                if ext in SUPPORTED_EXTENSIONS:
                    fpath = os.path.join(DOCS_DIR, fname)
                    sample_info = {
                        "filename": fname,
                        "filepath": fpath.replace("\\", "/"),
                        "size": os.path.getsize(fpath)
                    }
                    if ext == ".txt":
                        try:
                            with open(fpath, "r", encoding="utf-8", errors="replace") as sf:
                                sample_info["content"] = sf.read()
                        except Exception:
                            pass
                    samples.append(sample_info)
        self.send_json({"success": True, "samples": samples})

    def handle_upload(self, data):
        filename = data.get("filename")
        content = data.get("content", "")
        is_base64 = data.get("is_base64", False)

        if not filename:
            self.send_json({"success": False, "error": "Missing filename"}, 400)
            return

        safe_name = os.path.basename(filename)
        ext = os.path.splitext(safe_name)[1].lower()

        if ext not in SUPPORTED_EXTENSIONS:
            self.send_json({
                "success": False,
                "error": "Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX."
            }, 400)
            return

        # Decode content bytes
        try:
            if is_base64:
                # Strip potential data URL prefix (e.g. data:application/pdf;base64,...)
                if "," in content:
                    content = content.split(",", 1)[1]
                content_bytes = base64.b64decode(content)
            else:
                content_bytes = content.encode("utf-8")
        except Exception as e:
            self.send_json({"success": False, "error": f"Failed to decode uploaded data: {str(e)}"}, 400)
            return

        if len(content_bytes) == 0:
            self.send_json({"success": False, "error": f"Document '{safe_name}' is empty."}, 400)
            return

        # Generate unique collision-safe identifier based on content hash
        file_hash = hashlib.sha256(content_bytes).hexdigest()[:12]
        base_name_no_ext = os.path.splitext(safe_name)[0]
        unique_original_name = f"{file_hash}_{safe_name}"
        unique_extracted_name = f"{file_hash}_{base_name_no_ext}.txt"

        original_path = os.path.join(ORIGINALS_DIR, unique_original_name)
        extracted_path = os.path.join(EXTRACTED_DIR, unique_extracted_name)

        # 1. Save original file to disk
        try:
            with open(original_path, "wb") as f:
                f.write(content_bytes)
        except Exception as e:
            self.send_json({"success": False, "error": f"Could not save original file: {str(e)}"}, 500)
            return

        # 2. Extract plain text according to format
        try:
            plain_text = extract_document_text(original_path, ext)
        except ValueError as ve:
            self.send_json({"success": False, "error": str(ve)}, 400)
            return
        except Exception as e:
            self.send_json({"success": False, "error": f"Extraction failure: {str(e)}"}, 500)
            return

        # 3. Save normalized plain text
        try:
            with open(extracted_path, "w", encoding="utf-8") as f:
                f.write(plain_text)
        except Exception as e:
            self.send_json({"success": False, "error": f"Could not save extracted text: {str(e)}"}, 500)
            return

        # 4. Query C engine for metadata
        cmd = [C_EXECUTABLE, "--doc-info", extracted_path]
        try:
            res = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", check=True)
            stdout_text = (res.stdout or "").strip()
            doc_info = json.loads(stdout_text)
            doc_info["filepath"] = extracted_path.replace("\\", "/")
            doc_info["original_path"] = original_path.replace("\\", "/")
            doc_info["filename"] = safe_name  # Preserve original filename
            doc_info["file_size"] = len(content_bytes)
            doc_info["format"] = ext.lstrip(".").upper()
            self.send_json(doc_info)
        except Exception as e:
            self.send_json({
                "success": True,
                "filepath": extracted_path.replace("\\", "/"),
                "original_path": original_path.replace("\\", "/") if 'original_path' in locals() else "",
                "filename": safe_name,
                "file_size": len(content_bytes),
                "word_count": len(plain_text.split()),
                "sentence_count": plain_text.count(".") + plain_text.count("?") + plain_text.count("!"),
                "unique_words": len(set(plain_text.lower().split())),
                "top_words": [],
                "format": ext.lstrip(".").upper()
            })

    def handle_compare(self, data):
        ref_file = data.get("ref_file")
        cand_file = data.get("cand_file")
        ref_display_name = data.get("ref_name")
        cand_display_name = data.get("cand_name")

        if not ref_file or not cand_file:
            self.send_json({"success": False, "error": "Both ref_file and cand_file are required."}, 400)
            return

        cmd = [C_EXECUTABLE, "--compare-json", ref_file, cand_file]
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)
            if proc.returncode != 0:
                err_msg = (proc.stderr or proc.stdout or f"C engine process failed with return code {proc.returncode}").strip()
                self.send_json({"success": False, "error": err_msg}, 500)
                return

            stdout_text = (proc.stdout or "").strip()
            if not stdout_text:
                self.send_json({"success": False, "error": "C engine returned an empty response."}, 500)
                return

            result = json.loads(stdout_text)

            # Restore original user-facing filenames
            if ref_display_name:
                result["reference_name"] = ref_display_name
            if cand_display_name:
                result["compared_name"] = cand_display_name

            self.send_json(result)
        except subprocess.TimeoutExpired:
            self.send_json({"success": False, "error": "C engine comparison timed out."}, 500)
        except Exception as e:
            self.send_json({"success": False, "error": f"Failed to execute C engine: {str(e)}"}, 500)

    def handle_batch_compare(self, data):
        ref_file = data.get("ref_file")
        cand_files = data.get("cand_files", [])
        ref_display_name = data.get("ref_name")
        cand_display_map = data.get("cand_names", {})

        if not ref_file or not cand_files:
            self.send_json({"success": False, "error": "ref_file and at least one cand_file are required."}, 400)
            return

        cmd = [C_EXECUTABLE, "--batch-json", ref_file] + cand_files
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
            if proc.returncode != 0:
                err_msg = (proc.stderr or proc.stdout or f"C batch engine failed with return code {proc.returncode}").strip()
                self.send_json({"success": False, "error": err_msg}, 500)
                return

            stdout_text = (proc.stdout or "").strip()
            if not stdout_text:
                self.send_json({"success": False, "error": "C engine returned an empty response."}, 500)
                return

            result = json.loads(stdout_text)

            if ref_display_name:
                result["reference_name"] = ref_display_name

            # Map extracted filenames back to original user filenames
            for row in result.get("rankings", []):
                extracted_path = row.get("filepath", "")
                if extracted_path in cand_display_map:
                    row["document"] = cand_display_map[extracted_path]

            self.send_json(result)
        except Exception as e:
            self.send_json({"success": False, "error": f"Failed to execute batch comparison: {str(e)}"}, 500)

    def handle_export_report(self, data):
        ref_file = data.get("ref_file")
        cand_file = data.get("cand_file")
        out_name = data.get("output_filename")
        ref_display_name = data.get("ref_name")
        cand_display_name = data.get("cand_name")

        if not ref_file or not cand_file or not out_name:
            self.send_json({"success": False, "error": "Missing required fields for report export"}, 400)
            return

        safe_out_name = os.path.basename(out_name)
        if not safe_out_name.endswith(".txt"):
            safe_out_name += ".txt"

        dest_report = os.path.join(REPORTS_DIR, safe_out_name)
        cmd = [C_EXECUTABLE, "--export-report", ref_file, cand_file, dest_report]

        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=15)
            if proc.returncode != 0:
                err_msg = (proc.stderr or proc.stdout or f"C report engine failed with return code {proc.returncode}").strip()
                self.send_json({"success": False, "error": err_msg}, 500)
                return

            report_text = ""
            if os.path.isfile(dest_report):
                with open(dest_report, "r", encoding="utf-8") as f:
                    report_text = f.read()

                # Clean header display names in the saved report file
                if ref_display_name or cand_display_name:
                    lines = report_text.splitlines()
                    new_lines = []
                    for line in lines:
                        if line.startswith("Reference Document:") and ref_display_name:
                            new_lines.append("Reference Document:\n" + ref_display_name)
                        elif line.startswith("Compared Document:") and cand_display_name:
                            new_lines.append("Compared Document:\n" + cand_display_name)
                        else:
                            new_lines.append(line)
                    report_text = "\n".join(new_lines)
                    with open(dest_report, "w", encoding="utf-8") as f:
                        f.write(report_text)

            self.send_json({
                "success": True,
                "exported_path": dest_report.replace("\\", "/"),
                "filename": safe_out_name,
                "content": report_text
            })
        except Exception as e:
            self.send_json({"success": False, "error": f"Report export failed: {str(e)}"}, 500)

    def handle_ai_detect(self, data):
        """Handle AI detection requests using the lightweight ADAFAI adapter.
        Accepts single document upload (PDF, DOCX, PPTX, TXT) via base64 or raw text,
        or references an existing extracted/original filepath on disk.
        """
        if not isinstance(data, dict):
            self.send_json({"success": False, "error": "Invalid request payload."}, 400)
            return

        if _AI_ADAPTER is None or not _AI_ADAPTER.is_available():
            err_msg = _AI_IMPORT_ERROR or "AI detection engine is not available."
            self.send_json({"success": False, "error": sanitize_error_message(err_msg)}, 503)
            return

        filename = data.get("filename")
        content = data.get("content")
        is_base64 = data.get("is_base64", False)
        filepath = data.get("filepath")

        file_size = 0
        safe_name = ""
        ext = ""
        extracted_path = ""
        original_path = ""

        # Case 1: Existing filepath provided
        if filepath and content is None:
            norm_fp = os.path.normpath(filepath)
            allowed = False
            for allowed_dir in [EXTRACTED_DIR, ORIGINALS_DIR, DOCS_DIR]:
                try:
                    if os.path.commonpath([allowed_dir, os.path.abspath(norm_fp)]) == allowed_dir:
                        allowed = True
                        break
                except ValueError:
                    pass

            if not allowed or not os.path.isfile(norm_fp):
                self.send_json({"success": False, "error": "File not found or access denied."}, 404)
                return

            safe_name = os.path.basename(norm_fp)
            ext = os.path.splitext(safe_name)[1].lower()

            if norm_fp.startswith(EXTRACTED_DIR) or ext == ".txt":
                try:
                    plain_text = extract_text_from_txt(norm_fp)
                except Exception as e:
                    self.send_json({"success": False, "error": sanitize_error_message(str(e))}, 400)
                    return
                extracted_path = norm_fp
            else:
                if ext not in SUPPORTED_EXTENSIONS:
                    self.send_json({
                        "success": False,
                        "error": "Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX."
                    }, 400)
                    return
                try:
                    plain_text = extract_document_text(norm_fp, ext)
                except ValueError as ve:
                    self.send_json({"success": False, "error": sanitize_error_message(str(ve))}, 400)
                    return
                except Exception as e:
                    self.send_json({"success": False, "error": "Extraction failure: " + sanitize_error_message(str(e))}, 500)
                    return
                original_path = norm_fp

            if os.path.isfile(norm_fp):
                file_size = os.path.getsize(norm_fp)

        # Case 2: Uploaded file content provided
        elif filename is not None and content is not None:
            safe_name = os.path.basename(filename)
            if not safe_name:
                self.send_json({"success": False, "error": "Missing filename"}, 400)
                return

            ext = os.path.splitext(safe_name)[1].lower()
            if ext not in SUPPORTED_EXTENSIONS:
                self.send_json({
                    "success": False,
                    "error": "Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX."
                }, 400)
                return

            try:
                if is_base64:
                    if "," in content:
                        content = content.split(",", 1)[1]
                    content_bytes = base64.b64decode(content)
                else:
                    content_bytes = content.encode("utf-8")
            except Exception as e:
                self.send_json({"success": False, "error": "Failed to decode uploaded data: " + sanitize_error_message(str(e))}, 400)
                return

            if len(content_bytes) == 0:
                self.send_json({"success": False, "error": f"Document '{safe_name}' is empty."}, 400)
                return

            file_size = len(content_bytes)
            file_hash = hashlib.sha256(content_bytes).hexdigest()[:12]
            base_name_no_ext = os.path.splitext(safe_name)[0]
            unique_original_name = f"{file_hash}_{safe_name}"
            unique_extracted_name = f"{file_hash}_{base_name_no_ext}.txt"

            original_path = os.path.join(ORIGINALS_DIR, unique_original_name)
            extracted_path = os.path.join(EXTRACTED_DIR, unique_extracted_name)

            try:
                with open(original_path, "wb") as f:
                    f.write(content_bytes)
            except Exception as e:
                self.send_json({"success": False, "error": "Could not save original file: " + sanitize_error_message(str(e))}, 500)
                return

            try:
                plain_text = extract_document_text(original_path, ext)
            except ValueError as ve:
                self.send_json({"success": False, "error": sanitize_error_message(str(ve))}, 400)
                return
            except Exception as e:
                self.send_json({"success": False, "error": "Extraction failure: " + sanitize_error_message(str(e))}, 500)
                return

            try:
                with open(extracted_path, "w", encoding="utf-8") as f:
                    f.write(plain_text)
            except Exception as e:
                self.send_json({"success": False, "error": "Could not save extracted text: " + sanitize_error_message(str(e))}, 500)
                return

        else:
            self.send_json({
                "success": False,
                "error": "Missing required upload parameters (filename and content required)."
            }, 400)
            return

        if not plain_text or not plain_text.strip():
            self.send_json({"success": False, "error": "This document does not contain extractable text."}, 400)
            return

        try:
            ai_result = _AI_ADAPTER.analyze(plain_text)
        except Exception as exc:
            self.send_json({"success": False, "error": "AI detection analysis failed: " + sanitize_error_message(str(exc))}, 500)
            return

        response_payload = dict(ai_result)
        response_payload["success"] = ai_result.get("status") in ("completed", "insufficient_text")
        response_payload["filename"] = safe_name
        response_payload["format"] = ext.lstrip(".").upper()
        response_payload["file_size"] = file_size
        if extracted_path:
            response_payload["extracted_filepath"] = extracted_path.replace("\\", "/")
        if original_path:
            response_payload["original_path"] = original_path.replace("\\", "/")

        self.send_json(response_payload, 200)

    def handle_analyze_combined(self, data):
        """Handle unified AI-content detection + C11 plagiarism detection.
        Extracts document text once, runs ADAFAI AI detection and C11 plagiarism detection
        against the reference corpus independently, and returns a unified JSON response.
        """
        if not isinstance(data, dict):
            self.send_json({"success": False, "error": "Invalid request payload."}, 400)
            return

        filename = data.get("filename")
        content = data.get("content")
        is_base64 = data.get("is_base64", False)
        filepath = data.get("filepath")
        target_corpus_dir = data.get("corpus_dir") or os.environ.get("PLAGIARISM_CORPUS_DIR") or DOCS_DIR

        file_size = 0
        safe_name = ""
        ext = ""
        extracted_path = ""
        original_path = ""
        plain_text = ""
        content_bytes = b""

        # 1. Document Extraction & Ingestion
        # Case A: Existing filepath
        if filepath and content is None:
            norm_fp = os.path.normpath(filepath)
            allowed = False
            for allowed_dir in [EXTRACTED_DIR, ORIGINALS_DIR, DOCS_DIR]:
                try:
                    if os.path.commonpath([allowed_dir, os.path.abspath(norm_fp)]) == allowed_dir:
                        allowed = True
                        break
                except ValueError:
                    pass

            if not allowed or not os.path.isfile(norm_fp):
                self.send_json({"success": False, "error": "File not found or access denied."}, 404)
                return

            safe_name = os.path.basename(norm_fp)
            ext = os.path.splitext(safe_name)[1].lower()

            try:
                with open(norm_fp, "rb") as rf:
                    content_bytes = rf.read()
                file_size = len(content_bytes)
            except Exception as e:
                self.send_json({"success": False, "error": "Could not read document: " + sanitize_error_message(str(e))}, 400)
                return

            if file_size == 0:
                self.send_json({"success": False, "error": f"Document '{safe_name}' is empty."}, 400)
                return

            if norm_fp.startswith(EXTRACTED_DIR) or ext == ".txt":
                try:
                    plain_text = extract_text_from_txt(norm_fp)
                except Exception as e:
                    self.send_json({"success": False, "error": sanitize_error_message(str(e))}, 400)
                    return
                extracted_path = norm_fp
            else:
                if ext not in SUPPORTED_EXTENSIONS:
                    self.send_json({
                        "success": False,
                        "error": "Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX."
                    }, 400)
                    return
                try:
                    plain_text = extract_document_text(norm_fp, ext)
                except ValueError as ve:
                    self.send_json({"success": False, "error": sanitize_error_message(str(ve))}, 400)
                    return
                except Exception as e:
                    self.send_json({"success": False, "error": "Extraction failure: " + sanitize_error_message(str(e))}, 500)
                    return
                original_path = norm_fp

                # Write extracted version for C engine
                file_hash = hashlib.sha256(content_bytes).hexdigest()[:12]
                base_name_no_ext = os.path.splitext(safe_name)[0]
                extracted_path = os.path.join(EXTRACTED_DIR, f"{file_hash}_{base_name_no_ext}.txt")
                try:
                    with open(extracted_path, "w", encoding="utf-8") as wf:
                        wf.write(plain_text)
                except Exception as e:
                    self.send_json({"success": False, "error": "Could not save extracted text: " + sanitize_error_message(str(e))}, 500)
                    return

        # Case B: Uploaded document content
        elif filename is not None and content is not None:
            safe_name = os.path.basename(filename)
            if not safe_name:
                self.send_json({"success": False, "error": "Missing filename"}, 400)
                return

            ext = os.path.splitext(safe_name)[1].lower()
            if ext not in SUPPORTED_EXTENSIONS:
                self.send_json({
                    "success": False,
                    "error": "Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX."
                }, 400)
                return

            try:
                if is_base64:
                    if "," in content:
                        content = content.split(",", 1)[1]
                    content_bytes = base64.b64decode(content)
                else:
                    content_bytes = content.encode("utf-8")
            except Exception as e:
                self.send_json({"success": False, "error": "Failed to decode uploaded data: " + sanitize_error_message(str(e))}, 400)
                return

            if len(content_bytes) == 0:
                self.send_json({"success": False, "error": f"Document '{safe_name}' is empty."}, 400)
                return

            if len(content_bytes) > MAX_UPLOAD_SIZE:
                self.send_json({"success": False, "error": f"File size exceeds maximum upload limit of {MAX_UPLOAD_SIZE // (1024 * 1024)}MB"}, 400)
                return

            file_size = len(content_bytes)
            file_hash = hashlib.sha256(content_bytes).hexdigest()[:12]
            base_name_no_ext = os.path.splitext(safe_name)[0]
            unique_original_name = f"{file_hash}_{safe_name}"
            unique_extracted_name = f"{file_hash}_{base_name_no_ext}.txt"

            original_path = os.path.join(ORIGINALS_DIR, unique_original_name)
            extracted_path = os.path.join(EXTRACTED_DIR, unique_extracted_name)

            try:
                with open(original_path, "wb") as f:
                    f.write(content_bytes)
            except Exception as e:
                self.send_json({"success": False, "error": "Could not save original file: " + sanitize_error_message(str(e))}, 500)
                return

            try:
                plain_text = extract_document_text(original_path, ext)
            except ValueError as ve:
                self.send_json({"success": False, "error": sanitize_error_message(str(ve))}, 400)
                return
            except Exception as e:
                self.send_json({"success": False, "error": "Extraction failure: " + sanitize_error_message(str(e))}, 500)
                return

            try:
                with open(extracted_path, "w", encoding="utf-8") as f:
                    f.write(plain_text)
            except Exception as e:
                self.send_json({"success": False, "error": "Could not save extracted text: " + sanitize_error_message(str(e))}, 500)
                return

        else:
            self.send_json({
                "success": False,
                "error": "Missing required upload parameters (filename and content required)."
            }, 400)
            return

        if not plain_text or not plain_text.strip():
            self.send_json({"success": False, "error": "This document does not contain extractable text."}, 400)
            return

        # 2. AI-Content Detection (Independent Engine)
        ai_section = {
            "status": "completed",
            "engine": "ADAFAI",
            "mode": "lightweight",
            "result": None
        }

        if _AI_ADAPTER is None or not _AI_ADAPTER.is_available():
            ai_section["status"] = "unavailable"
            ai_section["error"] = sanitize_error_message(_AI_IMPORT_ERROR or "AI detection engine is unavailable.")
        else:
            try:
                ai_res = _AI_ADAPTER.analyze(plain_text)
                ai_section["status"] = ai_res.get("status", "completed")
                ai_section["result"] = ai_res
            except Exception as exc:
                ai_section["status"] = "failed"
                ai_section["error"] = sanitize_error_message(f"AI detection analysis failed: {str(exc)}")

        # 3. C11 Plagiarism Detection against Reference Corpus (Independent Engine)
        plagiarism_section = {
            "status": "completed",
            "engine": "C11",
            "reference_documents_checked": 0,
            "result": None
        }

        if not os.path.isdir(target_corpus_dir):
            plagiarism_section["status"] = "unavailable"
            plagiarism_section["message"] = "Reference corpus directory not found."
        else:
            # Discover and prepare candidate reference documents
            candidate_list = []
            uploaded_bytes_hash = hashlib.sha256(content_bytes).hexdigest() if content_bytes else None
            uploaded_text_clean = plain_text.strip().lower()

            for entry in sorted(os.listdir(target_corpus_dir)):
                cand_path = os.path.join(target_corpus_dir, entry)
                if not os.path.isfile(cand_path):
                    continue

                cand_ext = os.path.splitext(entry)[1].lower()
                if cand_ext not in SUPPORTED_EXTENSIONS:
                    continue

                # Exclude exact binary match with uploaded document (self-reference)
                if uploaded_bytes_hash:
                    try:
                        with open(cand_path, "rb") as cf:
                            c_bytes = cf.read()
                        if hashlib.sha256(c_bytes).hexdigest() == uploaded_bytes_hash:
                            continue
                    except Exception:
                        pass

                # Prepare candidate text
                cand_text = None
                cand_text_path = None
                if cand_ext == ".txt":
                    try:
                        cand_text = extract_text_from_txt(cand_path)
                        cand_text_path = cand_path
                    except Exception:
                        continue
                else:
                    try:
                        cand_text = extract_document_text(cand_path, cand_ext)
                        c_hash = hashlib.sha256(cand_text.encode("utf-8")).hexdigest()[:12]
                        extracted_cand_name = f"corpus_{c_hash}_{os.path.splitext(entry)[0]}.txt"
                        cand_text_path = os.path.join(EXTRACTED_DIR, extracted_cand_name)
                        with open(cand_text_path, "w", encoding="utf-8") as cw:
                            cw.write(cand_text)
                    except Exception:
                        continue

                # Exclude text-level identity match (self-reference / duplicate)
                if cand_text and cand_text.strip().lower() == uploaded_text_clean:
                    continue

                candidate_list.append({
                    "display_name": entry,
                    "text_path": cand_text_path
                })

            if len(candidate_list) == 0:
                plagiarism_section["status"] = "unavailable"
                plagiarism_section["reference_documents_checked"] = 0
                plagiarism_section["message"] = "No reference documents available for comparison in reference corpus (self-references excluded)."
            elif not os.path.isfile(C_EXECUTABLE):
                plagiarism_section["status"] = "failed"
                plagiarism_section["reference_documents_checked"] = 0
                plagiarism_section["error"] = "C plagiarism engine executable not found."
            else:
                cand_text_paths = [c["text_path"] for c in candidate_list]
                cmd = [C_EXECUTABLE, "--batch-json", extracted_path] + cand_text_paths
                try:
                    proc = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
                    if proc.returncode != 0:
                        err_msg = sanitize_error_message(proc.stderr or proc.stdout or f"C batch engine failed with return code {proc.returncode}")
                        plagiarism_section["status"] = "failed"
                        plagiarism_section["error"] = err_msg
                    else:
                        stdout_text = (proc.stdout or "").strip()
                        raw_batch = json.loads(stdout_text)

                        path_to_display = {
                            c["text_path"]: c["display_name"] for c in candidate_list
                        }
                        for c in candidate_list:
                            path_to_display[os.path.normpath(c["text_path"])] = c["display_name"]
                            path_to_display[c["text_path"].replace("\\", "/")] = c["display_name"]

                        sanitized_rankings = []
                        for r in raw_batch.get("rankings", []):
                            r_fp = r.get("filepath", "")
                            disp = path_to_display.get(os.path.normpath(r_fp),
                                   path_to_display.get(r_fp, os.path.basename(r_fp)))
                            sanitized_rankings.append({
                                "rank": r.get("rank"),
                                "document": disp,
                                "similarity": r.get("similarity"),
                                "level": r.get("level")
                            })

                        sanitized_batch = {
                            "total_compared": raw_batch.get("total_compared", len(candidate_list)),
                            "reference_document": safe_name,
                            "rankings": sanitized_rankings,
                            "statistics": raw_batch.get("statistics", {})
                        }

                        # Inspect top-ranked candidate for phrase-level details
                        if sanitized_rankings:
                            top_doc_name = sanitized_rankings[0]["document"]
                            top_cand_path = None
                            for c in candidate_list:
                                if c["display_name"] == top_doc_name:
                                    top_cand_path = c["text_path"]
                                    break
                            if top_cand_path and os.path.isfile(top_cand_path):
                                cmp_cmd = [C_EXECUTABLE, "--compare-json", extracted_path, top_cand_path]
                                try:
                                    cmp_p = subprocess.run(cmp_cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=30)
                                    if cmp_p.returncode == 0 and cmp_p.stdout:
                                        raw_cmp = json.loads(cmp_p.stdout.strip())
                                        sanitized_batch["top_match_details"] = {
                                            "compared_document": top_doc_name,
                                            "similarity_score": raw_cmp.get("similarity_score"),
                                            "similarity_level": raw_cmp.get("similarity_level"),
                                            "word_similarity": raw_cmp.get("word_similarity"),
                                            "sentence_similarity": raw_cmp.get("sentence_similarity"),
                                            "phrase_similarity": raw_cmp.get("phrase_similarity"),
                                            "matching_phrase_count": raw_cmp.get("matching_phrase_count"),
                                            "matching_phrase_words": raw_cmp.get("matching_phrase_words"),
                                            "common_words": raw_cmp.get("common_words"),
                                            "matching_sentences": raw_cmp.get("matching_sentences"),
                                            "phrases": raw_cmp.get("phrases", [])
                                        }
                                except Exception:
                                    pass

                        plagiarism_section["status"] = "completed"
                        plagiarism_section["reference_documents_checked"] = sanitized_batch["total_compared"]
                        plagiarism_section["result"] = sanitized_batch
                except Exception as ex:
                    plagiarism_section["status"] = "failed"
                    plagiarism_section["error"] = sanitize_error_message(f"Failed to execute C engine: {str(ex)}")

        # 4. Formulate Unified Response
        words = plain_text.split()
        sentences = [s for s in re.split(r'[.!?]+', plain_text) if s.strip()]

        overall_success = (
            (ai_section.get("status") in ("completed", "insufficient_text")) or
            (plagiarism_section.get("status") in ("completed", "unavailable"))
        )

        response_payload = {
            "success": overall_success,
            "filename": safe_name,
            "format": ext.lstrip(".").upper(),
            "document": {
                "filename": safe_name,
                "format": ext.lstrip(".").upper(),
                "file_size": file_size,
                "word_count": len(words),
                "sentence_count": len(sentences),
                "text": plain_text
            },
            "analysis": {
                "ai_detection": ai_section,
                "plagiarism_detection": plagiarism_section
            },
            "limitations": [
                "AI detection is heuristic and does not prove authorship.",
                "Scores reflect statistical regularity, vocabulary patterns, and discourse heuristics, not mathematical certainty.",
                "Never use AI analysis as the sole basis for academic-integrity or disciplinary decisions.",
                "Plagiarism results depend strictly on the reference documents available in the local corpus.",
                "Plagiarism analysis does not search the public internet or external proprietary databases."
            ]
        }

        # Ensure zero absolute server paths in output
        scrubbed_payload = scrub_server_paths(response_payload)
        status_code = 200 if overall_success else 500
        self.send_json(scrubbed_payload, status_code)


class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def get_port():
    """Resolve port from CLI argument, PORT environment variable (Render), or default."""
    # 1. Command-line argument: python server.py <port>
    if len(sys.argv) > 1:
        try:
            return int(sys.argv[1])
        except ValueError:
            pass
    # 2. Render / Cloud environment variable
    env_port = os.environ.get("PORT")
    if env_port:
        try:
            return int(env_port)
        except ValueError:
            pass
    # 3. Fallback for local development
    return DEFAULT_PORT


def run_server(host=HOST, port=None):
    if port is None:
        port = get_port()
    ensure_c_executable()
    server_address = (host, port)
    with ThreadedHTTPServer(server_address, PlagiarismRequestHandler) as httpd:
        print(f"Plagiarism Detection Server running on http://{host}:{port}")
        print(f"Loaded C Engine executable: {C_EXECUTABLE}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer shutting down gracefully.")
            httpd.shutdown()


if __name__ == "__main__":
    run_server()
