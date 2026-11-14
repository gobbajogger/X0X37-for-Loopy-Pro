// Host-only composition adapter. The decoded Settings document stays unchanged.
(() => {
  const $ = id => document.getElementById(id);
  const settings = window.__designData.settings;
  let frame = null, token = '', pendingTask = null;
  const routeNames = new Set(['overview.html', 'README.md', 'source-receipt.json', 'qa/interaction-results.json', 'emulator-evidence.json']);
  function dispose() {
    // Destroying the browsing context releases the component's repeat timers
    // and held-input owners, rather than preserving them in a hidden frame.
    if (frame) frame.remove();
    frame = null; pendingTask = null;
    $('settingsMount').replaceChildren();
    $('closeSettings').hidden = true;
    const current = settings.cases[Number($('settingsChoice').value) || 0];
    $('settingsStage').textContent = 'Settings inspection closed. Current comparison: '+current.title+'. Open its separate inspection task to use the model controls.';
  }
  function render() {
    const c = settings.cases[Number($('settingsChoice').value) || 0];
    $('settingsPair').innerHTML = `<h3>${esc(c.title)}</h3><p><strong>Comparison boundary:</strong> ${esc(c.match)}</p><div class="compare"><div><span class="tag">ACTUAL HISTORICAL · requested25.1</span><p><img class="lcd" width="240" height="240" src="${c.actual_image}" alt="Actual historical Settings: ${esc(c.title)}"></p><p class="caption">Exact accepted R2 capture. Registry assigns this package0.0.25; hidden state matches only where declared.</p></div><div><span class="tag">PROPOSED · R2 MODEL · source94197fc8</span><p><img class="lcd" width="240" height="240" src="${c.proposed_image}" alt="Settings R2 proposal: ${esc(c.title)}"></p></div></div><div class="rationale"><div><strong>Retained</strong><p>${esc(c.stayed)}</p></div><div><strong>Changed / reconsidered</strong><p>${esc(c.changed)}</p></div><div><strong>Tradeoff / limits</strong><p>${esc(c.cost)}</p></div></div><ol>${c.steps.map(s=>`<li>${esc(s)}</li>`).join('')}</ol>`;
  }
  function openTask() {
    dispose();
    const c = settings.cases[Number($('settingsChoice').value) || 0];
    pendingTask = c.task;
    token = crypto.randomUUID ? crypto.randomUUID() : String(Date.now()) + Math.random();
    frame = document.createElement('iframe');
    frame.id = 'settingsFrame';
    frame.title = 'Settings R2 interaction model — separate inspection fixture';
    frame.setAttribute('sandbox', 'allow-scripts');
    frame.style.cssText = 'display:block;width:100%;height:1500px;border:1px solid #48616a;background:#101719';
    const original = new TextDecoder().decode(Uint8Array.from(atob(settings.component_b64), c => c.charCodeAt(0)));
    const child = `(() => {
      const token=${JSON.stringify(token)};
      document.addEventListener('click',e=>{const a=e.target.closest('a');if(!a)return;e.preventDefault();parent.postMessage({type:'dnbox-settings-route',token,path:a.getAttribute('href')},'*')});
      addEventListener('message',e=>{if(e.source!==parent||e.data?.type!=='dnbox-settings-stage'||e.data.token!==token)return;try{study.scene(e.data.task);parent.postMessage({type:'dnbox-settings-staged',token,task:e.data.task},'*')}catch(error){parent.postMessage({type:'dnbox-settings-error',token,error:String(error)},'*')}});
      parent.postMessage({type:'dnbox-settings-ready',token},'*');
    })();`;
    frame.srcdoc = original + '<script>' + child + '<' + '/script>';
    $('settingsMount').append(frame);
    $('closeSettings').hidden = false;
    $('settingsReference').hidden = true;
    $('settingsStage').textContent = 'Opening original R2 inspection task: '+c.task+'. Its original fixture is separate from the frozen pair.';
  }
  window.addEventListener('message', e => {
    if (!frame || e.source !== frame.contentWindow || e.data?.token !== token) return;
    const d = e.data;
    if (d.type === 'dnbox-settings-ready' && pendingTask) {
      frame.contentWindow.postMessage({type:'dnbox-settings-stage',token,task:pendingTask}, '*');
    } else if (d.type === 'dnbox-settings-staged') {
      $('settingsStage').textContent = 'Original R2 inspection task: '+d.task+'. Controls below run its unchanged design model; the original fixture differs from the paired baseline. No firmware, audio or storage runs.';
      pendingTask = null;
    } else if (d.type === 'dnbox-settings-error') {
      $('settingsStage').textContent = 'Settings task did not stage: '+d.error;
    } else if (d.type === 'dnbox-settings-route' && routeNames.has(d.path)) {
      if (d.path === 'overview.html') {
        dispose(); $('settingsChoice').focus();
        $('settingsStage').textContent = 'Returned to the six curated Settings comparisons. Inspection fixture closed.';
      } else {
        $('settingsReferenceTitle').textContent = 'Unchanged Settings reference: '+d.path;
        $('settingsReferenceText').textContent = settings.references[d.path];
        $('settingsReference').hidden = false;
        $('settingsReference').scrollIntoView({block:'nearest'});
      }
    }
  });
  $('settingsChoice').innerHTML = settings.cases.map((c,i)=>`<option value="${i}">${esc(c.title)}</option>`).join('');
  $('settingsChoice').onchange = () => {dispose(); render(); $('settingsReference').hidden=true;};
  $('openSettings').onclick = openTask;
  $('closeSettings').onclick = () => {dispose(); $('openSettings').focus();};
  document.querySelectorAll('[data-tab]').forEach(b => b.addEventListener('click', () => {if(b.dataset.tab!=='settings')dispose();}));
  $('showLoad').onclick = () => {
    dispose();
    document.querySelector('[data-tab="compare"]').click();
    $('screenChoice').value = 'LOAD-QUESTION';
    renderScreen();
    $('screenChoice').closest('details').open = true;
    $('screenChoice').focus();
  };
  render();
})();
