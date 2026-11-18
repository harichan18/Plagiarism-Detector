/**
 * Plagiarism Detection System — AI Content Detection & Plagiarism Analysis
 * Frontend Application Controller (ui/app.js)
 * 
 * Interacts with:
 *   - GET  /api/health            (Backend connectivity & engine health)
 *   - GET  /api/sample-docs       (Preloaded institutional reference files)
 *   - POST /api/analyze-combined  (Unified single-document AI + C11 plagiarism analysis)
 */

// ============================================================================
// 1. API Base Resolution (Local vs Deployed Render Configuration)
// ============================================================================

const IS_LOCAL =
  window.location.hostname === 'localhost' ||
  window.location.hostname === '127.0.0.1' ||
  window.location.hostname.startsWith('192.168.') ||
  window.location.hostname.startsWith('10.') ||
  window.location.hostname === '' ||
  window.location.protocol === 'file:';

const API_BASE = IS_LOCAL
  ? (window.location.origin && window.location.origin.startsWith('http') ? window.location.origin : 'http://localhost:8080')
  : 'https://plagiarism-detector-yyg0.onrender.com';

// ============================================================================
// 2. Application State & Storage
// ============================================================================

const state = {
  selectedFile: null,      // { name, size, type, content, isBase64, ext }
  isAnalyzing: false,
  currentAnalysis: null,   // Latest combined API payload
  sentenceFilter: 'all',   // 'all' | 'flagged' | 'clean'
  backendHealth: null,     // { status, engine, ai_engine, executable }
  history: []              // Session & localStorage audit trail
};

const SUPPORTED_EXTENSIONS = ['.pdf', '.docx', '.pptx', '.txt'];
const MAX_UPLOAD_SIZE = 50 * 1024 * 1024; // 50 MB
const HISTORY_STORAGE_KEY = 'pds_audit_history';

// ============================================================================
// 3. DOM Element References
// ============================================================================

const elements = {};

function initDOMElements() {
  // Navigation & Health
  elements.navTabs = document.querySelectorAll('.nav-tab');
  elements.tabPanels = document.querySelectorAll('.tab-content-panel');
  elements.statusDot = document.getElementById('status-dot');
  elements.statusLabel = document.getElementById('status-label');
  elements.connectionStatus = document.getElementById('connection-status');

  // Alerts
  elements.alertContainer = document.getElementById('alert-container');
  elements.alertMessage = document.getElementById('alert-message');
  elements.alertClose = document.getElementById('alert-close');

  // Single Document Upload Zone
  elements.dropZone = document.getElementById('drop-zone');
  elements.fileInput = document.getElementById('file-input');
  elements.browseBtn = document.getElementById('browse-btn');
  elements.uploadIdleView = document.getElementById('upload-idle-view');
  elements.uploadSelectedView = document.getElementById('upload-selected-view');
  elements.selectedFileExt = document.getElementById('selected-file-ext');
  elements.selectedFileName = document.getElementById('selected-file-name');
  elements.selectedFileSize = document.getElementById('selected-file-size');
  elements.btnRemoveFile = document.getElementById('btn-remove-file');
  elements.btnLoadSample = document.getElementById('btn-load-sample');

  // Action Buttons
  elements.btnAnalyze = document.getElementById('btn-analyze');
  elements.btnResetInput = document.getElementById('btn-reset-input');
  elements.analyzeSpinner = document.getElementById('analyze-spinner');
  elements.analyzeBtnText = document.getElementById('analyze-btn-text');
  elements.processingBanner = document.getElementById('processing-banner');
  elements.processingStepTitle = document.getElementById('processing-step-title');
  elements.processingStepDesc = document.getElementById('processing-step-desc');

  // Results Containers
  elements.emptyState = document.getElementById('empty-state');
  elements.resultsContainer = document.getElementById('results-container');

  // Document Summary Strip
  elements.metaDocName = document.getElementById('meta-doc-name');
  elements.metaDocFormat = document.getElementById('meta-doc-format');
  elements.metaWordCount = document.getElementById('meta-word-count');
  elements.metaSentenceCount = document.getElementById('meta-sentence-count');
  elements.metaCharCount = document.getElementById('meta-char-count');
  elements.metaFileSize = document.getElementById('meta-file-size');
  elements.btnQuickPrint = document.getElementById('btn-quick-print');
  elements.btnQuickExport = document.getElementById('btn-quick-export');

  // Full-Width AI Detection Overview Card
  elements.aiGaugeFill = document.getElementById('ai-gauge-fill');
  elements.aiScoreDisplay = document.getElementById('ai-score-display');
  elements.aiVerdictBadge = document.getElementById('ai-verdict-badge');
  elements.aiVerdictDesc = document.getElementById('ai-verdict-desc');
  elements.aiMetricFlagged = document.getElementById('ai-metric-flagged');
  elements.aiMetricTotalSentences = document.getElementById('ai-metric-total-sentences');
  elements.aiMetricWords = document.getElementById('ai-metric-words');
  elements.aiMetricSufficiency = document.getElementById('ai-metric-sufficiency');
  elements.aiMetricRawScore = document.getElementById('ai-metric-raw-score');
  elements.aiInsufficientBox = document.getElementById('ai-insufficient-box');
  elements.insufficientWordCount = document.getElementById('insufficient-word-count');
  elements.aiFailureBox = document.getElementById('ai-failure-box');
  elements.aiFailureMessage = document.getElementById('ai-failure-message');

  // Left Column: Breakdown & Sentences
  elements.sentencesList = document.getElementById('sentences-list');
  elements.countAllSentences = document.getElementById('count-all-sentences');
  elements.countFlaggedSentences = document.getElementById('count-flagged-sentences');
  elements.countCleanSentences = document.getElementById('count-clean-sentences');
  elements.filterChips = document.querySelectorAll('.sentence-filter-group .filter-chip');

  elements.summaryMattr = document.getElementById('summary-mattr');
  elements.summarySentCv = document.getElementById('summary-sent-cv');
  elements.summaryTrigram = document.getElementById('summary-trigram');
  elements.summaryPhraseDensity = document.getElementById('summary-phrase-density');
  elements.summaryTricolons = document.getElementById('summary-tricolons');
  elements.summaryNegation = document.getElementById('summary-negation');
  elements.summarySelfQa = document.getElementById('summary-self-qa');
  elements.summaryParaCv = document.getElementById('summary-para-cv');

  // Right Column: Interactive Document Preview
  elements.previewCharsCount = document.getElementById('preview-chars-count');
  elements.previewWordsCount = document.getElementById('preview-words-count');
  elements.previewSentencesCount = document.getElementById('preview-sentences-count');
  elements.documentTextContainer = document.getElementById('document-text-container');

  // Detailed Technical Signals
  elements.sigMattr = document.getElementById('sig-mattr');
  elements.sigSentCv = document.getElementById('sig-sent-cv');
  elements.sigTrigram = document.getElementById('sig-trigram');
  elements.sigPhraseDensity = document.getElementById('sig-phrase-density');
  elements.sigPunctEntropy = document.getElementById('sig-punct-entropy');
  elements.sigStyloScore = document.getElementById('sig-stylo-score');

  elements.sigScripts = document.getElementById('sig-scripts');
  elements.sigInvisibles = document.getElementById('sig-invisibles');
  elements.sigSpaces = document.getElementById('sig-spaces');
  elements.sigHomoglyphs = document.getElementById('sig-homoglyphs');
  elements.sigCurlyQuotes = document.getElementById('sig-curly-quotes');
  elements.sigUnicodeScore = document.getElementById('sig-unicode-score');

  elements.sigTricolons = document.getElementById('sig-tricolons');
  elements.sigNegation = document.getElementById('sig-negation');
  elements.sigSelfQa = document.getElementById('sig-self-qa');
  elements.sigPunchline = document.getElementById('sig-punchline');
  elements.sigParaCv = document.getElementById('sig-para-cv');
  elements.sigDiscourseScore = document.getElementById('sig-discourse-score');

  // Plagiarism Tab Elements (Separate Feature)
  elements.plagTabResultsView = document.getElementById('plag-tab-results-view');
  elements.plagNoDocPrompt = document.getElementById('plag-no-doc-prompt');
  elements.btnPlagGoUpload = document.getElementById('btn-plag-go-upload');
  elements.btnPlagAnalyzeCurrent = document.getElementById('btn-plag-analyze-current');

  elements.plagGaugeFill = document.getElementById('plag-gauge-fill');
  elements.plagTopScoreVal = document.getElementById('plag-top-score-val');
  elements.plagSimBadge = document.getElementById('plag-sim-badge');
  elements.plagTopDocName = document.getElementById('plag-top-doc-name');
  elements.plagMatchingPhrases = document.getElementById('plag-matching-phrases');
  elements.plagPhraseWords = document.getElementById('plag-phrase-words');
  elements.plagDocsChecked = document.getElementById('plag-docs-checked');
  elements.plagCorpusStatusText = document.getElementById('plag-corpus-status-text');
  elements.plagUnavailableBox = document.getElementById('plag-unavailable-box');
  elements.plagUnavailableMessage = document.getElementById('plag-unavailable-message');
  elements.plagFailureBox = document.getElementById('plag-failure-box');
  elements.plagFailureMessage = document.getElementById('plag-failure-message');

  elements.scoreWordSim = document.getElementById('score-word-sim');
  elements.barWordSim = document.getElementById('bar-word-sim');
  elements.scoreSentenceSim = document.getElementById('score-sentence-sim');
  elements.barSentenceSim = document.getElementById('bar-sentence-sim');
  elements.scorePhraseSim = document.getElementById('score-phrase-sim');
  elements.barPhraseSim = document.getElementById('bar-phrase-sim');
  elements.rankingsCountBadge = document.getElementById('rankings-count-badge');
  elements.rankingsTbody = document.getElementById('rankings-tbody');
  elements.phraseCardsGrid = document.getElementById('phrase-cards-grid');
  elements.phrasesCountPill = document.getElementById('phrases-count-pill');

  // History Tab Elements
  elements.historyTbody = document.getElementById('history-tbody');
  elements.btnClearHistory = document.getElementById('btn-clear-history');

  // Export Buttons
  elements.btnExportHtml = document.getElementById('btn-export-html');
  elements.btnExportPdf = document.getElementById('btn-export-pdf');
  elements.btnExportJson = document.getElementById('btn-export-json');

  // Plaintext Modal
  elements.textReportModal = document.getElementById('text-report-modal');
  elements.modalCloseBtn = document.getElementById('modal-close-btn');
  elements.modalPlaintextContent = document.getElementById('modal-plaintext-content');
  elements.modalCopyBtn = document.getElementById('modal-copy-btn');
  elements.modalDownloadBtn = document.getElementById('modal-download-btn');
}

// ============================================================================
// 4. Utility Functions (Safe Formatting & Escaping)
// ============================================================================

function escapeHtml(str) {
  if (str === null || str === undefined) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

function formatBytes(bytes) {
  if (!bytes || bytes <= 0) return '0 B';
  const units = ['B', 'KB', 'MB', 'GB'];
  const i = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  const val = (bytes / Math.pow(1024, i)).toFixed(i === 0 ? 0 : 1);
  return `${val} ${units[i]}`;
}

function formatAiScorePct(score) {
  if (score === null || score === undefined || score === 'Abstained') return 'Abstained';
  if (typeof score === 'string') {
    if (score.endsWith('%')) return score;
    const n = parseFloat(score);
    if (!isNaN(n)) return (n * 100).toFixed(2) + '%';
    return score;
  }
  if (typeof score === 'number' && !isNaN(score)) {
    return (score * 100).toFixed(2) + '%';
  }
  return '--';
}

function showAlert(message, isError = true) {
  elements.alertMessage.textContent = message;
  elements.alertContainer.classList.remove('hidden');
  if (isError) {
    elements.alertContainer.classList.remove('success');
  } else {
    elements.alertContainer.classList.add('success');
  }
}

function hideAlert() {
  elements.alertContainer.classList.add('hidden');
}

// ============================================================================
// 5. Backend Connectivity & Health Check
// ============================================================================

async function checkBackendHealth() {
  try {
    elements.statusDot.className = 'status-indicator-dot connecting';
    elements.statusLabel.textContent = 'Connecting...';

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 12000);

    const res = await fetch(`${API_BASE}/api/health`, {
      method: 'GET',
      signal: controller.signal
    });
    clearTimeout(timeoutId);

    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    state.backendHealth = data;

    if (data.status === 'healthy' && data.engine === 'ready') {
      elements.statusDot.className = 'status-indicator-dot connected';
      elements.statusLabel.textContent = 'Backend Ready';
      elements.connectionStatus.title = `Connected to C11 Plagiarism Engine (${data.executable || 'plagiarism_detector.exe'}) + AI Writing Engine (${data.ai_engine || 'ready'})`;
    } else {
      elements.statusDot.className = 'status-indicator-dot connecting';
      elements.statusLabel.textContent = 'Engine Standby';
      elements.connectionStatus.title = 'Backend online; C engine compilation or verification in progress';
    }
  } catch (err) {
    elements.statusDot.className = 'status-indicator-dot offline';
    elements.statusLabel.textContent = 'Backend Offline';
    elements.connectionStatus.title = `Unable to connect to ${API_BASE}. Free-tier host may be spinning up.`;
  }
}

// ============================================================================
// 6. Navigation Tabs
// ============================================================================

function initNavigation() {
  elements.navTabs.forEach(tab => {
    tab.addEventListener('click', () => {
      const targetId = tab.dataset.tab;
      elements.navTabs.forEach(t => t.classList.remove('active'));
      elements.tabPanels.forEach(p => p.classList.remove('active'));

      tab.classList.add('active');
      const targetPanel = document.getElementById(targetId);
      if (targetPanel) targetPanel.classList.add('active');

      if (targetId === 'tab-history') {
        renderHistoryTable();
      } else if (targetId === 'tab-plagiarism') {
        updatePlagiarismTabState();
      }
    });
  });

  if (elements.btnPlagAnalyzeCurrent) {
    elements.btnPlagAnalyzeCurrent.addEventListener('click', () => {
      const aiTab = document.getElementById('nav-btn-ai-detector');
      if (aiTab) aiTab.click();
      if (state.selectedFile && !state.isAnalyzing) {
        executeCombinedAnalysis();
      } else if (!state.selectedFile) {
        elements.fileInput.click();
      }
    });
  }

  if (elements.btnPlagGoUpload) {
    elements.btnPlagGoUpload.addEventListener('click', () => {
      const aiTab = document.getElementById('nav-btn-ai-detector');
      if (aiTab) aiTab.click();
      elements.dropZone.scrollIntoView({ behavior: 'smooth', block: 'center' });
    });
  }
}

function updatePlagiarismTabState() {
  if (state.currentAnalysis && state.currentAnalysis.analysis?.plagiarism_detection) {
    elements.plagNoDocPrompt.classList.add('hidden');
    elements.plagTabResultsView.classList.remove('hidden');
  } else {
    elements.plagNoDocPrompt.classList.remove('hidden');
    elements.plagTabResultsView.classList.add('hidden');
  }
}

// ============================================================================
// 7. Single Document Upload & Drag-and-Drop
// ============================================================================

function initUploadHandlers() {
  const dropZone = elements.dropZone;
  const fileInput = elements.fileInput;
  const browseBtn = elements.browseBtn;

  browseBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    fileInput.click();
  });

  dropZone.addEventListener('click', () => {
    if (!state.selectedFile) {
      fileInput.click();
    }
  });

  dropZone.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' || e.key === ' ') {
      e.preventDefault();
      fileInput.click();
    }
  });

  ['dragenter', 'dragover'].forEach(name => {
    dropZone.addEventListener(name, (e) => {
      e.preventDefault();
      e.stopPropagation();
      dropZone.classList.add('dragover');
    });
  });

  ['dragleave', 'drop'].forEach(name => {
    dropZone.addEventListener(name, (e) => {
      e.preventDefault();
      e.stopPropagation();
      dropZone.classList.remove('dragover');
    });
  });

  dropZone.addEventListener('drop', (e) => {
    const files = e.dataTransfer.files;
    if (files && files.length > 0) {
      processFile(files[0]);
    }
  });

  fileInput.addEventListener('change', (e) => {
    if (e.target.files && e.target.files.length > 0) {
      processFile(e.target.files[0]);
    }
  });

  elements.btnRemoveFile.addEventListener('click', (e) => {
    e.stopPropagation();
    resetFileInput();
  });

  elements.btnResetInput.addEventListener('click', () => {
    resetFileInput();
  });

  elements.btnLoadSample.addEventListener('click', handleLoadSample);
}

function processFile(file) {
  hideAlert();
  if (!file) return;

  const ext = '.' + file.name.split('.').pop().toLowerCase();
  if (!SUPPORTED_EXTENSIONS.includes(ext)) {
    showAlert(`Unsupported file format "${ext}". Supported formats: PDF, DOCX, PPTX, TXT.`);
    return;
  }

  if (file.size === 0) {
    showAlert(`Selected file "${file.name}" is empty (0 bytes).`);
    return;
  }

  if (file.size > MAX_UPLOAD_SIZE) {
    showAlert(`File exceeds maximum size limit of 50 MB.`);
    return;
  }

  const reader = new FileReader();

  if (ext === '.txt') {
    reader.onload = (e) => {
      const textContent = e.target.result;
      state.selectedFile = {
        name: file.name,
        size: file.size,
        type: file.type || 'text/plain',
        ext: 'TXT',
        isBase64: false,
        content: textContent,
        rawText: textContent
      };
      updateUploadUIWithFile();
    };
    reader.readAsText(file, 'utf-8');
  } else {
    // Read binary files as Data URL Base64
    reader.onload = (e) => {
      const dataUrl = e.target.result;
      const base64 = dataUrl.includes(',') ? dataUrl.split(',')[1] : dataUrl;
      state.selectedFile = {
        name: file.name,
        size: file.size,
        type: file.type || 'application/octet-stream',
        ext: ext.replace('.', '').toUpperCase(),
        isBase64: true,
        content: base64,
        rawText: ''
      };
      updateUploadUIWithFile();
    };
    reader.readAsDataURL(file);
  }
}

function updateUploadUIWithFile() {
  if (!state.selectedFile) return;

  elements.uploadIdleView.classList.add('hidden');
  elements.uploadSelectedView.classList.remove('hidden');

  elements.selectedFileExt.textContent = state.selectedFile.ext;
  elements.selectedFileName.textContent = state.selectedFile.name;
  elements.selectedFileSize.textContent = `${formatBytes(state.selectedFile.size)} • Ready for analysis`;

  elements.btnAnalyze.disabled = false;
  elements.btnResetInput.disabled = false;
}

function resetFileInput() {
  state.selectedFile = null;
  elements.fileInput.value = '';

  elements.uploadSelectedView.classList.add('hidden');
  elements.uploadIdleView.classList.remove('hidden');

  elements.btnAnalyze.disabled = true;
  elements.btnResetInput.disabled = true;
}

async function handleLoadSample() {
  try {
    elements.btnLoadSample.disabled = true;
    elements.btnLoadSample.textContent = 'Loading...';

    const res = await fetch(`${API_BASE}/api/sample-docs`);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();

    if (!data.success || !data.samples || data.samples.length === 0) {
      throw new Error('No sample documents currently available on server.');
    }

    const sample = data.samples[0]; // sample1.txt
    let content = sample.content || '';
    if (!content) {
      const docRes = await fetch(`${API_BASE}/documents/${sample.filename}`);
      if (docRes.ok) {
        content = await docRes.text();
      } else {
        content = "My name is Rahul, and I am a computer science student who enjoys learning about technology and developing software projects. I am particularly interested in programming, data structures, algorithms, and application development.\n\nDuring my academic journey, I have worked on several projects that helped me understand how theoretical concepts can be applied to real-world problems. I enjoy building applications and experimenting with different technologies to improve my programming skills.\n\nOne of my major projects is a plagiarism detection and document similarity analysis system. The project uses data structures and algorithms to compare documents and identify similar content. I implemented a hash table for word frequency analysis and used KMP and Rabin-Karp algorithms for matching phrases.\n\nThe project also uses Merge Sort to rank documents according to their similarity percentage. It generates detailed similarity reports containing matching phrases, document statistics, and similarity scores.\n\nWorking on this project has improved my understanding of data structures, string matching, memory management, and algorithm complexity. It has also taught me how to organize a larger C project into multiple modules and test each component separately.\n\nApart from academics, I like exploring new technologies and improving my problem-solving abilities. My goal is to continue developing useful software applications and gain practical experience through challenging projects.";
      }
    }

    state.selectedFile = {
      name: sample.filename,
      size: sample.size || content.length,
      type: 'text/plain',
      ext: 'TXT',
      isBase64: false,
      content: content,
      rawText: content
    };

    updateUploadUIWithFile();
    showAlert(`Sample document "${sample.filename}" loaded successfully.`, false);
  } catch (err) {
    showAlert(`Could not load sample document: ${err.message}`);
  } finally {
    elements.btnLoadSample.disabled = false;
    elements.btnLoadSample.innerHTML = `
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg>
      Load Sample Document
    `;
  }
}

// ============================================================================
// 8. Document Analysis Execution (POST /api/analyze-combined)
// ============================================================================

function initAnalysisHandler() {
  elements.btnAnalyze.addEventListener('click', executeCombinedAnalysis);
}

async function executeCombinedAnalysis() {
  if (!state.selectedFile || state.isAnalyzing) return;

  hideAlert();
  state.isAnalyzing = true;
  elements.btnAnalyze.disabled = true;
  elements.analyzeSpinner.classList.remove('hidden');
  elements.analyzeBtnText.textContent = 'Analyzing...';
  elements.processingBanner.classList.remove('hidden');
  elements.processingStepTitle.textContent = 'Extracting and evaluating document...';
  elements.processingStepDesc.textContent = 'Executing AI writing pattern evaluation and C11 Merge Sort corpus comparisons.';

  const payload = {
    filename: state.selectedFile.name,
    content: state.selectedFile.content,
    is_base64: state.selectedFile.isBase64
  };

  try {
    const res = await fetch(`${API_BASE}/api/analyze-combined`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const data = await res.json();

    if (!res.ok && !data.analysis) {
      const errMsg = data.error || `Server returned error HTTP ${res.status}`;
      throw new Error(errMsg);
    }

    state.currentAnalysis = data;
    saveToHistory(data);
    renderAnalysisResults(data);

    // Scroll smoothly to results overview
    elements.resultsContainer.scrollIntoView({ behavior: 'smooth', block: 'start' });
  } catch (err) {
    showAlert(`Analysis failed: ${err.message}`);
  } finally {
    state.isAnalyzing = false;
    elements.btnAnalyze.disabled = false;
    elements.analyzeSpinner.classList.add('hidden');
    elements.analyzeBtnText.textContent = 'Analyze Document';
    elements.processingBanner.classList.add('hidden');
  }
}

// ============================================================================
// 9. Results Rendering Engine
// ============================================================================

function renderAnalysisResults(data) {
  if (!data || !data.analysis) return;

  elements.emptyState.classList.add('hidden');
  elements.resultsContainer.classList.remove('hidden');

  const doc = data.document || {};
  const ai = data.analysis.ai_detection || {};
  const plag = data.analysis.plagiarism_detection || {};

  // Extract raw text for preview
  const plainText = doc.text || (state.selectedFile && !state.selectedFile.isBase64 ? state.selectedFile.content : '');

  // 1. Top Document Summary Strip
  elements.metaDocName.textContent = data.filename || doc.filename || 'Unknown Document';
  elements.metaDocFormat.textContent = data.format || doc.format || 'TXT';
  elements.metaWordCount.textContent = (doc.word_count || 0).toLocaleString();
  elements.metaSentenceCount.textContent = (doc.sentence_count || 0).toLocaleString();
  elements.metaCharCount.textContent = (plainText ? plainText.length : (doc.word_count || 0) * 6).toLocaleString();
  elements.metaFileSize.textContent = formatBytes(doc.file_size || state.selectedFile?.size || 0);

  // 2. Render Full-Width AI Content Detection Result Card (EXCLUSIVE on AI Detector Screen)
  renderAICard(ai);

  // 3. Render Left Column: Detailed Sentence Analysis & Stylometry/Discourse Summaries
  renderSentenceBreakdown(ai.result);
  renderStylometrySummary(ai.result);
  renderDiscourseSummary(ai.result);

  // 4. Render Right Column: Interactive Document Text Preview
  renderDocumentPreview(plainText, ai.result);

  // 5. Render Detailed Technical Signals Drawer
  renderTechnicalSignals(ai.result);

  // 6. Update Plagiarism Tab View (Kept in its separate feature tab)
  renderPlagiarismTabDetails(plag.result, plag);
}

// ============================================================================
// 9.1 Render Full-Width Card: AI Content Detection Overview
// ============================================================================

function renderAICard(ai) {
  elements.aiInsufficientBox.classList.add('hidden');
  elements.aiFailureBox.classList.add('hidden');

  const GAUGE_CIRCUMFERENCE = 326.73; // 2 * PI * 52

  if (ai.status === 'failed' || ai.status === 'unavailable') {
    elements.aiVerdictBadge.className = 'verdict-pill verdict-unknown';
    elements.aiVerdictBadge.textContent = ai.status === 'failed' ? 'Analysis Failed' : 'Engine Unavailable';
    elements.aiScoreDisplay.textContent = '--';
    elements.aiGaugeFill.style.strokeDashoffset = GAUGE_CIRCUMFERENCE;
    elements.aiGaugeFill.style.stroke = '#94a3b8';
    elements.aiVerdictDesc.textContent = 'AI writing pattern evaluation could not complete.';
    elements.aiFailureBox.classList.remove('hidden');
    elements.aiFailureMessage.textContent = ai.error || 'The AI detection core encountered an execution problem.';
    elements.aiMetricWords.textContent = '-';
    elements.aiMetricFlagged.textContent = '-';
    elements.aiMetricTotalSentences.textContent = '-';
    elements.aiMetricSufficiency.textContent = 'Unknown';
    elements.aiMetricRawScore.textContent = '--';
    return;
  }

  const aiRes = ai.result || {};
  const assessment = aiRes.overall_assessment || {};
  const verdictRaw = (assessment.verdict || 'unknown').toLowerCase();
  const scoreRaw = assessment.score; // Heuristic float from 0.00 to 1.00
  const wordCount = aiRes.word_count || 0;
  const isInsufficient = ai.status === 'insufficient_text' || assessment.is_sufficient_length === false || wordCount < 150;

  const sentences = aiRes.sentence_analysis || [];
  let flaggedCount = 0;
  sentences.forEach(s => {
    const hasSignal = (s.score && s.score > 0) || (s.reasons && s.reasons.length > 0 && !s.reasons[0].toLowerCase().includes('no ai signal'));
    if (hasSignal) flaggedCount++;
  });

  elements.aiMetricWords.textContent = wordCount.toLocaleString();
  elements.aiMetricFlagged.textContent = `${flaggedCount} sentence${flaggedCount === 1 ? '' : 's'}`;
  elements.aiMetricTotalSentences.textContent = sentences.length.toLocaleString();

  // Handle Insufficient Evidence (Abstention State)
  if (isInsufficient) {
    elements.aiVerdictBadge.className = 'verdict-pill verdict-insufficient';
    elements.aiVerdictBadge.textContent = 'Insufficient Evidence';
    elements.aiScoreDisplay.textContent = 'Abstained';
    elements.aiGaugeFill.style.strokeDashoffset = GAUGE_CIRCUMFERENCE;
    elements.aiGaugeFill.style.stroke = '#94a3b8';
    elements.aiVerdictDesc.textContent = 'Document length is below the heuristic floor of 150 words. Global scoring is abstained to prevent false positives.';
    elements.aiMetricSufficiency.textContent = 'Below 150 Words (Abstained)';
    elements.aiMetricRawScore.textContent = 'Abstained';
    elements.insufficientWordCount.textContent = wordCount.toLocaleString();
    elements.aiInsufficientBox.classList.remove('hidden');
    return;
  }

  // Normal Heuristic Scoring (0.00 to 1.00 converted to percentage representation)
  let scoreFloat = 0.0;
  if (typeof scoreRaw === 'number') {
    scoreFloat = Math.min(Math.max(scoreRaw, 0.0), 1.0);
    const scorePct = (scoreFloat * 100).toFixed(2) + '%';
    elements.aiScoreDisplay.textContent = scorePct;
    elements.aiMetricRawScore.textContent = scorePct;
  } else {
    elements.aiScoreDisplay.textContent = '--';
    elements.aiMetricRawScore.textContent = '--';
  }

  elements.aiMetricSufficiency.textContent = '≥ 150 Words (Sufficient)';

  // Animate Gauge
  const offset = GAUGE_CIRCUMFERENCE * (1 - scoreFloat);
  elements.aiGaugeFill.style.strokeDashoffset = offset;

  // Verdict Badge, Color & Description Mapping
  if (verdictRaw.includes('human')) {
    elements.aiVerdictBadge.className = 'verdict-pill verdict-human';
    elements.aiVerdictBadge.textContent = 'Likely Human';
    elements.aiGaugeFill.style.stroke = '#059669';
    elements.aiVerdictDesc.textContent = 'Writing demonstrates natural lexical diversity, varied sentence lengths, and human discourse progression.';
  } else if (verdictRaw.includes('uncertain')) {
    elements.aiVerdictBadge.className = 'verdict-pill verdict-uncertain';
    elements.aiVerdictBadge.textContent = 'Uncertain / Mixed Signals';
    elements.aiGaugeFill.style.stroke = '#d97706';
    elements.aiVerdictDesc.textContent = 'Text exhibits mixed patterns. Some stylistic uniformity detected without sufficient certainty.';
  } else if (verdictRaw.includes('ai') || verdictRaw.includes('generated')) {
    elements.aiVerdictBadge.className = 'verdict-pill verdict-ai';
    elements.aiVerdictBadge.textContent = 'Likely AI-Generated';
    elements.aiGaugeFill.style.stroke = '#7c3aed';
    elements.aiVerdictDesc.textContent = 'High statistical regularity, repetitive trigrams, and characteristic discourse formulations detected.';
  } else {
    elements.aiVerdictBadge.className = 'verdict-pill verdict-insufficient';
    elements.aiVerdictBadge.textContent = 'Insufficient Evidence';
    elements.aiGaugeFill.style.stroke = '#64748b';
    elements.aiVerdictDesc.textContent = 'Sample length is below confidence floor. Scoring abstained to prevent false positives.';
  }
}

// ============================================================================
// 9.2 Render Left Column: Detailed Sentence Analysis & Summaries
// ============================================================================

function renderSentenceBreakdown(aiRes) {
  elements.sentencesList.innerHTML = '';
  if (!aiRes || !aiRes.sentence_analysis || aiRes.sentence_analysis.length === 0) {
    elements.sentencesList.innerHTML = '<div class="table-empty" style="text-align:center; padding:20px; color:var(--text-muted);">No sentence spans available for this document.</div>';
    elements.countAllSentences.textContent = '0';
    elements.countFlaggedSentences.textContent = '0';
    elements.countCleanSentences.textContent = '0';
    return;
  }

  const sentences = aiRes.sentence_analysis;
  let flaggedCount = 0;
  let cleanCount = 0;

  sentences.forEach((s, idx) => {
    const hasSignal = (s.score && s.score > 0) || (s.reasons && s.reasons.length > 0 && !s.reasons[0].toLowerCase().includes('no ai signal'));
    if (hasSignal) flaggedCount++;
    else cleanCount++;

    const card = document.createElement('div');
    card.className = `sentence-card ${hasSignal ? 'has-signal' : 'is-clean'}`;
    card.dataset.status = hasSignal ? 'flagged' : 'clean';
    card.dataset.sentenceIndex = idx;
    card.id = `sentence-card-${idx}`;

    const scoreDisplay = typeof s.score === 'number' ? (s.score * 100).toFixed(2) + '%' : '0.00%';
    const reasonsHtml = (s.reasons || [])
      .map(r => `<span class="reason-pill">${escapeHtml(r)}</span>`)
      .join(' ');

    const words = s.word_count || 0;
    const confidenceTag = words >= 8 ? 'Confidence: Standard' : 'Low Confidence (Short)';

    card.innerHTML = `
      <div class="sentence-meta-row">
        <span class="sentence-idx-tag">Sentence #${s.sentence_index || (idx + 1)} • ${words} words • ${confidenceTag}</span>
        <span class="sentence-score-tag">Signal Score: ${scoreDisplay}</span>
      </div>
      <p class="sentence-text">${escapeHtml(s.text)}</p>
      <div class="sentence-reasons-row">${reasonsHtml}</div>
    `;

    // Click on sentence card scrolls and highlights the preview span on right
    card.addEventListener('click', () => {
      highlightLinkedSentence(idx);
    });

    elements.sentencesList.appendChild(card);
  });

  elements.countAllSentences.textContent = sentences.length.toLocaleString();
  elements.countFlaggedSentences.textContent = flaggedCount.toLocaleString();
  elements.countCleanSentences.textContent = cleanCount.toLocaleString();

  applySentenceFilter(state.sentenceFilter);
}

function initSentenceFilters() {
  elements.filterChips.forEach(chip => {
    chip.addEventListener('click', () => {
      elements.filterChips.forEach(c => c.classList.remove('active'));
      chip.classList.add('active');
      state.sentenceFilter = chip.dataset.filter;
      applySentenceFilter(state.sentenceFilter);
    });
  });
}

function applySentenceFilter(filterType) {
  const cards = elements.sentencesList.querySelectorAll('.sentence-card');
  cards.forEach(card => {
    if (filterType === 'all') {
      card.style.display = 'block';
    } else if (filterType === 'flagged') {
      card.style.display = card.dataset.status === 'flagged' ? 'block' : 'none';
    } else if (filterType === 'clean') {
      card.style.display = card.dataset.status === 'clean' ? 'block' : 'none';
    }
  });
}

function renderStylometrySummary(aiRes) {
  if (!aiRes || !aiRes.signals || !aiRes.signals.stylometry) {
    elements.summaryMattr.textContent = '-';
    elements.summarySentCv.textContent = '-';
    elements.summaryTrigram.textContent = '-';
    elements.summaryPhraseDensity.textContent = '-';
    return;
  }

  const sty = aiRes.signals.stylometry;
  elements.summaryMattr.textContent = typeof sty.lexical_diversity_mattr === 'number' ? sty.lexical_diversity_mattr.toFixed(3) : '-';
  elements.summarySentCv.textContent = typeof sty.sentence_length_cv === 'number' ? sty.sentence_length_cv.toFixed(3) : '-';
  elements.summaryTrigram.textContent = typeof sty.repeated_trigram_ratio === 'number' ? sty.repeated_trigram_ratio.toFixed(3) : '-';
  elements.summaryPhraseDensity.textContent = typeof sty.ai_phrase_density_per_1000w === 'number' ? `${sty.ai_phrase_density_per_1000w.toFixed(1)}/1kw` : '-';
}

function renderDiscourseSummary(aiRes) {
  if (!aiRes || !aiRes.signals || !aiRes.signals.discourse) {
    elements.summaryTricolons.textContent = '-';
    elements.summaryNegation.textContent = '-';
    elements.summarySelfQa.textContent = '-';
    elements.summaryParaCv.textContent = '-';
    return;
  }

  const disc = aiRes.signals.discourse;
  elements.summaryTricolons.textContent = typeof disc.tricolons_per_1000w === 'number' ? `${disc.tricolons_per_1000w.toFixed(1)}/1kw` : '0.0/1kw';
  elements.summaryNegation.textContent = typeof disc.negation_pivots_per_1000w === 'number' ? `${disc.negation_pivots_per_1000w.toFixed(1)}/1kw` : '0.0/1kw';
  elements.summarySelfQa.textContent = typeof disc.self_qa_per_1000w === 'number' ? `${disc.self_qa_per_1000w.toFixed(1)}/1kw` : '0.0/1kw';
  elements.summaryParaCv.textContent = typeof disc.paragraph_length_cv === 'number' ? disc.paragraph_length_cv.toFixed(3) : '-';
}

// ============================================================================
// 9.3 Render Right Column: Interactive Document Text Preview
// ============================================================================

function renderDocumentPreview(plainText, aiRes) {
  elements.documentTextContainer.innerHTML = '';

  const sentences = aiRes && aiRes.sentence_analysis ? aiRes.sentence_analysis : [];
  const words = plainText ? plainText.trim().split(/\s+/) : [];

  elements.previewCharsCount.textContent = `${(plainText ? plainText.length : 0).toLocaleString()} Chars`;
  elements.previewWordsCount.textContent = `${words.length.toLocaleString()} Words`;
  elements.previewSentencesCount.textContent = `${sentences.length.toLocaleString()} Sentences`;

  if (!plainText && sentences.length === 0) {
    elements.documentTextContainer.innerHTML = '<p class="text-placeholder">No extractable text available to preview.</p>';
    return;
  }

  // If sentence analysis exists, render sentence spans with highlighting
  if (sentences.length > 0) {
    const container = document.createElement('div');
    container.className = 'doc-paragraphs-wrap';

    sentences.forEach((s, idx) => {
      const hasSignal = (s.score && s.score > 0) || (s.reasons && s.reasons.length > 0 && !s.reasons[0].toLowerCase().includes('no ai signal'));
      const span = document.createElement('span');
      span.className = `doc-sentence ${hasSignal ? 'flagged-ai' : ''}`;
      span.dataset.sentenceIndex = idx;
      span.id = `doc-sentence-span-${idx}`;
      span.textContent = s.text + ' ';

      if (hasSignal) {
        span.title = `Flagged: ${(s.reasons || []).join(', ')}`;
      }

      // Bi-directional click
      span.addEventListener('click', () => {
        highlightLinkedSentence(idx);
      });

      container.appendChild(span);
    });

    elements.documentTextContainer.appendChild(container);
  } else {
    // Plain text fallback
    elements.documentTextContainer.textContent = plainText;
  }
}

function highlightLinkedSentence(idx) {
  // Clear existing active highlights
  document.querySelectorAll('.doc-sentence.active-highlight').forEach(el => el.classList.remove('active-highlight'));
  document.querySelectorAll('.sentence-card.active-card').forEach(el => el.classList.remove('active-card'));

  // Highlight preview span
  const targetSpan = document.getElementById(`doc-sentence-span-${idx}`);
  if (targetSpan) {
    targetSpan.classList.add('active-highlight');
    targetSpan.scrollIntoView({ behavior: 'smooth', block: 'center' });
  }

  // Highlight sentence card on left
  const targetCard = document.getElementById(`sentence-card-${idx}`);
  if (targetCard) {
    targetCard.classList.add('active-card');
    targetCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }
}

// ============================================================================
// 9.4 Render Detailed Technical Signals Drawer
// ============================================================================

function renderTechnicalSignals(aiRes) {
  if (!aiRes || !aiRes.signals) {
    elements.sigMattr.textContent = '-';
    elements.sigSentCv.textContent = '-';
    elements.sigTrigram.textContent = '-';
    elements.sigPhraseDensity.textContent = '-';
    elements.sigPunctEntropy.textContent = '-';
    elements.sigStyloScore.textContent = '-';

    elements.sigScripts.textContent = '-';
    elements.sigInvisibles.textContent = 'None';
    elements.sigSpaces.textContent = 'None';
    elements.sigHomoglyphs.textContent = 'None';
    elements.sigCurlyQuotes.textContent = '-';
    elements.sigUnicodeScore.textContent = '-';

    elements.sigTricolons.textContent = '-';
    elements.sigNegation.textContent = '-';
    elements.sigSelfQa.textContent = '-';
    elements.sigPunchline.textContent = '-';
    elements.sigParaCv.textContent = '-';
    elements.sigDiscourseScore.textContent = '-';
    return;
  }

  const s = aiRes.signals;
  const stylo = s.stylometry || {};
  const unicode = s.unicode || {};
  const disc = s.discourse || {};

  // Stylometry
  elements.sigMattr.textContent = typeof stylo.lexical_diversity_mattr === 'number' ? stylo.lexical_diversity_mattr.toFixed(4) : '-';
  elements.sigSentCv.textContent = typeof stylo.sentence_length_cv === 'number' ? stylo.sentence_length_cv.toFixed(4) : '-';
  elements.sigTrigram.textContent = typeof stylo.repeated_trigram_ratio === 'number' ? stylo.repeated_trigram_ratio.toFixed(4) : '-';
  elements.sigPhraseDensity.textContent = typeof stylo.ai_phrase_density_per_1000w === 'number' ? stylo.ai_phrase_density_per_1000w.toFixed(2) : '-';
  elements.sigPunctEntropy.textContent = typeof stylo.punctuation_entropy === 'number' ? stylo.punctuation_entropy.toFixed(4) : '-';
  elements.sigStyloScore.textContent = typeof stylo.score === 'number' ? (stylo.score * 100).toFixed(2) + '%' : '-';

  // Unicode
  elements.sigScripts.textContent = (unicode.scripts && unicode.scripts.length > 0) ? unicode.scripts.join(', ') : 'LATIN';
  elements.sigInvisibles.textContent = Object.keys(unicode.invisible_characters || {}).length > 0 ? `${Object.keys(unicode.invisible_characters).length} detected` : 'None';
  elements.sigSpaces.textContent = Object.keys(unicode.exotic_spaces || {}).length > 0 ? `${Object.keys(unicode.exotic_spaces).length} detected` : 'None';
  elements.sigHomoglyphs.textContent = Object.keys(unicode.homoglyphs || {}).length > 0 ? `${Object.keys(unicode.homoglyphs).length} detected` : 'None';
  const typo = unicode.typography || {};
  elements.sigCurlyQuotes.textContent = typeof typo.curly_quote_ratio === 'number' ? typo.curly_quote_ratio.toFixed(2) : '0.00';
  elements.sigUnicodeScore.textContent = typeof unicode.score === 'number' ? (unicode.score * 100).toFixed(2) + '%' : '0.00%';

  // Discourse
  elements.sigTricolons.textContent = typeof disc.tricolons_per_1000w === 'number' ? disc.tricolons_per_1000w.toFixed(2) : '0.00';
  elements.sigNegation.textContent = typeof disc.negation_pivots_per_1000w === 'number' ? disc.negation_pivots_per_1000w.toFixed(2) : '0.00';
  elements.sigSelfQa.textContent = typeof disc.self_qa_per_1000w === 'number' ? disc.self_qa_per_1000w.toFixed(2) : '0.00';
  elements.sigPunchline.textContent = typeof disc.punch_line_ratio === 'number' ? disc.punch_line_ratio.toFixed(4) : '0.0000';
  elements.sigParaCv.textContent = typeof disc.paragraph_length_cv === 'number' ? disc.paragraph_length_cv.toFixed(4) : '-';
  elements.sigDiscourseScore.textContent = typeof disc.score === 'number' ? (disc.score * 100).toFixed(2) + '%' : '-';
}

// ============================================================================
// 9.5 Render Plagiarism Tab (Separate Feature — Single Document vs Corpus)
// ============================================================================

function renderPlagiarismTabDetails(pRes, plag) {
  elements.rankingsTbody.innerHTML = '';
  elements.phraseCardsGrid.innerHTML = '';
  elements.plagUnavailableBox.classList.add('hidden');
  elements.plagFailureBox.classList.add('hidden');

  const GAUGE_CIRCUMFERENCE = 326.73;

  if (!pRes || (plag && plag.status === 'failed')) {
    elements.plagSimBadge.className = 'similarity-pill sim-unknown';
    elements.plagSimBadge.textContent = 'Engine Error';
    elements.plagTopScoreVal.textContent = '--';
    elements.plagGaugeFill.style.strokeDashoffset = GAUGE_CIRCUMFERENCE;
    elements.plagGaugeFill.style.stroke = '#94a3b8';
    elements.plagTopDocName.textContent = 'Error';
    elements.plagDocsChecked.textContent = '0';
    elements.plagFailureBox.classList.remove('hidden');
    elements.plagFailureMessage.textContent = (plag && plag.error) || 'The C11 plagiarism engine process failed.';
    elements.scoreWordSim.textContent = '0.00%';
    elements.barWordSim.style.width = '0%';
    elements.scoreSentenceSim.textContent = '0.00%';
    elements.barSentenceSim.style.width = '0%';
    elements.scorePhraseSim.textContent = '0.00%';
    elements.barPhraseSim.style.width = '0%';
    elements.rankingsTbody.innerHTML = '<tr><td colspan="4" class="text-center py-4">No reference comparisons available.</td></tr>';
    elements.phraseCardsGrid.innerHTML = '<div class="table-empty">No continuous matching phrases detected.</div>';
    return;
  }

  const checkedCount = (plag && plag.reference_documents_checked) || pRes.total_compared || 0;

  if (checkedCount === 0 || (plag && plag.status === 'unavailable')) {
    elements.plagSimBadge.className = 'similarity-pill sim-unknown';
    elements.plagSimBadge.textContent = 'Corpus Unavailable';
    elements.plagTopScoreVal.textContent = 'N/A';
    elements.plagGaugeFill.style.strokeDashoffset = GAUGE_CIRCUMFERENCE;
    elements.plagGaugeFill.style.stroke = '#94a3b8';
    elements.plagTopDocName.textContent = 'No References Available';
    elements.plagDocsChecked.textContent = '0';
    elements.plagCorpusStatusText.textContent = 'Reference corpus contains 0 candidate documents.';
    elements.plagUnavailableBox.classList.remove('hidden');
    elements.plagUnavailableMessage.textContent = (plag && plag.message) || 'No reference documents were found in the institutional repository for comparison.';
    elements.plagMatchingPhrases.textContent = '-';
    elements.plagPhraseWords.textContent = '-';
    elements.scoreWordSim.textContent = '0.00%';
    elements.barWordSim.style.width = '0%';
    elements.scoreSentenceSim.textContent = '0.00%';
    elements.barSentenceSim.style.width = '0%';
    elements.scorePhraseSim.textContent = '0.00%';
    elements.barPhraseSim.style.width = '0%';
    elements.rankingsTbody.innerHTML = '<tr><td colspan="4" class="text-center py-4">Reference corpus is empty.</td></tr>';
    elements.phraseCardsGrid.innerHTML = '<div class="table-empty">No continuous matching phrases detected.</div>';
    return;
  }

  const rankings = pRes.rankings || [];
  const topMatch = rankings.length > 0 ? rankings[0] : null;
  const topScore = topMatch ? Number(topMatch.similarity || 0) : 0.0;
  const levelStr = topMatch ? topMatch.level : 'No Match';

  elements.plagTopScoreVal.textContent = `${topScore.toFixed(1)}%`;
  elements.plagTopDocName.textContent = topMatch ? topMatch.document : 'None';
  elements.plagDocsChecked.textContent = checkedCount.toLocaleString();
  elements.plagCorpusStatusText.textContent = `Evaluated against ${checkedCount} institutional reference document${checkedCount === 1 ? '' : 's'}. Results do not search the public internet.`;

  // Animate Gauge & Color
  const offset = GAUGE_CIRCUMFERENCE * (1 - (Math.min(topScore, 100) / 100));
  elements.plagGaugeFill.style.strokeDashoffset = offset;

  const levelLower = levelStr.toLowerCase();
  let simClass = 'sim-unknown';
  let gaugeColor = '#0f766e';

  if (levelLower.includes('very high') || topScore >= 80) {
    simClass = 'sim-vhigh';
    gaugeColor = '#dc2626';
  } else if (levelLower.includes('high') || topScore >= 60) {
    simClass = 'sim-high';
    gaugeColor = '#ea580c';
  } else if (levelLower.includes('moderate') || topScore >= 40) {
    simClass = 'sim-moderate';
    gaugeColor = '#d97706';
  } else if (levelLower.includes('low') || topScore >= 20) {
    simClass = 'sim-low';
    gaugeColor = '#059669';
  } else {
    simClass = 'sim-vlow';
    gaugeColor = '#0f766e';
  }

  elements.plagGaugeFill.style.stroke = gaugeColor;
  elements.plagSimBadge.className = `similarity-pill ${simClass}`;
  elements.plagSimBadge.textContent = levelStr;

  const topDetails = pRes.top_match_details || {};
  elements.plagMatchingPhrases.textContent = topDetails.matching_phrase_count !== undefined ? topDetails.matching_phrase_count.toLocaleString() : '0';
  elements.plagPhraseWords.textContent = topDetails.matching_phrase_words !== undefined ? topDetails.matching_phrase_words.toLocaleString() : '0';

  // 1. Progress Bars
  const wSim = Number(topDetails.word_similarity || 0);
  const sSim = Number(topDetails.sentence_similarity || 0);
  const pSim = Number(topDetails.phrase_similarity || 0);

  elements.scoreWordSim.textContent = `${wSim.toFixed(2)}%`;
  elements.barWordSim.style.width = `${Math.min(wSim, 100)}%`;

  elements.scoreSentenceSim.textContent = `${sSim.toFixed(2)}%`;
  elements.barSentenceSim.style.width = `${Math.min(sSim, 100)}%`;

  elements.scorePhraseSim.textContent = `${pSim.toFixed(2)}%`;
  elements.barPhraseSim.style.width = `${Math.min(pSim, 100)}%`;

  // 2. Rankings Table (Merge Sort)
  elements.rankingsCountBadge.textContent = `${rankings.length} Ranked Documents`;

  if (rankings.length === 0) {
    elements.rankingsTbody.innerHTML = '<tr><td colspan="4" style="text-align:center; padding:18px;">No candidate rankings available.</td></tr>';
  } else {
    rankings.forEach(r => {
      const tr = document.createElement('tr');
      const simVal = Number(r.similarity || 0).toFixed(2);
      const level = r.level || 'Unknown';
      let badgeClass = 'sim-low';
      if (simVal >= 80) badgeClass = 'sim-vhigh';
      else if (simVal >= 60) badgeClass = 'sim-high';
      else if (simVal >= 40) badgeClass = 'sim-moderate';

      tr.innerHTML = `
        <td class="rank-cell">#${r.rank}</td>
        <td class="doc-name-cell">${escapeHtml(r.document)}</td>
        <td class="sim-pct-cell">${simVal}%</td>
        <td><span class="similarity-pill ${badgeClass}" style="font-size:0.75rem; padding:2px 8px;">${escapeHtml(level)}</span></td>
      `;
      elements.rankingsTbody.appendChild(tr);
    });
  }

  // 3. Matched Phrases Grid
  const phrases = topDetails.phrases || [];
  elements.phrasesCountPill.textContent = `${phrases.length} Phrases (≥ 3 words)`;

  if (phrases.length === 0) {
    elements.phraseCardsGrid.innerHTML = '<div class="table-empty" style="padding:20px; text-align:center; color:var(--text-muted);">No continuous matching phrases (≥ 3 words) found between these documents.</div>';
  } else {
    phrases.forEach(ph => {
      const card = document.createElement('div');
      card.className = 'phrase-match-card';
      card.innerHTML = `
        <div class="phrase-card-header">
          <span class="phrase-algo-badge">${escapeHtml(ph.algorithm || 'KMP')}</span>
          <span class="phrase-len-badge">${ph.word_count || 0} consecutive words</span>
        </div>
        <div class="phrase-text-quote">"${escapeHtml(ph.phrase)}"</div>
        <div class="phrase-offsets-row">
          <span>Uploaded Token: <strong>#${ph.doc1_pos !== undefined ? ph.doc1_pos : 0}</strong></span>
          <span>•</span>
          <span>Reference Token: <strong>#${ph.doc2_pos !== undefined ? ph.doc2_pos : 0}</strong></span>
        </div>
      `;
      elements.phraseCardsGrid.appendChild(card);
    });
  }
}

// ============================================================================
// 10. Audit History & Local Storage
// ============================================================================

function loadHistory() {
  try {
    let raw = localStorage.getItem(HISTORY_STORAGE_KEY);
    if (!raw) {
      // Migrate from older key if present
      raw = localStorage.getItem('integritycheck_audit_history');
    }
    if (raw) {
      state.history = JSON.parse(raw);
    }
  } catch (e) {
    state.history = [];
  }
}

function saveToHistory(data) {
  if (!data) return;
  const doc = data.document || {};
  const ai = data.analysis?.ai_detection?.result?.overall_assessment || {};
  const plag = data.analysis?.plagiarism_detection?.result?.rankings?.[0] || {};

  const entry = {
    id: 'hist_' + Date.now(),
    timestamp: new Date().toLocaleString(),
    filename: data.filename || doc.filename || 'Document',
    format: data.format || doc.format || 'TXT',
    wordCount: doc.word_count || 0,
    aiScore: typeof ai.score === 'number' ? (ai.score * 100).toFixed(2) + '%' : 'Abstained',
    aiVerdict: ai.verdict || 'Unknown',
    plagOverlap: plag.similarity !== undefined ? Number(plag.similarity).toFixed(1) + '%' : 'N/A',
    plagLevel: plag.level || 'Unknown',
    fullData: data
  };

  state.history.unshift(entry);
  if (state.history.length > 25) state.history.pop();

  try {
    localStorage.setItem(HISTORY_STORAGE_KEY, JSON.stringify(state.history));
  } catch (e) {
    // Local storage quota or security error
  }
}

function renderHistoryTable() {
  elements.historyTbody.innerHTML = '';
  if (!state.history || state.history.length === 0) {
    elements.historyTbody.innerHTML = '<tr><td colspan="8" class="text-center py-4" style="color:var(--text-muted); text-align:center; padding:24px;">No document analyses recorded in this session yet.</td></tr>';
    return;
  }

  state.history.forEach((h, idx) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td style="font-weight:600;">${escapeHtml(h.filename)}</td>
      <td><span class="badge-format">${escapeHtml(h.format)}</span></td>
      <td>${h.wordCount.toLocaleString()}</td>
      <td><span class="verdict-pill ${getVerdictClass(h.aiVerdict)}" style="font-size:0.75rem; padding:2px 8px;">${escapeHtml(h.aiVerdict)}</span></td>
      <td style="font-weight:700;">${escapeHtml(formatAiScorePct(h.aiScore))}</td>
      <td style="font-weight:700;">${escapeHtml(h.plagOverlap)}</td>
      <td style="font-size:0.75rem; color:var(--text-muted);">${escapeHtml(h.timestamp)}</td>
      <td>
        <button class="btn btn-secondary btn-sm" onclick="loadHistoricalAnalysis(${idx})">View Analysis</button>
      </td>
    `;
    elements.historyTbody.appendChild(tr);
  });
}

function getVerdictClass(v) {
  const vl = (v || '').toLowerCase();
  if (vl.includes('human')) return 'verdict-human';
  if (vl.includes('uncertain')) return 'verdict-uncertain';
  if (vl.includes('ai')) return 'verdict-ai';
  return 'verdict-insufficient';
}

window.loadHistoricalAnalysis = function(idx) {
  const item = state.history[idx];
  if (!item || !item.fullData) return;

  state.currentAnalysis = item.fullData;
  renderAnalysisResults(item.fullData);

  // Switch to AI Detector tab
  const aiTab = document.getElementById('nav-btn-ai-detector');
  if (aiTab) aiTab.click();
  elements.resultsContainer.scrollIntoView({ behavior: 'smooth', block: 'start' });
};

function initHistoryHandlers() {
  if (elements.btnClearHistory) {
    elements.btnClearHistory.addEventListener('click', () => {
      state.history = [];
      try {
        localStorage.removeItem(HISTORY_STORAGE_KEY);
      } catch (e) {}
      renderHistoryTable();
      showAlert('Analysis history cleared.', false);
    });
  }
}

// ============================================================================
// 11. Report Exports & Modal
// ============================================================================

function initReportExports() {
  elements.btnExportPdf.addEventListener('click', () => window.print());
  elements.btnQuickPrint.addEventListener('click', () => window.print());

  elements.btnExportJson.addEventListener('click', handleExportJson);
  elements.btnExportHtml.addEventListener('click', handleExportHtml);
  elements.btnQuickExport.addEventListener('click', handleExportHtml);

  // Modal controls
  elements.modalCloseBtn.addEventListener('click', () => elements.textReportModal.classList.add('hidden'));
  elements.textReportModal.addEventListener('click', (e) => {
    if (e.target === elements.textReportModal) elements.textReportModal.classList.add('hidden');
  });

  elements.modalCopyBtn.addEventListener('click', () => {
    navigator.clipboard.writeText(elements.modalPlaintextContent.textContent);
    elements.modalCopyBtn.textContent = 'Copied!';
    setTimeout(() => { elements.modalCopyBtn.textContent = 'Copy to Clipboard'; }, 1800);
  });

  elements.modalDownloadBtn.addEventListener('click', () => {
    const text = elements.modalPlaintextContent.textContent;
    const blob = new Blob([text], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `Plagiarism_Detection_System_Report_${state.currentAnalysis?.filename || 'doc'}.txt`;
    a.click();
    URL.revokeObjectURL(url);
  });
}

function handleExportJson() {
  if (!state.currentAnalysis) return;
  const jsonStr = JSON.stringify(state.currentAnalysis, null, 2);
  const blob = new Blob([jsonStr], { type: 'application/json;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `Plagiarism_Detection_System_Data_${state.currentAnalysis.filename || 'document'}.json`;
  a.click();
  URL.revokeObjectURL(url);
}

function handleExportHtml() {
  if (!state.currentAnalysis) return;
  const data = state.currentAnalysis;
  const doc = data.document || {};
  const ai = data.analysis?.ai_detection || {};
  const plag = data.analysis?.plagiarism_detection || {};
  const aiRes = ai.result || {};
  const pRes = plag.result || {};
  const topMatch = pRes.top_match_details || {};

  const htmlContent = `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Plagiarism Detection System Report — ${escapeHtml(data.filename)}</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; line-height: 1.6; color: #0f172a; max-width: 900px; margin: 40px auto; padding: 0 20px; }
    h1, h2, h3 { color: #0f172a; }
    .header { border-bottom: 2px solid #e2e8f0; padding-bottom: 20px; margin-bottom: 30px; }
    .badge { display: inline-block; padding: 3px 8px; border-radius: 4px; font-size: 12px; font-weight: bold; background: #e2e8f0; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin-bottom: 30px; }
    .card { border: 1px solid #e2e8f0; border-radius: 8px; padding: 20px; background: #fafbfc; }
    .score { font-size: 28px; font-weight: bold; font-family: monospace; }
    table { width: 100%; border-collapse: collapse; margin: 20px 0; }
    th, td { border: 1px solid #e2e8f0; padding: 10px 14px; text-align: left; }
    th { background: #f1f5f9; font-size: 12px; text-transform: uppercase; }
    .disclaimer { background: #fffbeb; border-left: 4px solid #d97706; padding: 14px; font-size: 13px; color: #92400e; margin-top: 40px; }
  </style>
</head>
<body>
  <div class="header">
    <h1>Plagiarism Detection System Analysis Report</h1>
    <p>Analyzed Document: <strong>${escapeHtml(data.filename)}</strong> (${escapeHtml(data.format || 'TXT')}) &bull; ${doc.word_count || 0} Words &bull; Generated: ${new Date().toLocaleString()}</p>
  </div>

  <div class="grid">
    <div class="card">
      <h3>AI Heuristic Score (%)</h3>
      <p>Verdict: <strong>${escapeHtml(aiRes.overall_assessment?.verdict || 'Unknown')}</strong></p>
      <div class="score">${typeof aiRes.overall_assessment?.score === 'number' ? (aiRes.overall_assessment.score * 100).toFixed(2) + '%' : 'Abstained'}</div>
      <p style="font-size: 13px; color: #64748b;">Flagged Sentences: ${aiRes.flagged_sentences_count || 0} / ${aiRes.word_count || 0} words</p>
      <p style="font-size: 12px; color: #64748b; margin-top: 6px;">Percentage representation of the detector's heuristic score. This is not a calibrated probability of AI authorship.</p>
    </div>
    <div class="card">
      <h3>Corpus Plagiarism (C11 DSA Engine)</h3>
      <p>Highest Similarity: <strong>${pRes.rankings?.[0]?.level || 'No match'}</strong></p>
      <div class="score">${pRes.rankings?.[0]?.similarity ? Number(pRes.rankings[0].similarity).toFixed(1) + '%' : '0.0%'}</div>
      <p style="font-size: 13px; color: #64748b;">Top Reference: ${escapeHtml(pRes.rankings?.[0]?.document || 'None')} (${plag.reference_documents_checked || 0} checked)</p>
    </div>
  </div>

  <h2>Linguistic Similarity Breakdown</h2>
  <table>
    <tr><th>Metric</th><th>Score</th><th>Algorithm</th></tr>
    <tr><td>Word Similarity</td><td>${(topMatch.word_similarity || 0).toFixed(2)}%</td><td>Separate Chaining Hash Table Jaccard</td></tr>
    <tr><td>Sentence Similarity</td><td>${(topMatch.sentence_similarity || 0).toFixed(2)}%</td><td>Sentence Array Jaccard</td></tr>
    <tr><td>Phrase Coverage</td><td>${(topMatch.phrase_similarity || 0).toFixed(2)}%</td><td>KMP & Rabin-Karp String Matchers</td></tr>
  </table>

  <h2>Reference Corpus Rankings (Merge Sort)</h2>
  <table>
    <tr><th>Rank</th><th>Candidate Document</th><th>Similarity %</th><th>Classification</th></tr>
    ${(pRes.rankings || []).map(r => `<tr><td>#${r.rank}</td><td>${escapeHtml(r.document)}</td><td>${Number(r.similarity).toFixed(2)}%</td><td>${escapeHtml(r.level)}</td></tr>`).join('')}
  </table>

  <h2>Continuous Matching Phrases (≥ 3 words)</h2>
  ${(topMatch.phrases || []).map(p => `
    <div style="border-left: 3px solid #2563eb; padding: 6px 12px; margin-bottom: 10px; background: #f8fafc;">
      <em>"${escapeHtml(p.phrase)}"</em>
      <div style="font-size: 12px; color: #64748b; font-family: monospace;">Algorithm: ${escapeHtml(p.algorithm)} &bull; Words: ${p.word_count} &bull; Uploaded Pos: ${p.doc1_pos} &bull; Reference Pos: ${p.doc2_pos}</div>
    </div>
  `).join('')}

  <div class="disclaimer">
    <strong>Academic Policy Disclaimer:</strong> AI-content detection is based on heuristic indicators and does not establish authorship. Plagiarism findings depend strictly on the reference corpus available to the system. Results should be reviewed in context and should not be used as the sole basis for disciplinary decisions.
  </div>
</body>
</html>`;

  const blob = new Blob([htmlContent], { type: 'text/html;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `Plagiarism_Detection_System_Report_${data.filename || 'document'}.html`;
  a.click();
  URL.revokeObjectURL(url);
}

// ============================================================================
// 11B. Direct File Comparison (Plagiarism Checker — Compare Two Files Mode)
// ============================================================================

const cmpState = {
  refDoc: null,        // { filepath, filename, file_size, word_count, sentence_count, unique_words, format }
  candidates: [],      // Array of uploaded candidate docs
  lastResult: null,    // Latest /api/compare JSON result
  lastBatch: null,     // Latest /api/batch-compare JSON result
  isRunning: false
};

const cmpEls = {};

function initFileComparison() {
  // Mode switcher inside Plagiarism Checker tab
  const modeButtons = document.querySelectorAll('.plag-mode-btn');
  const modePanels = document.querySelectorAll('.plag-mode-panel');
  modeButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      const mode = btn.dataset.plagMode;
      modeButtons.forEach(b => {
        b.classList.remove('active');
        b.setAttribute('aria-selected', 'false');
      });
      modePanels.forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      btn.setAttribute('aria-selected', 'true');
      const target = document.getElementById(`plag-mode-${mode}`);
      if (target) target.classList.add('active');

      if (mode === 'corpus') {
        updatePlagiarismTabState();
      }
    });
  });

  // Cache comparison DOM elements
  cmpEls.refDropzone = document.getElementById('cmp-ref-dropzone');
  cmpEls.refDropContent = document.getElementById('cmp-ref-drop-content');
  cmpEls.refFileInput = document.getElementById('cmp-ref-file-input');
  cmpEls.refMeta = document.getElementById('cmp-ref-meta');
  cmpEls.refFormat = document.getElementById('cmp-ref-format');
  cmpEls.refFilename = document.getElementById('cmp-ref-filename');
  cmpEls.refRemoveBtn = document.getElementById('cmp-ref-remove-btn');
  cmpEls.refSize = document.getElementById('cmp-ref-size');
  cmpEls.refWords = document.getElementById('cmp-ref-words');
  cmpEls.refSentences = document.getElementById('cmp-ref-sentences');
  cmpEls.refUnique = document.getElementById('cmp-ref-unique');

  cmpEls.candDropzone = document.getElementById('cmp-cand-dropzone');
  cmpEls.candDropContent = document.getElementById('cmp-cand-drop-content');
  cmpEls.candFileInput = document.getElementById('cmp-cand-file-input');
  cmpEls.candAddInput = document.getElementById('cmp-cand-add-input');
  cmpEls.candListWrapper = document.getElementById('cmp-cand-list-wrapper');
  cmpEls.candCount = document.getElementById('cmp-cand-count');
  cmpEls.candClearAll = document.getElementById('cmp-cand-clear-all');
  cmpEls.candFileList = document.getElementById('cmp-cand-file-list');

  cmpEls.btnRun = document.getElementById('btn-cmp-run');
  cmpEls.btnReset = document.getElementById('btn-cmp-reset');
  cmpEls.spinner = document.getElementById('cmp-spinner');
  cmpEls.btnText = document.getElementById('cmp-btn-text');

  cmpEls.emptyState = document.getElementById('cmp-empty-state');
  cmpEls.resultsContainer = document.getElementById('cmp-results-container');

  cmpEls.resTitle = document.getElementById('cmp-res-title');
  cmpEls.levelBadge = document.getElementById('cmp-level-badge');
  cmpEls.btnViewReport = document.getElementById('btn-cmp-view-report');
  cmpEls.btnExportReport = document.getElementById('btn-cmp-export-report');
  cmpEls.gaugeFill = document.getElementById('cmp-gauge-fill');
  cmpEls.scoreText = document.getElementById('cmp-score-text');
  cmpEls.identicalBanner = document.getElementById('cmp-identical-banner');

  cmpEls.statRefWords = document.getElementById('cmp-stat-ref-words');
  cmpEls.statCompWords = document.getElementById('cmp-stat-comp-words');
  cmpEls.statCommonWords = document.getElementById('cmp-stat-common-words');
  cmpEls.statMatchingSentences = document.getElementById('cmp-stat-matching-sentences');
  cmpEls.statMatchingPhrases = document.getElementById('cmp-stat-matching-phrases');
  cmpEls.statPhraseWords = document.getElementById('cmp-stat-phrase-words');

  cmpEls.wordScore = document.getElementById('cmp-word-score');
  cmpEls.wordBar = document.getElementById('cmp-word-bar');
  cmpEls.sentenceScore = document.getElementById('cmp-sentence-score');
  cmpEls.sentenceBar = document.getElementById('cmp-sentence-bar');
  cmpEls.phraseScore = document.getElementById('cmp-phrase-score');
  cmpEls.phraseBar = document.getElementById('cmp-phrase-bar');

  cmpEls.batchSection = document.getElementById('cmp-batch-ranking-section');
  cmpEls.batchCountBadge = document.getElementById('cmp-batch-count-badge');
  cmpEls.batchTbody = document.getElementById('cmp-batch-ranking-tbody');

  cmpEls.paneRefName = document.getElementById('cmp-pane-ref-name');
  cmpEls.paneCompName = document.getElementById('cmp-pane-comp-name');
  cmpEls.textRefView = document.getElementById('cmp-text-ref-view');
  cmpEls.textCompView = document.getElementById('cmp-text-comp-view');

  cmpEls.phrasesCountBadge = document.getElementById('cmp-phrases-count-badge');
  cmpEls.phrasesGrid = document.getElementById('cmp-phrases-grid');

  if (!cmpEls.refDropzone || !cmpEls.candDropzone) return;

  // Drag-and-drop wiring
  [cmpEls.refDropzone, cmpEls.candDropzone].forEach(zone => {
    ['dragenter', 'dragover'].forEach(evt => {
      zone.addEventListener(evt, (e) => {
        e.preventDefault();
        e.stopPropagation();
        zone.classList.add('dragover');
      });
    });
    ['dragleave', 'drop'].forEach(evt => {
      zone.addEventListener(evt, (e) => {
        e.preventDefault();
        e.stopPropagation();
        zone.classList.remove('dragover');
      });
    });
  });

  cmpEls.refDropzone.addEventListener('drop', (e) => {
    if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      handleCmpFileSelection(e.dataTransfer.files[0], true);
    }
  });

  cmpEls.candDropzone.addEventListener('drop', (e) => {
    if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      Array.from(e.dataTransfer.files).forEach(f => handleCmpFileSelection(f, false));
    }
  });

  // File input wiring
  cmpEls.refFileInput.addEventListener('change', (e) => {
    if (e.target.files && e.target.files.length > 0) {
      handleCmpFileSelection(e.target.files[0], true);
    }
  });

  const onCandSelect = (e) => {
    if (e.target.files && e.target.files.length > 0) {
      Array.from(e.target.files).forEach(f => handleCmpFileSelection(f, false));
      e.target.value = '';
    }
  };
  cmpEls.candFileInput.addEventListener('change', onCandSelect);
  if (cmpEls.candAddInput) cmpEls.candAddInput.addEventListener('change', onCandSelect);

  cmpEls.refRemoveBtn.addEventListener('click', () => {
    cmpState.refDoc = null;
    cmpEls.refFileInput.value = '';
    cmpEls.refMeta.classList.add('hidden');
    cmpEls.refDropContent.classList.remove('hidden');
    updateCmpRunButton();
  });

  cmpEls.candClearAll.addEventListener('click', () => {
    cmpState.candidates = [];
    cmpEls.candFileInput.value = '';
    renderCmpCandidateList();
    updateCmpRunButton();
  });

  cmpEls.btnRun.addEventListener('click', handleCmpCompare);
  cmpEls.btnReset.addEventListener('click', handleCmpReset);
  cmpEls.btnViewReport.addEventListener('click', showCmpReportModal);
  cmpEls.btnExportReport.addEventListener('click', handleCmpExportReport);
}

function handleCmpFileSelection(file, isReference) {
  hideAlert();
  if (!file) return;

  const ext = '.' + file.name.split('.').pop().toLowerCase();
  if (!SUPPORTED_EXTENSIONS.includes(ext)) {
    showAlert(`Unsupported file format "${ext}". Supported formats: TXT, PDF, DOCX, PPTX.`);
    return;
  }
  if (file.size === 0) {
    showAlert(`Document "${file.name}" is empty (0 bytes).`);
    return;
  }
  if (file.size > MAX_UPLOAD_SIZE) {
    showAlert(`File "${file.name}" exceeds the 50 MB size limit.`);
    return;
  }

  const reader = new FileReader();
  if (ext === '.txt') {
    reader.onload = async (e) => {
      const content = e.target.result;
      if (!content || !content.trim()) {
        showAlert(`Document "${file.name}" contains no extractable text.`);
        return;
      }
      await uploadCmpFile(file.name, content, false, isReference);
    };
    reader.readAsText(file, 'utf-8');
  } else {
    reader.onload = async (e) => {
      const dataUrl = e.target.result;
      if (!dataUrl) {
        showAlert(`Failed to read document "${file.name}".`);
        return;
      }
      await uploadCmpFile(file.name, dataUrl, true, isReference);
    };
    reader.readAsDataURL(file);
  }
}

async function uploadCmpFile(filename, content, isBase64, isReference) {
  try {
    const res = await fetch(`${API_BASE}/api/upload`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ filename, content, is_base64: isBase64 })
    });
    const data = await res.json();
    if (!res.ok || (data.success === false && data.error)) {
      showAlert(`Upload error for "${filename}": ${data.error || `HTTP ${res.status}`}`);
      return;
    }

    if (isReference) {
      cmpState.refDoc = data;
      cmpEls.refFormat.textContent = (data.format || 'TXT').toUpperCase();
      cmpEls.refFilename.textContent = data.filename;
      cmpEls.refSize.textContent = formatBytes(data.file_size || 0);
      cmpEls.refWords.textContent = (data.word_count || 0).toLocaleString();
      cmpEls.refSentences.textContent = (data.sentence_count || 0).toLocaleString();
      cmpEls.refUnique.textContent = (data.unique_words || 0).toLocaleString();
      cmpEls.refDropContent.classList.add('hidden');
      cmpEls.refMeta.classList.remove('hidden');
    } else {
      const exists = cmpState.candidates.some(c => c.filepath === data.filepath);
      if (!exists) {
        cmpState.candidates.push(data);
      }
      renderCmpCandidateList();
    }

    updateCmpRunButton();
  } catch (err) {
    showAlert(`Failed to upload "${filename}": ${err.message}`);
  }
}

function renderCmpCandidateList() {
  cmpEls.candFileList.innerHTML = '';
  cmpEls.candCount.textContent = cmpState.candidates.length;

  if (cmpState.candidates.length === 0) {
    cmpEls.candListWrapper.classList.add('hidden');
    cmpEls.candDropContent.classList.remove('hidden');
    return;
  }

  cmpEls.candDropContent.classList.add('hidden');
  cmpEls.candListWrapper.classList.remove('hidden');

  cmpState.candidates.forEach((cand, idx) => {
    const li = document.createElement('li');
    li.className = 'cmp-cand-item';
    li.innerHTML = `
      <div class="cmp-cand-item-info">
        <span class="badge-format">${escapeHtml((cand.format || 'TXT').toUpperCase())}</span>
        <span class="cmp-cand-item-name" title="${escapeHtml(cand.filename)}">${escapeHtml(cand.filename)}</span>
        <span class="cmp-cand-item-stats">(${(cand.word_count || 0).toLocaleString()} w, ${(cand.sentence_count || 0).toLocaleString()} s)</span>
      </div>
      <button type="button" class="btn-icon remove-file-btn" data-idx="${idx}" title="Remove file" aria-label="Remove candidate file">&times;</button>
    `;
    li.querySelector('.remove-file-btn').addEventListener('click', () => {
      cmpState.candidates.splice(idx, 1);
      renderCmpCandidateList();
      updateCmpRunButton();
    });
    cmpEls.candFileList.appendChild(li);
  });
}

function updateCmpRunButton() {
  cmpEls.btnRun.disabled = !(cmpState.refDoc && cmpState.candidates.length > 0 && !cmpState.isRunning);
}

function handleCmpReset() {
  cmpState.refDoc = null;
  cmpState.candidates = [];
  cmpState.lastResult = null;
  cmpState.lastBatch = null;

  cmpEls.refFileInput.value = '';
  cmpEls.candFileInput.value = '';
  cmpEls.refMeta.classList.add('hidden');
  cmpEls.refDropContent.classList.remove('hidden');
  cmpEls.candListWrapper.classList.add('hidden');
  cmpEls.candDropContent.classList.remove('hidden');
  cmpEls.candFileList.innerHTML = '';
  cmpEls.candCount.textContent = '0';

  cmpEls.resultsContainer.classList.add('hidden');
  cmpEls.batchSection.classList.add('hidden');
  cmpEls.emptyState.classList.remove('hidden');
  updateCmpRunButton();
  hideAlert();
}

async function handleCmpCompare() {
  if (!cmpState.refDoc || cmpState.candidates.length === 0 || cmpState.isRunning) return;

  hideAlert();
  cmpState.isRunning = true;
  cmpEls.btnRun.disabled = true;
  cmpEls.spinner.classList.remove('hidden');
  cmpEls.btnText.textContent = 'Comparing...';

  try {
    if (cmpState.candidates.length === 1) {
      cmpEls.batchSection.classList.add('hidden');
      await executeCmpPairwise(cmpState.refDoc.filepath, cmpState.candidates[0].filepath);
    } else {
      await executeCmpBatch(cmpState.refDoc.filepath, cmpState.candidates.map(c => c.filepath));
    }
  } catch (err) {
    showAlert(`Comparison failed: ${err.message}`);
  } finally {
    cmpState.isRunning = false;
    cmpEls.spinner.classList.add('hidden');
    cmpEls.btnText.textContent = 'Compare Documents';
    updateCmpRunButton();
  }
}

async function executeCmpPairwise(refPath, candPath) {
  const candObj = cmpState.candidates.find(c => c.filepath === candPath);
  const res = await fetch(`${API_BASE}/api/compare`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      ref_file: refPath,
      cand_file: candPath,
      ref_name: cmpState.refDoc ? cmpState.refDoc.filename : null,
      cand_name: candObj ? candObj.filename : null
    })
  });

  const data = await res.json();
  if (!res.ok || (!data.success && data.error)) {
    throw new Error(data.error || `HTTP ${res.status}`);
  }

  cmpState.lastResult = data;
  renderCmpResults(data);
}

async function executeCmpBatch(refPath, candPaths) {
  const candNamesMap = {};
  cmpState.candidates.forEach(c => {
    candNamesMap[c.filepath] = c.filename;
  });

  const res = await fetch(`${API_BASE}/api/batch-compare`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      ref_file: refPath,
      cand_files: candPaths,
      ref_name: cmpState.refDoc ? cmpState.refDoc.filename : null,
      cand_names: candNamesMap
    })
  });

  const data = await res.json();
  if (!res.ok || (!data.success && data.error)) {
    throw new Error(data.error || `HTTP ${res.status}`);
  }

  cmpState.lastBatch = data;
  renderCmpBatchRankings(data);

  if (data.rankings && data.rankings.length > 0) {
    await executeCmpPairwise(refPath, data.rankings[0].filepath);
    showAlert(`Compared against ${data.total_compared} files via Merge Sort. Displaying Rank #1: ${data.rankings[0].document}.`, false);
  }
}

function getCmpSeverityMeta(score) {
  if (score >= 80) return { cls: 'sim-vhigh', color: '#dc2626' };
  if (score >= 60) return { cls: 'sim-high', color: '#ea580c' };
  if (score >= 40) return { cls: 'sim-moderate', color: '#d97706' };
  if (score >= 20) return { cls: 'sim-low', color: '#059669' };
  return { cls: 'sim-vlow', color: '#0f766e' };
}

function renderCmpResults(res) {
  cmpEls.emptyState.classList.add('hidden');
  cmpEls.resultsContainer.classList.remove('hidden');

  const score = Number(res.similarity_score || 0);
  const sev = getCmpSeverityMeta(score);
  const GAUGE_CIRCUMFERENCE = 326.73; // 2 * PI * 52

  cmpEls.resTitle.textContent = `${res.reference_name || 'Document 1'} vs ${res.compared_name || 'Document 2'}`;
  cmpEls.scoreText.textContent = `${score.toFixed(2)}%`;
  cmpEls.levelBadge.className = `similarity-pill ${sev.cls}`;
  cmpEls.levelBadge.textContent = res.similarity_level || 'Completed';

  cmpEls.gaugeFill.style.stroke = sev.color;
  cmpEls.gaugeFill.style.strokeDashoffset = GAUGE_CIRCUMFERENCE * (1 - Math.min(score, 100) / 100);

  if (res.is_identical) {
    cmpEls.identicalBanner.classList.remove('hidden');
  } else {
    cmpEls.identicalBanner.classList.add('hidden');
  }

  // Document statistics
  cmpEls.statRefWords.textContent = (res.reference_words ?? 0).toLocaleString();
  cmpEls.statCompWords.textContent = (res.compared_words ?? 0).toLocaleString();
  cmpEls.statCommonWords.textContent = (res.common_words ?? 0).toLocaleString();
  cmpEls.statMatchingSentences.textContent = (res.matching_sentences ?? 0).toLocaleString();
  cmpEls.statMatchingPhrases.textContent = (res.matching_phrase_count ?? 0).toLocaleString();
  cmpEls.statPhraseWords.textContent = (res.matching_phrase_words ?? 0).toLocaleString();

  // 3 Breakdown bars
  const wSim = Number(res.word_similarity || 0);
  const sSim = Number(res.sentence_similarity || 0);
  const pSim = Number(res.phrase_similarity || 0);

  cmpEls.wordScore.textContent = `${wSim.toFixed(2)}%`;
  cmpEls.wordBar.style.width = `${Math.min(wSim, 100)}%`;
  cmpEls.sentenceScore.textContent = `${sSim.toFixed(2)}%`;
  cmpEls.sentenceBar.style.width = `${Math.min(sSim, 100)}%`;
  cmpEls.phraseScore.textContent = `${pSim.toFixed(2)}%`;
  cmpEls.phraseBar.style.width = `${Math.min(pSim, 100)}%`;

  // Side-by-side highlighted text
  cmpEls.paneRefName.textContent = res.reference_name || 'Document 1';
  cmpEls.paneCompName.textContent = res.compared_name || 'Document 2';
  cmpEls.textRefView.innerHTML = cmpHighlightMarkersToHtml(res.highlighted_ref_text || res.raw_ref_text);
  cmpEls.textCompView.innerHTML = cmpHighlightMarkersToHtml(res.highlighted_comp_text || res.raw_comp_text);

  // Matching phrases cards
  const phrases = res.phrases || [];
  cmpEls.phrasesGrid.innerHTML = '';
  cmpEls.phrasesCountBadge.textContent = `${phrases.length} Match${phrases.length === 1 ? '' : 'es'}`;

  if (phrases.length === 0) {
    cmpEls.phrasesGrid.innerHTML = '<div class="table-empty" style="padding:20px; text-align:center; color:var(--text-muted);">No continuous matching phrases (&ge; 3 words) were found between these two files.</div>';
  } else {
    phrases.forEach((ph, idx) => {
      const card = document.createElement('div');
      card.className = 'phrase-match-card';
      card.innerHTML = `
        <div class="phrase-card-header">
          <span class="phrase-algo-badge">${escapeHtml(ph.algorithm || 'KMP')}</span>
          <span class="phrase-len-badge">Match #${idx + 1} &bull; ${ph.word_count || 0} words</span>
        </div>
        <div class="phrase-text-quote">"${escapeHtml(ph.phrase)}"</div>
        <div class="phrase-offsets-row">
          <span>Doc 1 Token: <strong>#${ph.doc1_pos !== undefined ? ph.doc1_pos : 0}</strong></span>
          <span>&bull;</span>
          <span>Doc 2 Token: <strong>#${ph.doc2_pos !== undefined ? ph.doc2_pos : 0}</strong></span>
        </div>
      `;
      cmpEls.phrasesGrid.appendChild(card);
    });
  }

  cmpEls.resultsContainer.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

function cmpHighlightMarkersToHtml(rawText) {
  if (!rawText) return '<em style="color:var(--text-muted);">No content available</em>';
  let escaped = escapeHtml(rawText);
  escaped = escaped.replaceAll('&gt;&gt;&gt; MATCH &lt;&lt;&lt;', '<mark class="match-hl">');
  escaped = escaped.replaceAll('&gt;&gt;&gt; END MATCH &lt;&lt;&lt;', '</mark>');
  return escaped;
}

function renderCmpBatchRankings(data) {
  cmpEls.batchTbody.innerHTML = '';
  const rankings = data.rankings || [];
  if (rankings.length === 0) {
    cmpEls.batchSection.classList.add('hidden');
    return;
  }

  cmpEls.batchSection.classList.remove('hidden');
  cmpEls.batchCountBadge.textContent = `${rankings.length} Ranked`;

  rankings.forEach(r => {
    const tr = document.createElement('tr');
    const simVal = Number(r.similarity || 0);
    const sev = getCmpSeverityMeta(simVal);
    tr.innerHTML = `
      <td class="rank-cell">#${r.rank}</td>
      <td class="doc-name-cell">${escapeHtml(r.document)}</td>
      <td class="sim-pct-cell">${simVal.toFixed(2)}%</td>
      <td><span class="similarity-pill ${sev.cls}" style="font-size:0.75rem; padding:2px 8px;">${escapeHtml(r.level || 'Unknown')}</span></td>
      <td><button type="button" class="btn btn-secondary btn-sm cmp-inspect-btn">Inspect</button></td>
    `;
    tr.querySelector('.cmp-inspect-btn').addEventListener('click', async () => {
      try {
        await executeCmpPairwise(cmpState.refDoc.filepath, r.filepath);
      } catch (err) {
        showAlert(`Failed to inspect "${r.document}": ${err.message}`);
      }
    });
    cmpEls.batchTbody.appendChild(tr);
  });
}

function showCmpReportModal() {
  if (!cmpState.lastResult) {
    showAlert('Run a file comparison first to view the detailed report.');
    return;
  }
  const r = cmpState.lastResult;
  const repText = `============================================================
PLAGIARISM DETECTION REPORT
============================================================

Reference Document:
${r.reference_name}

Compared Document:
${r.compared_name}

---

## OVERALL RESULT

Similarity Score:  ${Number(r.similarity_score || 0).toFixed(2)}%
Similarity Level:  ${r.similarity_level}
${r.is_identical ? 'Identical Content: YES\n' : ''}
---

## ANALYSIS BREAKDOWN

Word Similarity:       ${Number(r.word_similarity || 0).toFixed(2)}%
Sentence Similarity:   ${Number(r.sentence_similarity || 0).toFixed(2)}%
Phrase Similarity:     ${Number(r.phrase_similarity || 0).toFixed(2)}%

---

## DOCUMENT STATISTICS

Reference Words:       ${r.reference_words}
Compared Words:        ${r.compared_words}
Reference Sentences:   ${r.reference_sentences}
Compared Sentences:    ${r.compared_sentences}
Common Unique Words:   ${r.common_words}
Matching Sentences:    ${r.matching_sentences}
Matching Phrases:      ${r.matching_phrase_count}
Matching Phrase Words: ${r.matching_phrase_words}

---

## DETECTED MATCHING PHRASES

${(r.phrases && r.phrases.length > 0)
    ? r.phrases.map((p, i) => `${i + 1}. [${p.algorithm}] Length: ${p.word_count} words\n   Ref Pos: ${p.doc1_pos} | Comp Pos: ${p.doc2_pos}\n   Phrase: "${p.phrase}"\n`).join('\n')
    : 'None detected.'}

============================================================
Report Generated by C11 DSA Engine
`;

  elements.modalPlaintextContent.textContent = repText;
  elements.textReportModal.classList.remove('hidden');
}

async function handleCmpExportReport() {
  if (!cmpState.lastResult) {
    showAlert('Run a file comparison first to export a report.');
    return;
  }

  const r = cmpState.lastResult;
  const baseName = (r.compared_name || 'comparison').replace(/\.[^/.]+$/, '');
  const outName = `${baseName}_report.txt`;

  try {
    const res = await fetch(`${API_BASE}/api/export-report`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        ref_file: r.reference_file,
        cand_file: r.compared_file,
        output_filename: outName,
        ref_name: r.reference_name,
        cand_name: r.compared_name
      })
    });
    const data = await res.json();
    if (data.success) {
      showAlert(`Report exported successfully to "${data.exported_path}".`, false);
    } else {
      showAlert(`Report export failed: ${data.error || 'Unknown error'}`);
    }
  } catch (err) {
    showAlert(`Report export error: ${err.message}`);
  }
}

// ============================================================================
// 12. Initialization Entrypoint
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  initDOMElements();
  initNavigation();
  initUploadHandlers();
  initAnalysisHandler();
  initSentenceFilters();
  initHistoryHandlers();
  initReportExports();
  initFileComparison();
  loadHistory();

  elements.alertClose.addEventListener('click', hideAlert);

  // Check backend server on load
  checkBackendHealth();
});
