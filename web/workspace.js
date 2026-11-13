// SPDX-License-Identifier: GPL-3.0-only
// Offline audio preparation and task navigation. No MIDI permission or device access here.
export function initDnboxWorkspace({ $, t }) {
  let prepared = [], audioUrls = [], serial = 0, transferPending = false;
  const clearAudio = () => {
    document.querySelectorAll('#preplist audio').forEach((node) => node.pause());
    audioUrls.forEach((url) => URL.revokeObjectURL(url)); audioUrls = [];
    $('preplist').replaceChildren();
  };
  function view() {
    const name = ['samples', 'projects'].includes(location.hash.slice(1)) ? location.hash.slice(1) : 'install';
    for (const key of ['install', 'samples', 'projects']) {
      $(key + 'view').hidden = key !== name;
      const anchor = document.querySelector(`[data-view="${key}"]`);
      if (key === name) anchor.setAttribute('aria-current', 'page'); else anchor.removeAttribute('aria-current');
    }
    document.querySelectorAll('#preplist audio').forEach((node) => node.pause());
    if (name !== 'install') {
      const frame = $(name + 'frame');
      if (!frame.getAttribute('src')) frame.src = location.pathname.endsWith('/x0x_installer.html') ? `./x0x_${name === 'samples' ? 'breaks' : 'projects'}.html` : frame.dataset.src;
    }
  }
  window.addEventListener('hashchange', view); view();
  window.addEventListener('message', (event) => {
    if (event.origin !== location.origin) return;
    if (event.source === $('samplesframe').contentWindow) {
      if (event.data?.type === 'dnbox-prepared-received') { transferPending = false; $('prepstatus').textContent = t('preptransferred'); }
      if (event.data?.type === 'dnbox-prepared-queued') $('prepstatus').textContent = t('prepqueued');
      if (event.data?.type === 'dnbox-prepared-declined') { transferPending = false; $('prepstatus').textContent = t('prepdeclined'); }
    }
    if (event.source === $('samplesframe').contentWindow && event.data?.type === 'dnbox-samples-ready' && transferPending) {
      event.source.postMessage({ type: 'dnbox-prepared', loops: prepared }, location.origin); transferPending = false;
    }
    for (const name of ['samples', 'projects']) if (event.source === $(name + 'frame').contentWindow &&
        event.data?.type === 'dnbox-height' && Number.isFinite(event.data.height))
      $(name + 'frame').style.height = Math.max(300, Math.min(20000, event.data.height + 24)) + 'px';
  });
  const prepare = async () => {
    const request = ++serial;
    const files = [...$('prepaudio').files], rate = Number($('preprate').value), factor = Number($('prepspeed').value);
    clearAudio(); prepared = []; transferPending = false;
    if (files.length > 64 || files.reduce((n, file) => n + file.size, 0) > 32 * 1024 * 1024) { $("prepstatus").textContent = t("prepfailed") + "Choose at most 64 files and 32 MiB total."; return; }
    $('prepstatus').textContent = t('preparing');
    try {
      for (const file of files) {
        if (file.size > 32 * 1024 * 1024 || !file.size) throw new Error(file.name + ': empty or larger than 32 MiB');
        const raw = new Uint8Array(await file.arrayBuffer());
        if (request !== serial) return;
        let decoded;
        try { decoded = readWav(raw); }
        catch {
          const ctx = new OfflineAudioContext(1, 1, 44100);
          const buffer = await ctx.decodeAudioData(raw.buffer.slice(0));
          if (buffer.duration > 120) throw new Error(file.name + ': longer than two minutes');
          const x = new Float32Array(buffer.length);
          for (let c = 0; c < buffer.numberOfChannels; c++) {
            const ch = buffer.getChannelData(c); for (let i = 0; i < x.length; i++) x[i] += ch[i] / buffer.numberOfChannels;
          }
          decoded = { rate: buffer.sampleRate, x };
        }
        if (request !== serial) return;
        if (decoded.x.length / decoded.rate > 120) throw new Error(file.name + ': longer than two minutes');
        const cut = cutLoop(decoded.x, decoded.rate, false, rate, false, factor);
        const { samples, rate: playback } = loopPreviewPcm(cut.s, cut.rate);
        prepared.push({ file: file.name, s: cut.s, n: cut.s.length, rate: cut.rate, adpcm: imaEncode(cut.s),
          gritFactor: factor, isNew: true, srcRate: decoded.rate, x: Array.from(decoded.x) });
        const row = document.createElement('div'); row.className = 'prepared-loop';
        const name = document.createElement('p'); name.textContent = file.name;
        const info = document.createElement('p'); info.className = 'small';
        const baseline = Math.ceil(decoded.x.length * rate / decoded.rate / 2), used = Math.ceil(cut.s.length / 2);
        info.textContent = `${t('prepsaved')}: ${used.toLocaleString()} B / ${baseline.toLocaleString()} B (${Math.round(100 * (1 - used / baseline))}% saved). ${t('prepplayback')}: ${playback} Hz. ${(samples.length / playback).toFixed(2)} s.`;
        const audio = document.createElement('audio'); audio.controls = true; audio.preload = 'metadata';
        audio.setAttribute('aria-label', file.name);
        const pcm = new Int16Array(samples.length); for (let i = 0; i < samples.length; i++) pcm[i] = Math.round(samples[i] * 32767);
        const wav = new Uint8Array(44 + pcm.length * 2), v = new DataView(wav.buffer);
        const text = (off, value) => { for (let i = 0; i < value.length; i++) wav[off + i] = value.charCodeAt(i); };
        text(0, 'RIFF'); v.setUint32(4, wav.length - 8, true); text(8, 'WAVEfmt '); v.setUint32(16, 16, true);
        v.setUint16(20, 1, true); v.setUint16(22, 1, true); v.setUint32(24, playback, true); v.setUint32(28, playback * 2, true);
        v.setUint16(32, 2, true); v.setUint16(34, 16, true); text(36, 'data'); v.setUint32(40, pcm.length * 2, true);
        for (let i = 0; i < pcm.length; i++) v.setInt16(44 + i * 2, pcm[i], true);
        audio.src = URL.createObjectURL(new Blob([wav], { type: 'audio/wav' })); audioUrls.push(audio.src);
        row.append(name, info, audio); $('preplist').append(row);
      }
      if (request !== serial) return;
      $('prepstatus').textContent = files.length ? t('prepready') : '';
      if (prepared.length) {
        const use = document.createElement('button'); use.type = 'button'; use.textContent = t('opensamples');
        use.onclick = () => {
          const frame = $('samplesframe');
          transferPending = true; location.hash = 'samples'; view();
          // Already-loaded workspace receives exactly the same processed PCM/ADPCM,
          // not a WAV resampled again at the default import rate.
          frame.contentWindow?.postMessage({ type: 'dnbox-prepared', loops: prepared }, location.origin);
        };
        $('preplist').append(use);
      }
    } catch (e) {
      if (request !== serial) return;
      clearAudio(); prepared = []; $('prepstatus').textContent = t('prepfailed') + e.message;
    }
  };
  $('prepaudio').onchange = $('preprate').onchange = $('prepspeed').onchange = prepare;
}
