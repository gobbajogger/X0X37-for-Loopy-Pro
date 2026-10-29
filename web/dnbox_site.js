// SPDX-License-Identifier: GPL-3.0-only
// DnBOX browser presentation. No firmware admission or MIDI transaction authority.
export const LABELS = Object.freeze({
  en: {
    install: 'Install', samples: 'Samples', projects: 'Projects', manual: 'Manual',
    changes: "What’s new", history: 'History', home: 'DnBOX home',
    prep: 'Prepare samples after choosing firmware',
    prepHelp: 'These WAV files replace built-in breaks in a selected firmware package. You can listen locally before installing. For separate uploads and backups, open Samples.',
    workspace: 'Open sample workspace',
    steps: 'Installation stages',
    scan: 'Connect keyboard', select: 'Select and check firmware',
    protect: 'Protect existing samples', write: 'Write and restart',
    current: 'Current', completed: 'Completed', pending: 'Pending',
    local: 'Local firmware file',
    browserOk: 'Web MIDI is available. Connect a USB data cable and allow MIDI with SysEx when prompted.',
    browserNo: 'Web MIDI with SysEx is unavailable here. Use a secure Chrome or Edge desktop browser. No firmware can be written from this page.',
    browserInsecure: 'This page needs HTTPS or localhost to request Web MIDI. Nothing will be written.',
    retry: 'Rescan', fileLoading: 'Reading and checking local firmware…',
    fileInvalid: 'The local firmware file failed checks; no write was started.',
    fileOk: 'Local package parsed. Confirm the physical model and save a backup before writing.',
    preview: 'Local sample preview (nothing sent to the keyboard)',
    previewLimited: 'Showing only the first eight local previews. All selected files remain selected.',
  },
  ru: {
    install: 'Установка', samples: 'Сэмплы', projects: 'Проекты', manual: 'Руководство',
    changes: 'Что нового', history: 'История', home: 'Главная DnBOX',
    prep: 'Подготовка сэмплов после выбора прошивки',
    prepHelp: 'Эти WAV заменяют встроенные брейки в выбранной прошивке. Их можно прослушать до установки. Для отдельного импорта и резервных копий откройте «Сэмплы».',
    workspace: 'Открыть редактор сэмплов',
    steps: 'Этапы установки',
    scan: 'Подключение клавиатуры', select: 'Выбор и проверка прошивки',
    protect: 'Защита существующих сэмплов', write: 'Запись и перезапуск',
    current: 'Текущий', completed: 'Завершён', pending: 'Ожидание',
    local: 'Локальный файл прошивки',
    browserOk: 'Web MIDI доступен. Подключите USB-кабель передачи данных и разрешите MIDI с SysEx по запросу.',
    browserNo: 'Web MIDI с SysEx недоступен. Откройте страницу по HTTPS в Chrome или Edge на компьютере. Запись прошивки невозможна.',
    browserInsecure: 'Для Web MIDI нужен HTTPS либо localhost. Ничего не будет записано.',
    retry: 'Повторный поиск', fileLoading: 'Чтение и проверка локальной прошивки…',
    fileInvalid: 'Локальная прошивка не прошла проверку. Запись не начиналась.',
    fileOk: 'Формат файла проверен. Перед записью проверьте модель корпуса и сохраните копию.',
    preview: 'Прослушивание локальных сэмплов (ничего не отправлено)',
    previewLimited: 'Показаны восемь первых превью. Все выбранные файлы сохранены.',
  },
  ja: {
    install: 'インストール', samples: 'サンプル', projects: 'プロジェクト', manual: 'マニュアル',
    changes: '更新内容', history: '履歴', home: 'DnBOX ホーム',
    prep: 'ファームウェア選択後にサンプルを準備',
    prepHelp: 'これらの WAV は選択したパッケージの内蔵ブレイクを置き換えます。書き込み前に試聴できます。別のアップロードやバックアップはサンプル画面を使用してください。',
    workspace: 'サンプル画面へ', steps: 'インストール段階',
    scan: 'キーボードに接続', select: 'パッケージを確認', protect: 'サンプルを保護', write: '書き込みと再起動',
    current: '現在', completed: '完了', pending: '待機中',
    local: 'ローカルファームウェア',
    browserOk: 'Web MIDI が使えます。USB データケーブルをつなぎ、SysEx MIDI の使用を許可してください。',
    browserNo: 'Web MIDI (SysEx) は使用できません。パソコンの Chrome または Edge を使ってください。書き込みは始まりません。',
    browserInsecure: 'Web MIDI の使用には HTTPS または localhost が必要です。書き込みは始まりません。',
    retry: '再検索', fileLoading: 'ローカルファイルを確認中…', fileInvalid: 'ファイルを確認できません。書き込みは始まりません。',
    fileOk: 'ファイル形式を確認しました。書き込み前に本体のモデルとバックアップを確認してください。',
    preview: 'ローカルサンプルを試聴（本体へは送信されません）',
    previewLimited: '最初の8件のみ試聴表示します。ファイルの選択は変わりません。',
  },
});

export function availableMidi(env) {
  if (env?.isSecureContext === false) return 'insecure';
  if (typeof env?.navigator?.requestMIDIAccess !== 'function') return 'unsupported';
  return 'supported';
}

export function visibleVersionHistory(catalog, present = () => false) {
  // Never mark an entry installable based on a label or source revision alone.
  const entries = Array.isArray(catalog?.versions) ? catalog.versions : [];
  return entries.map(entry => ({
    version: String(entry.version || ''), label: String(entry.label || ''),
    models: Object.entries(entry.models || {}).map(([model, item]) => ({
      model, filename: String(item.file || String(item.pkg || '').split('/').pop() || ''),
      digest: /^[a-f0-9]{64}$/i.test(item.sha256 || '') ? item.sha256.toLowerCase() : '',
      available: /^[a-f0-9]{64}$/i.test(item.sha256 || '') && present(item),
    })),
  }));
}

export function enhanceDnboxSite(doc, env) {
  const main = doc.querySelector('main');
  if (!main || doc.getElementById('dnbox-site-nav')) return null;
  const lang = () => LABELS[doc.documentElement.lang] ? doc.documentElement.lang : 'en';
  const copy = (key) => LABELS[lang()][key];
  const nav = doc.createElement('nav');
  nav.id = 'dnbox-site-nav'; nav.className = 'dnbox-site-nav';
  nav.setAttribute('aria-label', 'DnBOX');
  const destinations = [
    ['install', '../install/'], ['samples', '../breaks/'], ['projects', '../projects/'],
    ['manual', '../manual/'], ['changes', '../new/'], ['history', '../history/'],
  ];
  const home = doc.createElement('a');
  home.href = '../'; home.className = 'dnbox-brand'; home.textContent = 'DnBOX';
  home.setAttribute('aria-label', copy('home'));
  nav.append(home);
  for (const [key, url] of destinations) {
    const link = doc.createElement('a');
    link.href = url; link.dataset.dnboxText = key; link.textContent = copy(key);
    nav.append(link);
  }
  main.prepend(nav);
  const pick = doc.getElementById('pick');
  const stageLabels = ['scan', 'select', 'protect', 'write'];
  let stages = null, current = -1, installing = false;
  const status = doc.getElementById('status');
  const conn = doc.getElementById('conn');
  if (status) {
    status.setAttribute('role', 'status');
    status.setAttribute('aria-live', 'polite');
    status.setAttribute('aria-atomic', 'true');
  }
  if (conn) { conn.setAttribute('role', 'status'); conn.setAttribute('aria-live', 'polite'); }
  if (pick) {
    stages = doc.createElement('ol');
    stages.id = 'dnbox-stages'; stages.className = 'dnbox-stages';
    stages.setAttribute('aria-label', copy('steps'));
    for (const name of stageLabels) {
      const li = doc.createElement('li'); li.dataset.stage = name;
      li.textContent = copy(name); stages.append(li);
    }
    pick.prepend(stages);
    const browser = doc.createElement('p');
    browser.id = 'dnbox-browser'; browser.className = 'dnbox-browser';
    browser.setAttribute('role', 'status'); browser.setAttribute('aria-live', 'polite');
    const connect = doc.getElementById('connbtn');
    if (connect) connect.after(browser);
    else pick.prepend(browser);
    const support = availableMidi(env);
    browser.dataset.state = support;
    browser.textContent = copy(support === 'insecure' ? 'browserInsecure' : support === 'unsupported' ? 'browserNo' : 'browserOk');
    if (support !== 'supported' && connect) {
      connect.disabled = true;
      connect.setAttribute('aria-disabled', 'true');
      connect.setAttribute('aria-describedby', 'dnbox-browser');
    }
  }
  const stock = doc.getElementById('stock');
  const loops = doc.getElementById('loops');
  const sample = doc.createElement('section');
  sample.id = 'dnbox-samples'; sample.className = 'dnbox-workspace';
  const sampleHeading = doc.createElement('h2'); sampleHeading.dataset.dnboxText = 'prep';
  const sampleHelp = doc.createElement('p'); sampleHelp.dataset.dnboxText = 'prepHelp';
  sampleHelp.className = 'small';
  const samplesLink = doc.createElement('a');
  samplesLink.href = '../breaks/'; samplesLink.dataset.dnboxText = 'workspace';
  sample.append(sampleHeading, sampleHelp);
  if (loops) sample.append(loops);
  const preview = doc.createElement('div'); preview.id = 'dnbox-sample-preview';
  sample.append(preview, samplesLink);
  if (pick) {
    // Reorder only DOM presentation. Original installer handlers keep using the same elements.
    if (stock) pick.after(stock);
    (stock || pick).after(sample);
  } else sample.remove();
  const stockDrop = doc.getElementById('stockdrop');
  const localStatus = doc.createElement('p');
  localStatus.id = 'dnbox-file-status'; localStatus.className = 'small';
  localStatus.setAttribute('role', 'status'); localStatus.setAttribute('aria-live', 'polite');
  if (stockDrop) stockDrop.after(localStatus);
  const file = doc.getElementById('stockfile');
  if (file) file.setAttribute('aria-label', copy('local'));
  const select = doc.getElementById('version');
  if (select) select.setAttribute('aria-label', copy('select'));
  const scan = doc.getElementById('connbtn');
  const setStage = (stage) => {
    const i = stageLabels.indexOf(stage);
    if (i < 0 || !stages) return;
    current = i;
    for (const [j, li] of [...stages.children].entries()) {
      li.removeAttribute('aria-current');
      li.dataset.state = j < current ? 'complete' : j === current ? 'current' : 'pending';
      if (j === current) li.setAttribute('aria-current', 'step');
    }
  };
  const updateLabels = () => {
    home.setAttribute('aria-label', copy('home'));
    for (const el of doc.querySelectorAll('[data-dnbox-text]')) el.textContent = copy(el.dataset.dnboxText);
    if (stages) {
      stages.setAttribute('aria-label', copy('steps'));
      for (const li of stages.children) li.textContent = copy(li.dataset.stage);
    }
    if (pick && doc.getElementById('dnbox-browser')) {
      const b = doc.getElementById('dnbox-browser');
      b.textContent = copy(b.dataset.state === 'insecure' ? 'browserInsecure' : b.dataset.state === 'unsupported' ? 'browserNo' : 'browserOk');
    }
    if (file) file.setAttribute('aria-label', copy('local'));
    if (select) select.setAttribute('aria-label', copy('select'));
  };
  updateLabels();
  if (typeof env.MutationObserver === 'function') {
    const observer = new env.MutationObserver(updateLabels);
    observer.observe(doc.documentElement, { attributes: true, attributeFilter: ['lang'] });
  }
  const urls = [];
  const loopfiles = doc.getElementById('loopfiles');
  if (loopfiles) loopfiles.addEventListener('change', () => {
    for (const u of urls.splice(0)) env.URL.revokeObjectURL(u);
    preview.replaceChildren();
    const chosen = [...(loopfiles.files || [])];
    if (!chosen.length) return;
    const heading = doc.createElement('h3'); heading.textContent = copy('preview'); preview.append(heading);
    for (const sampleFile of chosen.slice(0, 8)) {
      const row = doc.createElement('div'); row.className = 'dnbox-audio';
      const name = doc.createElement('span'); name.textContent = sampleFile.name;
      const audio = doc.createElement('audio'); audio.controls = true; audio.preload = 'none';
      const url = env.URL.createObjectURL(sampleFile); urls.push(url); audio.src = url;
      audio.setAttribute('aria-label', sampleFile.name);
      row.append(name, audio); preview.append(row);
    }
    if (chosen.length > 8) {
      const note = doc.createElement('p'); note.textContent = copy('previewLimited'); preview.append(note);
    }
  });
  if (typeof env.addEventListener === 'function') env.addEventListener('pagehide', () => {
    for (const u of urls.splice(0)) env.URL.revokeObjectURL(u);
  }, { once: true });
  setStage('scan');
  const api = {
    onStage(stage) { if (!installing) setStage(stage); },
    onConnection(on) {
      if (installing && on && stages) {
        // Completed only after the app has been seen again; a write ACK alone is insufficient.
        for (const li of stages.children) {
          li.dataset.state = 'complete'; li.removeAttribute('aria-current');
        }
        installing = false;
      } else if (!installing && on) setStage('select');
      if (scan) scan.textContent = on ? copy('retry') : copy('scan');
    },
    onStep(step) {
      if (step === 'start' || step === 'verify') setStage('select');
      if (step === 'loader') setStage('protect');
      if (step === 'write' || step === 'reboot') { installing = true; setStage('write'); }
    },
    onStatus(key, message) {
      if (/^stock(?:loading|ok|bad|loadfail)$/.test(key)) {
        localStatus.textContent = message || copy(key === 'stockok' ? 'fileOk' : key === 'stockloading' ? 'fileLoading' : 'fileInvalid');
        localStatus.dataset.state = key === 'stockok' ? 'ok' : key === 'stockloading' ? 'working' : 'error';
      }
      if (key === 'slotsave' || key === 'slotsaved') setStage('protect');
    },
  };
  env.DnBOXSite = api;
  return api;
}

if (typeof document !== 'undefined' && typeof window !== 'undefined') {
  document.addEventListener('DOMContentLoaded', () => enhanceDnboxSite(document, window), { once: true });
}