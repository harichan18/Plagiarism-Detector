/**
 * app.js - Frontend logic for Plagiarism Detection & Document Similarity Analysis.
 * Bridges user interactions directly to the authoritative C DSA backend via server.py.
 */

// Backend API Configuration: Connects deployed Vercel frontend to Render backend, while keeping local development working
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

// Global State
let currentRefDoc = null;         // { filepath, filename, file_size, word_count, sentence_count, unique_words }
let candidateFiles = [];          // Array of { filepath, filename, file_size, word_count, sentence_count }
let lastComparisonResult = null;  // Latest comparison result from C backend
let lastBatchResult = null;       // Latest batch Merge Sort result

// DOM Elements
const alertContainer = document.getElementById('alert-container');
const alertMessage = document.getElementById('alert-message');
const alertClose = document.getElementById('alert-close');

const refDropzone = document.getElementById('ref-dropzone');
const refFileInput = document.getElementById('ref-file-input');
const refMeta = document.getElementById('ref-meta');
const refRemoveBtn = document.getElementById('ref-remove-btn');

const candDropzone = document.getElementById('cand-dropzone');
const candFileInput = document.getElementById('cand-file-input');
const candListWrapper = document.getElementById('cand-list-wrapper');
const candFileList = document.getElementById('cand-file-list');
const candCountSpan = document.getElementById('cand-count');
const candClearAllBtn = document.getElementById('cand-clear-all');

const btnCompare = document.getElementById('btn-compare');
const compareSpinner = document.getElementById('compare-spinner');
const btnReset = document.getElementById('btn-reset');

const emptyState = document.getElementById('empty-state');
const resultsContainer = document.getElementById('results-container');
const multiDocRankingSection = document.getElementById('multi-doc-ranking-section');

// Report Modal Elements
const reportModal = document.getElementById('report-modal');
const modalCloseBtn = document.getElementById('modal-close-btn');
const modalReportText = document.getElementById('modal-report-text');
const modalCopyBtn = document.getElementById('modal-copy-btn');
const modalExportBtn = document.getElementById('modal-export-btn');
const btnViewReport = document.getElementById('btn-view-report');
const btnExportReport = document.getElementById('btn-export-report');

// Initialization
document.addEventListener('DOMContentLoaded', () => {
  initTabs();
  initDragAndDrop();
  initFileInputs();
  initReportModal();

  btnCompare.addEventListener('click', handleCompare);
  btnReset.addEventListener('click', handleReset);
  alertClose.addEventListener('click', hideAlert);
});

// Tab Navigation
function initTabs() {
  const tabs = document.querySelectorAll('.nav-link');
  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      document.querySelectorAll('.tab-pane').forEach(p => p.classList.remove('active'));

      tab.classList.add('active');
      const target = document.getElementById(tab.dataset.tab);
      if (target) target.classList.add('active');
    });
  });
}

function switchTab(tabId) {
  const tabBtn = document.querySelector(`.nav-link[data-tab="${tabId}"]`);
  if (tabBtn) tabBtn.click();
}

// Alert notifications
function showAlert(msg, isError = true) {
  alertMessage.textContent = msg;
  const box = document.getElementById('alert-box');
  if (isError) {
    box.style.backgroundColor = '#fef2f2';
    box.style.borderColor = '#fecaca';
    box.style.color = '#991b1b';
  } else {
    box.style.backgroundColor = '#f0fdf4';
    box.style.borderColor = '#bbf7d0';
    box.style.color = '#166534';
  }
  alertContainer.classList.remove('hidden');
  window.scrollTo({ top: 0, behavior: 'smooth' });
}

function hideAlert() {
  alertContainer.classList.add('hidden');
}

// Drag and Drop
function initDragAndDrop() {
  [refDropzone, candDropzone].forEach(zone => {
    ['dragenter', 'dragover'].forEach(eventName => {
      zone.addEventListener(eventName, (e) => {
        e.preventDefault();
        zone.classList.add('drag-over');
      }, false);
    });

    ['dragleave', 'drop'].forEach(eventName => {
      zone.addEventListener(eventName, (e) => {
        e.preventDefault();
        zone.classList.remove('drag-over');
      }, false);
    });
  });

  refDropzone.addEventListener('drop', (e) => {
    const files = e.dataTransfer.files;
    if (files.length > 0) {
      handleSingleFileInput(files[0], true);
    }
  });

  candDropzone.addEventListener('drop', (e) => {
    const files = e.dataTransfer.files;
    if (files.length > 0) {
      for (let i = 0; i < files.length; i++) {
        handleSingleFileInput(files[i], false);
      }
    }
  });
}

function initFileInputs() {
  refFileInput.addEventListener('change', (e) => {
    if (e.target.files.length > 0) {
      handleSingleFileInput(e.target.files[0], true);
    }
  });

  candFileInput.addEventListener('change', (e) => {
    if (e.target.files.length > 0) {
      for (let i = 0; i < e.target.files.length; i++) {
        handleSingleFileInput(e.target.files[i], false);
      }
    }
  });

  refRemoveBtn.addEventListener('click', () => {
    currentRefDoc = null;
    refFileInput.value = '';
    refMeta.classList.add('hidden');
    refDropzone.querySelector('.drop-zone-content').classList.remove('hidden');
    updateCompareButton();
  });

  candClearAllBtn.addEventListener('click', () => {
    candidateFiles = [];
    candFileInput.value = '';
    renderCandidateList();
    updateCompareButton();
  });
}

const SUPPORTED_EXTENSIONS = ['.txt', '.pdf', '.docx', '.pptx'];

function handleSingleFileInput(file, isReference) {
  const ext = '.' + file.name.split('.').pop().toLowerCase();
  if (!SUPPORTED_EXTENSIONS.includes(ext)) {
    showAlert('Unsupported file format. Supported formats: TXT, PDF, DOCX, PPTX.');
    return;
  }

  const reader = new FileReader();
  if (ext === '.txt') {
    reader.onload = async (e) => {
      const content = e.target.result;
      if (!content || content.trim().length === 0) {
        showAlert(`Document "${file.name}" is empty.`);
        return;
      }

      if (isReference) {
        await uploadFileAsReference(file.name, content, false);
      } else {
        await uploadFileAsCandidate(file.name, content, false);
      }
    };
    reader.readAsText(file);
  } else {
    // Binary formats: PDF, DOCX, PPTX
    reader.onload = async (e) => {
      const dataUrl = e.target.result;
      if (!dataUrl) {
        showAlert(`Failed to read document "${file.name}".`);
        return;
      }

      if (isReference) {
        await uploadFileAsReference(file.name, dataUrl, true);
      } else {
        await uploadFileAsCandidate(file.name, dataUrl, true);
      }
    };
    reader.readAsDataURL(file);
  }
}

// Upload helpers
async function uploadFileAsReference(filename, content, isBase64 = false) {
  try {
    const res = await fetch(`${API_BASE}/api/upload`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ filename, content, is_base64: isBase64 })
    });
    const data = await res.json();
    if (!data.success && data.error) {
      showAlert(`Reference upload error: ${data.error}`);
      return;
    }

    currentRefDoc = data;
    document.getElementById('ref-filename').textContent = data.filename;

    // Display format badge
    let badge = document.getElementById('ref-format-badge');
    if (!badge) {
      badge = document.createElement('span');
      badge.id = 'ref-format-badge';
      const titleBox = document.querySelector('#ref-meta .doc-title-box');
      if (titleBox) titleBox.appendChild(badge);
    }
    const fmt = (data.format || 'TXT').toLowerCase();
    badge.className = `format-badge ${fmt}`;
    badge.textContent = (data.format || 'TXT').toUpperCase();

    document.getElementById('ref-size').textContent = `${(data.file_size / 1024).toFixed(2)} KB`;
    document.getElementById('ref-words').textContent = data.word_count.toLocaleString();
    document.getElementById('ref-sentences').textContent = data.sentence_count.toLocaleString();
    document.getElementById('ref-unique').textContent = data.unique_words.toLocaleString();

    refDropzone.querySelector('.drop-zone-content').classList.add('hidden');
    refMeta.classList.remove('hidden');
    hideAlert();
    updateCompareButton();
  } catch (err) {
    showAlert(`Server communication error: ${err.message}`);
  }
}

async function uploadFileAsCandidate(filename, content, isBase64 = false) {
  try {
    const res = await fetch(`${API_BASE}/api/upload`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ filename, content, is_base64: isBase64 })
    });
    const data = await res.json();
    if (!data.success && data.error) {
      showAlert(`Candidate upload error: ${data.error}`);
      return;
    }

    // Deduplicate in candidate list
    candidateFiles = candidateFiles.filter(c => c.filename !== data.filename);
    candidateFiles.push(data);
    renderCandidateList();
    hideAlert();
    updateCompareButton();
  } catch (err) {
    showAlert(`Server communication error: ${err.message}`);
  }
}

function renderCandidateList() {
  candFileList.innerHTML = '';
  candCountSpan.textContent = candidateFiles.length;

  if (candidateFiles.length > 0) {
    candDropzone.querySelector('.drop-zone-content').classList.add('hidden');
    candListWrapper.classList.remove('hidden');

    candidateFiles.forEach((cand, idx) => {
      const li = document.createElement('li');
      li.className = 'cand-file-item';
      const fmt = (cand.format || 'TXT').toLowerCase();
      li.innerHTML = `
        <span class="cand-file-name">
          ${escapeHtml(cand.filename)} 
          <span class="format-badge ${fmt}">${(cand.format || 'TXT').toUpperCase()}</span>
          <span style="color: var(--text-muted); font-size: 0.78rem; margin-left: 6px;">(${cand.word_count} w, ${cand.sentence_count} s)</span>
        </span>
        <button class="btn-icon" data-idx="${idx}" title="Remove file">&times;</button>
      `;
      li.querySelector('.btn-icon').addEventListener('click', (e) => {
        const delIdx = parseInt(e.currentTarget.dataset.idx, 10);
        candidateFiles.splice(delIdx, 1);
        renderCandidateList();
        updateCompareButton();
      });
      candFileList.appendChild(li);
    });
  } else {
    candListWrapper.classList.add('hidden');
    candDropzone.querySelector('.drop-zone-content').classList.remove('hidden');
  }
}

function updateCompareButton() {
  if (currentRefDoc && candidateFiles.length > 0) {
    btnCompare.disabled = false;
  } else {
    btnCompare.disabled = true;
  }
}

function handleReset() {
  currentRefDoc = null;
  candidateFiles = [];
  refFileInput.value = '';
  candFileInput.value = '';
  refMeta.classList.add('hidden');
  refDropzone.querySelector('.drop-zone-content').classList.remove('hidden');
  candListWrapper.classList.add('hidden');
  candDropzone.querySelector('.drop-zone-content').classList.remove('hidden');
  emptyState.classList.remove('hidden');
  resultsContainer.classList.add('hidden');
  if (multiDocRankingSection) multiDocRankingSection.classList.add('hidden');
  updateCompareButton();
  hideAlert();
}

// Comparison Execution
async function handleCompare() {
  if (!currentRefDoc || candidateFiles.length === 0) {
    showAlert('Please select both a reference document and at least one candidate document.');
    return;
  }

  btnCompare.disabled = true;
  compareSpinner.classList.remove('hidden');
  hideAlert();

  try {
    if (candidateFiles.length === 1) {
      if (multiDocRankingSection) multiDocRankingSection.classList.add('hidden');
      await runSingleComparison(currentRefDoc.filepath, candidateFiles[0].filepath);
    } else {
      await runBatchComparison(currentRefDoc.filepath, candidateFiles.map(c => c.filepath));
    }
  } catch (err) {
    showAlert(`Analysis execution failed: ${err.message}`);
  } finally {
    btnCompare.disabled = false;
    compareSpinner.classList.add('hidden');
  }
}

// Single Comparison Handler
async function runSingleComparison(refPath, candPath) {
  const candObj = candidateFiles.find(c => c.filepath === candPath);
  const candName = candObj ? candObj.filename : null;
  const refName = currentRefDoc ? currentRefDoc.filename : null;

  const res = await fetch(`${API_BASE}/api/compare`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      ref_file: refPath,
      cand_file: candPath,
      ref_name: refName,
      cand_name: candName
    })
  });

  const result = await res.json();
  if (!result.success && result.error) {
    showAlert(`C Engine Error: ${result.error}`);
    return;
  }

  lastComparisonResult = result;
  renderComparisonResults(result);
}

// Batch Comparison Handler
async function runBatchComparison(refPath, candPaths) {
  const candNamesMap = {};
  candidateFiles.forEach(c => {
    candNamesMap[c.filepath] = c.filename;
  });
  const refName = currentRefDoc ? currentRefDoc.filename : null;

  const res = await fetch(`${API_BASE}/api/batch-compare`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      ref_file: refPath,
      cand_files: candPaths,
      ref_name: refName,
      cand_names: candNamesMap
    })
  });

  const result = await res.json();
  if (!result.success && result.error) {
    showAlert(`C Batch Engine Error: ${result.error}`);
    return;
  }

  lastBatchResult = result;
  if (multiDocRankingSection) multiDocRankingSection.classList.remove('hidden');
  renderRankingTable(result);

  // Automatically inspect top-ranked candidate in single comparison view
  if (result.rankings && result.rankings.length > 0) {
    const topDoc = result.rankings[0];
    await runSingleComparison(refPath, topDoc.filepath);
    showAlert(`Batch analysis completed via Merge Sort! Ranked ${result.total_compared} documents. Viewing details for Rank #1: ${topDoc.document}.`, false);
  }
}

// Render Single Comparison Results
function renderComparisonResults(res) {
  emptyState.classList.add('hidden');
  resultsContainer.classList.remove('hidden');

  document.getElementById('res-comparison-title').textContent = `${res.reference_name} vs ${res.compared_name}`;

  // Gauge Score
  const score = res.similarity_score;
  document.getElementById('score-text').textContent = `${score.toFixed(2)}%`;

  // Color & Level
  const levelBadge = document.getElementById('level-badge');
  levelBadge.textContent = res.similarity_level;
  applyLevelBadgeColor(levelBadge, score);

  // SVG Circular Ring Animation (Perimeter = 2 * PI * 74 ~= 464.95)
  const ring = document.getElementById('progress-indicator');
  const circumference = 464.95;
  const offset = circumference - (score / 100) * circumference;
  ring.style.strokeDashoffset = offset;
  ring.style.stroke = getLevelColor(score);

  // Identical Banner
  const identBanner = document.getElementById('identical-banner');
  if (res.is_identical) {
    identBanner.classList.remove('hidden');
  } else {
    identBanner.classList.add('hidden');
  }

  // 3 Metric Cards
  document.getElementById('word-score').textContent = `${res.word_similarity.toFixed(2)}%`;
  document.getElementById('sentence-score').textContent = `${res.sentence_similarity.toFixed(2)}%`;
  document.getElementById('phrase-score').textContent = `${res.phrase_similarity.toFixed(2)}%`;

  document.getElementById('word-bar').style.width = `${res.word_similarity}%`;
  document.getElementById('sentence-bar').style.width = `${res.sentence_similarity}%`;
  document.getElementById('phrase-bar').style.width = `${res.phrase_similarity}%`;

  // Document Statistics
  document.getElementById('stat-ref-words').textContent = res.reference_words;
  document.getElementById('stat-comp-words').textContent = res.compared_words;
  document.getElementById('stat-common-words').textContent = res.common_words;
  document.getElementById('stat-matching-sentences').textContent = res.matching_sentences;
  document.getElementById('stat-matching-phrases').textContent = res.matching_phrase_count;
  document.getElementById('stat-phrase-words').textContent = res.matching_phrase_words;

  // Matching Phrases
  renderMatchingPhrases(res.phrases);

  // Side-by-Side Text
  renderSideBySideText(res);

  resultsContainer.scrollIntoView({ behavior: 'smooth' });
}

function renderMatchingPhrases(phrases) {
  const container = document.getElementById('phrases-list');
  const countBadge = document.getElementById('phrases-count-badge');
  container.innerHTML = '';

  if (!phrases || phrases.length === 0) {
    countBadge.textContent = '0 Matches';
    container.innerHTML = '<div class="table-empty">No matching phrases (≥ 3 words) were identified between these documents.</div>';
    return;
  }

  countBadge.textContent = `${phrases.length} Match${phrases.length > 1 ? 'es' : ''}`;

  phrases.forEach((m, idx) => {
    const card = document.createElement('div');
    card.className = 'phrase-card';
    card.innerHTML = `
      <div class="phrase-card-header">
        <span class="phrase-match-num">Match #${idx + 1}</span>
        <div class="phrase-meta-tags">
          <span>${m.word_count} words</span>
          <span class="phrase-algo-tag">${m.algorithm}</span>
        </div>
      </div>
      <div class="phrase-card-body">
        <div class="phrase-snippet">"${escapeHtml(m.phrase)}"</div>
        <div class="phrase-pos-row">
          <span>Reference Token Index: <strong>${m.doc1_pos}</strong></span>
          <span>Compared Token Index: <strong>${m.doc2_pos}</strong></span>
        </div>
      </div>
    `;
    container.appendChild(card);
  });
}

function renderSideBySideText(res) {
  document.getElementById('pane-ref-name').textContent = res.reference_name;
  document.getElementById('pane-comp-name').textContent = res.compared_name;

  const refView = document.getElementById('text-ref-view');
  const compView = document.getElementById('text-comp-view');

  refView.innerHTML = highlightMarkersToHtml(res.highlighted_ref_text || res.raw_ref_text);
  compView.innerHTML = highlightMarkersToHtml(res.highlighted_comp_text || res.raw_comp_text);
}

function highlightMarkersToHtml(rawText) {
  if (!rawText) return '<em>No content available</em>';
  let escaped = escapeHtml(rawText);
  escaped = escaped.replaceAll('&gt;&gt;&gt; MATCH &lt;&lt;&lt;', '<mark class="match-hl">');
  escaped = escaped.replaceAll('&gt;&gt;&gt; END MATCH &lt;&lt;&lt;', '</mark>');
  return escaped;
}

// Render Multi-Document Ranking Table
function renderRankingTable(data) {
  const tbody = document.getElementById('ranking-table-body');
  const statsGrid = document.getElementById('ranking-stats-grid');
  tbody.innerHTML = '';

  if (!data || !data.rankings || data.rankings.length === 0) {
    tbody.innerHTML = '<tr><td colspan="5" class="table-empty">No comparison results.</td></tr>';
    statsGrid.classList.add('hidden');
    return;
  }

  // Populate Statistics
  const st = data.statistics;
  document.getElementById('rstat-total').textContent = st.total;
  document.getElementById('rstat-vhigh').textContent = st.very_high_count;
  document.getElementById('rstat-high').textContent = st.high_count;
  document.getElementById('rstat-mod').textContent = st.moderate_count;
  document.getElementById('rstat-low').textContent = st.low_count;
  document.getElementById('rstat-vlow').textContent = st.very_low_count;
  document.getElementById('rstat-avg').textContent = `${st.average_similarity.toFixed(1)}%`;
  statsGrid.classList.remove('hidden');

  data.rankings.forEach(row => {
    const tr = document.createElement('tr');

    let rankClass = '';
    if (row.rank === 1) rankClass = 'rank-1';
    else if (row.rank === 2) rankClass = 'rank-2';
    else if (row.rank === 3) rankClass = 'rank-3';

    const levelTagClass = getLevelClass(row.similarity);

    tr.innerHTML = `
      <td><span class="rank-badge ${rankClass}">#${row.rank}</span></td>
      <td><strong>${escapeHtml(row.document)}</strong></td>
      <td><strong>${row.similarity.toFixed(2)}%</strong></td>
      <td><span class="level-tag ${levelTagClass}">${row.level}</span></td>
      <td style="text-align: right;">
        <button class="btn btn-outline btn-sm btn-inspect" data-path="${row.filepath}">Inspect</button>
      </td>
    `;

    tr.querySelector('.btn-inspect').addEventListener('click', async (e) => {
      const candPath = e.currentTarget.dataset.path;
      await runSingleComparison(data.reference_file, candPath);
    });

    tbody.appendChild(tr);
  });
}

// Modal and Report Export
function initReportModal() {
  btnViewReport.addEventListener('click', showReportModal);
  modalCloseBtn.addEventListener('click', () => reportModal.classList.add('hidden'));
  reportModal.querySelector('.modal-backdrop').addEventListener('click', () => reportModal.classList.add('hidden'));

  modalCopyBtn.addEventListener('click', () => {
    navigator.clipboard.writeText(modalReportText.textContent);
    modalCopyBtn.textContent = 'Copied!';
    setTimeout(() => { modalCopyBtn.textContent = 'Copy Text'; }, 2000);
  });

  btnExportReport.addEventListener('click', handleExportReport);
  modalExportBtn.addEventListener('click', handleExportReport);
}

function showReportModal() {
  if (!lastComparisonResult) {
    showAlert('No report available. Perform a comparison first.');
    return;
  }

  const res = lastComparisonResult;
  const repText = `============================================================
PLAGIARISM DETECTION REPORT
============================================================

Reference Document:
${res.reference_name}

Compared Document:
${res.compared_name}

---

## OVERALL RESULT

Similarity Score:  ${res.similarity_score.toFixed(2)}%
Similarity Level:  ${res.similarity_level}
${res.is_identical ? 'Identical Content: YES\n' : ''}
---

## ANALYSIS BREAKDOWN

Word Similarity:       ${res.word_similarity.toFixed(2)}%
Sentence Similarity:   ${res.sentence_similarity.toFixed(2)}%
Phrase Similarity:     ${res.phrase_similarity.toFixed(2)}%

---

## DOCUMENT STATISTICS

Reference Words:       ${res.reference_words}
Compared Words:        ${res.compared_words}

Reference Sentences:   ${res.reference_sentences}
Compared Sentences:    ${res.compared_sentences}

Common Unique Words:   ${res.common_words}
Matching Sentences:    ${res.matching_sentences}
Matching Phrases:      ${res.matching_phrase_count}
Matching Phrase Words: ${res.matching_phrase_words}

---

## DETECTED MATCHING PHRASES

${(res.phrases && res.phrases.length > 0)
  ? res.phrases.map((p, i) => `${i + 1}. [${p.algorithm}] Length: ${p.word_count} words\n   Ref Pos: ${p.doc1_pos} | Comp Pos: ${p.doc2_pos}\n   Phrase: "${p.phrase}"\n`).join('\n')
  : 'None detected.'}

============================================================
Report Generated by Pure C DSA Engine
`;

  modalReportText.textContent = repText;
  reportModal.classList.remove('hidden');
}

async function handleExportReport() {
  if (!lastComparisonResult) {
    showAlert('No comparison available to export.');
    return;
  }

  const res = lastComparisonResult;
  const outName = `${res.compared_name.replace(/\.[^/.]+$/, "")}_report.txt`;

  try {
    const apiRes = await fetch(`${API_BASE}/api/export-report`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        ref_file: res.reference_file,
        cand_file: res.compared_file,
        output_filename: outName,
        ref_name: res.reference_name,
        cand_name: res.compared_name
      })
    });

    const exportData = await apiRes.json();
    if (exportData.success) {
      showAlert(`Report successfully exported by C Engine to "${exportData.exported_path}".`, false);
      if (!reportModal.classList.contains('hidden')) {
        reportModal.classList.add('hidden');
      }
    } else {
      showAlert(`Export failed: ${exportData.error}`);
    }
  } catch (err) {
    showAlert(`Export network error: ${err.message}`);
  }
}

// Helper Utilities
function getLevelColor(score) {
  if (score >= 80) return 'var(--c-vhigh)';
  if (score >= 60) return 'var(--c-high)';
  if (score >= 40) return 'var(--c-moderate)';
  if (score >= 20) return 'var(--c-low)';
  return 'var(--c-vlow)';
}

function getLevelClass(score) {
  if (score >= 80) return 'vhigh';
  if (score >= 60) return 'high';
  if (score >= 40) return 'moderate';
  if (score >= 20) return 'low';
  return 'vlow';
}

function applyLevelBadgeColor(el, score) {
  el.style.backgroundColor = '';
  el.style.color = '';
  if (score >= 80) {
    el.style.backgroundColor = 'var(--c-vhigh-bg)';
    el.style.color = 'var(--c-vhigh)';
  } else if (score >= 60) {
    el.style.backgroundColor = 'var(--c-high-bg)';
    el.style.color = 'var(--c-high)';
  } else if (score >= 40) {
    el.style.backgroundColor = 'var(--c-moderate-bg)';
    el.style.color = 'var(--c-moderate)';
  } else if (score >= 20) {
    el.style.backgroundColor = 'var(--c-low-bg)';
    el.style.color = 'var(--c-low)';
  } else {
    el.style.backgroundColor = 'var(--c-vlow-bg)';
    el.style.color = 'var(--c-vlow)';
  }
}

function escapeHtml(str) {
  if (!str) return '';
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}
