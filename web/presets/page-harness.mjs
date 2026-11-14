// SPDX-License-Identifier: GPL-3.0-only
// Source-driven DOM event checks for the W3 file-only page.
// Node runner: web/presets/test_page.mjs. The harness itself is runtime-neutral.
export async function checkPresetPage({ appSource, htmlSource, codec, zip }) {
  const failures = [], passed = [];
  const expect = (condition, message) => { if (!condition) throw Error(message); };
  const treeText = node => [node.textContent || "", ...(node.children || []).map(treeText)].join(" ");
  async function check(name, callback) {
    try { await callback(); passed.push(name); }
    catch (error) { failures.push(name + ": " + error.message); }
  }

  const makePreset = (engine = "chicago", origin = 2, factoryIndex = 0, sampleCount = null) => {
    const spec = Object.values(codec.DNPR_ENGINES).find(e => e.token === engine);
    return codec.encodeDnpr({
      engine, name:"Owner " + engine, sourceName:"OWNER", origin, factoryIndex,
      params:Uint8Array.from({ length:spec.params }, (_,i)=>i & 255),
      sampleRefs:Array.from({length:sampleCount ?? (spec.samples === -1 ? 0 : spec.samples)}, (_,i)=>({
        source:1, index:i, crc:0x11223344 + i, name:"R" + i
      }))
    });
  };
  function environment() {
    const elements = new Map(), downloads = [], blobs = new Map();
    let acceptsClear = true, blobId = 0;
    class Element {
      constructor(tag = "div") {
        this.tagName = tag.toUpperCase(); this.hidden = false; this.disabled = false; this.checked = false;
        this.value = ""; this.textContent = ""; this.dataset = {}; this.attributes = {};
        this.children = []; this.files = []; this.handlers = new Map();
        this.classList = { add(){}, remove(){} };
      }
      addEventListener(name, callback) { this.handlers.set(name, callback); }
      emit(name, event = {}) { return this.handlers.get(name)?.(event); }
      setAttribute(name, value) { this.attributes[name] = String(value); }
      append(...children) { this.children.push(...children); }
      replaceChildren(...children) { this.children = children; }
      remove() { }
      focus() { }
      click() { if (this.tagName === "A") downloads.push({ name:this.download, blob:blobs.get(this.href) }); }
    }
    for (const m of htmlSource.matchAll(/<([a-z][a-z0-9]*)\b([^>]*\bid="([^"]+)"[^>]*)>/g)) {
      const el = new Element(m[1]), attrs = m[2];
      el.disabled = /\bdisabled\b/.test(attrs); el.hidden = /\bhidden\b/.test(attrs);
      el.value = attrs.match(/\bvalue="([^"]*)"/)?.[1] || "";
      elements.set(m[3], el);
    }
    const document = {
      getElementById(id) { if (!elements.has(id)) throw Error("Missing DOM element " + id); return elements.get(id); },
      createElement: tag => new Element(tag),
      body: new Element("body")
    };
    const window = { confirm: () => acceptsClear };
    class LocalBlob {
      constructor(parts) { this.parts = parts; }
      async arrayBuffer() {
        const arrays = this.parts.map(p => p instanceof Uint8Array ? p : new Uint8Array(p));
        const out = new Uint8Array(arrays.reduce((s,a)=>s+a.length,0));
        let at = 0; for (const part of arrays) { out.set(part,at); at += part.length; }
        return out.buffer;
      }
    }
    const URL = { createObjectURL(blob) { const url = "blob:local-" + ++blobId; blobs.set(url,blob); return url; },
      revokeObjectURL(url) { blobs.delete(url); }
    };
    const source = appSource.replace(/^import .*;\s*$/gm, "");
    new Function("document","window","URL","Blob","setTimeout",
      "PresetError","PresetSession","parseDnpr","ensureExportable","fileNameFor","sampleWarnings",
      "readDnprZip","makeDnprZip","ZIP_MAX_INPUT",source)(
      document,window,URL,LocalBlob,()=>0,
      codec.PresetError,codec.PresetSession,codec.parseDnpr,codec.ensureExportable,codec.fileNameFor,codec.sampleWarnings,
      zip.readDnprZip,zip.makeDnprZip,zip.ZIP_MAX_INPUT
    );
    const $ = id => document.getElementById(id);
    const file = (name, bytes) => ({
      name, size:bytes.length, async arrayBuffer() { return bytes.slice().buffer; }
    });
    const choose = async (...files) => $("files").emit("change",{target:{files}});
    const click = id => $(id).emit("click");
    const selectedDownload = async () => downloads.length ?
      new Uint8Array(await downloads[downloads.length - 1].blob.arrayBuffer()) : null;
    return { $, file, choose, click, downloads, selectedDownload, setConfirmation(v) { acceptsClear = v; } };
  }

  await check("dark-only layout, focusable controls, status region and local-only policy", async () => {
    for (const id of ["model","target","files","keep","discard","save-file","save-zip","clear",
      "preview","preview-meta","preview-warnings","session-list","session-detail","status"]) {
      expect(new RegExp('id="' + id + '"').test(htmlSource), "Missing " + id);
    }
    expect(/color-scheme:\s*dark/.test(htmlSource) && !/color-scheme:\s*light/.test(htmlSource),
      "Dark-only palette must be fixed");
    expect(/:focus-visible/.test(htmlSource) && /max-width:740px/.test(htmlSource),
      "Focus or mobile affordance missing");
    expect(/role="status"/.test(htmlSource), "Missing live status region");
    expect(!/\bfetch\s*\(|requestMIDIAccess|navigator\.usb|navigator\.serial|localStorage|sessionStorage|\.send\s*\(/.test(appSource),
      "A device/network/persistent-storage call appeared in file-only UI");
  });

  await check("picker preview is non-mutating until explicit keep", async () => {
    const ui = environment(), b = makePreset("chicago");
    await ui.choose(ui.file("owner.chicago.dnpr",b));
    expect(ui.$("preview").hidden === false, "Preview not shown");
    expect(ui.$("session-list").children.length === 0, "Preview silently kept a file");
    expect(/passed file checks/.test(ui.$("status").textContent), "No validation status");
    ui.click("discard"); expect(ui.$("preview").hidden, "Discard leaves preview active");
    expect(ui.$("session-list").children.length === 0, "Discard mutated session");
    await ui.choose(ui.file("owner.chicago.dnpr",b));
    ui.click("keep"); expect(ui.$("session-list").children.length === 1, "Keep did not stage one file");
    expect(ui.$("save-file").disabled === false, "Download stayed disabled");
    ui.click("save-file");
    expect(ui.downloads[0].name.endsWith(".chicago.dnpr"), "Wrong file name");
    expect(JSON.stringify([...await ui.selectedDownload()]) === JSON.stringify([...b]), "Download changed bytes");
  });

  await check("bad CRC, wrong engine and newer schema cannot change existing session", async () => {
    const ui = environment(), valid = makePreset("chicago");
    await ui.choose(ui.file("safe.chicago.dnpr",valid)); ui.click("keep");
    const corrupt = Uint8Array.from(valid); corrupt[16] ^= 1;
    await ui.choose(ui.file("bad.dnpr",corrupt));
    expect(/BAD FILE/.test(ui.$("status").textContent), "Bad CRC not surfaced");
    expect(ui.$("session-list").children.length === 1, "Bad CRC changed session");
    ui.$("target").value = "miami";
    await ui.choose(ui.file("safe.chicago.dnpr",valid));
    expect(/WRONG ENGINE/.test(ui.$("status").textContent), "Wrong engine not surfaced");
    ui.$("target").value = "";
    const newer = Uint8Array.from(valid); newer[8] = 2;
    new DataView(newer.buffer).setUint32(16,codec.crc32(newer,true),true);
    await ui.choose(ui.file("future.chicago.dnpr",newer));
    expect(/NEWER DATA/.test(ui.$("status").textContent), "Newer schema not surfaced");
    expect(ui.$("session-list").children.length === 1, "Rejected file changed session");
  });

  await check("bulk ZIP preflight rejects one bad record without partial staging", async () => {
    const ui = environment(), valid = makePreset("miami"), bad = Uint8Array.from(valid); bad[20] = 0xff;
    new DataView(bad.buffer).setUint32(16,codec.crc32(bad,true),true);
    const zipped = zip.makeDnprZip([{name:"A.miami.dnpr",bytes:valid},{name:"B.miami.dnpr",bytes:bad}]);
    await ui.choose(ui.file("presets.zip",zipped));
    expect(/BAD FILE/.test(ui.$("status").textContent), "Bulk rejection not visible");
    expect(ui.$("session-list").children.length === 0, "Bulk failure inserted partial records");
  });

  await check("bulk ZIP success and per-file roundtrip with unique names", async () => {
    const ui = environment(), a = makePreset("juno"), b = makePreset("drone");
    const archive = zip.makeDnprZip([{name:"A.juno.dnpr",bytes:a},{name:"B.drone.dnpr",bytes:b}]);
    await ui.choose(ui.file("owner.zip",archive));
    expect(ui.$("preview").hidden === false && /2 validated records/.test(ui.$("preview-count").textContent),
      "ZIP preview missing");
    ui.click("keep"); expect(ui.$("session-list").children.length === 2, "ZIP staging failed");
    ui.click("save-zip"); expect(ui.downloads.at(-1).name === "DnBOX-user-presets.dnpr.zip", "ZIP filename");
    const restored = await zip.readDnprZip(await ui.selectedDownload());
    expect(restored.length === 2, "ZIP export lost files");
    expect(JSON.stringify([...restored[0].bytes]) === JSON.stringify([...a]), "Juno bytes changed");
    expect(JSON.stringify([...restored[1].bytes]) === JSON.stringify([...b]), "Drone bytes changed");
  });

  await check("sample-reference warning is visible in preview and session", async () => {
    const ui = environment();
    await ui.choose(ui.file("kit.909.dnpr",makePreset("909")));
    expect(!ui.$("preview-warnings").hidden && /not checked against keyboard/.test(ui.$("preview-warnings").children[1].children[0].textContent),
      "Sample integrity warning missing");
    ui.click("keep");
    expect(!ui.$("selected-warnings").hidden, "Sample warning hidden after keeping file");
  });

  await check("factory-derived record can be inspected, but neither download starts", async () => {
    const ui = environment(), factory = makePreset("miami",1,1);
    await ui.choose(ui.file("factory.miami.dnpr",factory)); ui.click("keep");
    expect(/Blocked/.test(ui.$("selected-meta").children.at(-1).children[1].textContent), "Factory block not displayed");
    ui.click("save-file"); expect(ui.downloads.length===0, "Factory export leaked bytes");
    expect(/FACTORY EXPORT OFF/.test(ui.$("status").textContent), "Factory refusal not visible");
    ui.click("save-zip"); expect(ui.downloads.length===0, "ZIP exported factory-derived data");
  });

  await check("multi-record preview shows sample warning on second record and reversed order", async () => {
    const clean = makePreset("chicago"), kit = makePreset("909");
    for (const second of [true, false]) {
      const ui = environment();
      const files = second
        ? [ui.file("clean.chicago.dnpr", clean), ui.file("kit.909.dnpr", kit)]
        : [ui.file("kit.909.dnpr", kit), ui.file("clean.chicago.dnpr", clean)];
      await ui.choose(...files);
      const warnings = ui.$("preview-warnings"), label = "Record " + (second ? 2 : 1) + " of 2: Owner 909 (909)";
      expect(!warnings.hidden && warnings.children.length === 1, "Missing indexed sample warning");
      expect(treeText(warnings).includes(label) && /not checked against keyboard/.test(treeText(warnings)),
        "Sample warning not associated with the correct record");
      expect(ui.$("preview-meta").children.length === 2 &&
        treeText(ui.$("preview-meta")).includes("Owner 909") &&
        treeText(ui.$("preview-meta")).includes("Owner chicago"), "Not all record metadata shown");
      expect(ui.$("session-list").children.length === 0, "Multi-file preview mutated session");
      ui.click("discard");
      expect(ui.$("preview").hidden && ui.$("session-list").children.length === 0, "Discard failed");
    }
  });

  await check("multi-record factory warning is visible in either order before keep and export stays blocked", async () => {
    const clean = makePreset("chicago"), factory = makePreset("miami", 1, 1);
    for (const second of [true, false]) {
      const ui = environment();
      const files = second
        ? [ui.file("clean.chicago.dnpr", clean), ui.file("factory.miami.dnpr", factory)]
        : [ui.file("factory.miami.dnpr", factory), ui.file("clean.chicago.dnpr", clean)];
      await ui.choose(...files);
      const warnings = ui.$("preview-warnings"), index = second ? 2 : 1;
      expect(!warnings.hidden && treeText(warnings).includes("Record " + index + " of 2: Owner miami (Miami)") &&
        treeText(warnings).includes("FACTORY EXPORT OFF"), "Factory warning missing before bulk Keep");
      expect(ui.$("session-list").children.length === 0, "Factory preview changed session");
      ui.click("keep");
      expect(ui.$("session-list").children.length === 2, "Keep failed to stage both validated records");
      ui.click("save-zip");
      expect(ui.downloads.length === 0 && /FACTORY EXPORT OFF/.test(ui.$("status").textContent),
        "Bulk ZIP exported a factory-derived record");
      ui.$("session-list").children[index - 1].emit("click");
      ui.click("save-file");
      expect(ui.downloads.length === 0 && /FACTORY EXPORT OFF/.test(ui.$("status").textContent),
        "Factory-derived single-file export was not blocked");
    }
  });

  await check("three-record mixed ZIP shows both sample and factory warnings before any bulk Keep", async () => {
    const ui = environment();
    const archive = zip.makeDnprZip([
      { name:"clean.chicago.dnpr", bytes:makePreset("chicago") },
      { name:"kit.909.dnpr", bytes:makePreset("909") },
      { name:"factory.miami.dnpr", bytes:makePreset("miami", 1, 1) }
    ]);
    await ui.choose(ui.file("mixed.zip", archive));
    const warnings = ui.$("preview-warnings"), text = treeText(warnings);
    expect(/3 validated records/.test(ui.$("preview-count").textContent), "ZIP count missing");
    expect(ui.$("preview-meta").children.length === 3, "All ZIP metadata not visible");
    expect(!warnings.hidden && warnings.children.length === 2, "Both ZIP warnings must be shown");
    expect(text.includes("Record 2 of 3: Owner 909 (909)") && text.includes("not checked against keyboard"),
      "ZIP sample warning missing or mislabelled");
    expect(text.includes("Record 3 of 3: Owner miami (Miami)") && text.includes("FACTORY EXPORT OFF"),
      "ZIP factory warning missing or mislabelled");
    expect(ui.$("session-list").children.length === 0, "ZIP preview silently staged records");
    ui.click("keep");
    expect(ui.$("session-list").children.length === 3, "ZIP Keep failed");
    ui.click("save-zip");
    expect(ui.downloads.length === 0 && /FACTORY EXPORT OFF/.test(ui.$("status").textContent),
      "Mixed ZIP export bypassed factory guard");
  });

  await check("cross-engine original-source warning on second record is visible before Keep", async () => {
    const ui = environment(), conflict = Uint8Array.from(makePreset("chicago"));
    conflict[21] = 2;
    new DataView(conflict.buffer).setUint32(16, codec.crc32(conflict, true), true);
    await ui.choose(ui.file("clean.chicago.dnpr", makePreset("chicago")),
      ui.file("source-other.chicago.dnpr", conflict));
    const warnings = ui.$("preview-warnings"), text = treeText(warnings);
    expect(!warnings.hidden && warnings.children.length === 1, "Source conflict warning hidden");
    expect(text.includes("Record 2 of 2: Owner chicago (Chicago)") &&
      text.includes("Original engine differs from record engine"), "Source-conflict warning missing");
    expect(ui.$("session-list").children.length === 0, "Source-conflict preview mutated session");
  });

  await check("explicit target accepts actual engine and preserves provenance warning and downloads", async () => {
    for (const [engine, provenance] of [["chicago",2],["miami",1]]) {
      const ui=environment(), b=makePreset(engine); b[21]=provenance;
      new DataView(b.buffer).setUint32(16,codec.crc32(b,true),true);
      ui.$("target").value=engine;
      await ui.choose(ui.file("provenance.dnpr",b));
      expect(!ui.$("preview").hidden, "Actual-engine target rejected valid provenance");
      const warning=treeText(ui.$("preview-warnings"));
      expect(warning.includes("Original engine differs from record engine") &&
        warning.includes("provenance") && warning.includes("actual record engine") &&
        !warning.includes("blocked unless both agree"), "Misleading target/provenance warning");
      expect(ui.$("session-list").children.length===0, "Preview changed session");
      ui.click("keep");
      expect(ui.$("session-list").children.length===1, "Keep did not accept actual engine");
      expect(treeText(ui.$("selected-warnings")).includes("provenance"), "Provenance warning lost after Keep");
      ui.click("save-file");
      expect(String(await ui.selectedDownload())===String(b), "Provenance file bytes changed");
    }
  });

  await check("rejected target and invalid provenance/schema/CRC cannot enter pending or replace session", async () => {
    for (const [engine, provenance, wrong] of [["chicago",2,"miami"],["miami",1,"chicago"]]) {
      const ui=environment(), b=makePreset(engine); b[21]=provenance;
      new DataView(b.buffer).setUint32(16,codec.crc32(b,true),true);
      await ui.choose(ui.file("existing.dnpr",makePreset(engine))); ui.click("keep");
      const retained=ui.$("session-list").children[0];
      for (const [at,value,code] of [[null,null,"WRONG ENGINE"],[21,255,"BAD FILE"],[8,2,"NEWER DATA"],[100,255,"BAD FILE"]]) {
        ui.$("target").value=at===null ? wrong : engine;
        const bad=Uint8Array.from(b);
        if (at!==null) { bad[at]=value; if(at!==100) new DataView(bad.buffer).setUint32(16,codec.crc32(bad,true),true); }
        await ui.choose(ui.file("valid.dnpr",makePreset(ui.$("target").value)),ui.file("bad.dnpr",bad));
        expect(ui.$("preview").hidden && ui.$("session-list").children[0]===retained &&
          ui.$("session-list").children.length===1, "Refusal mutated pending/session");
        expect(ui.$("status").textContent.includes(code), "Wrong rejection code");
        ui.click("keep");
        expect(ui.$("session-list").children.length===1, "Rejected bytes remained pending");
        ui.click("save-file");
        expect(String(await ui.selectedDownload())===String(makePreset(engine)), "Refusal changed existing bytes/selection");
      }
    }
  });

  await check("model filename conflict is blocked; model does not prove device compatibility", async () => {
    const ui=environment(), a=makePreset("chicago"); ui.$("model").value="pro";
    await ui.choose(ui.file("owner.elite.chicago.dnpr",a));
    expect(/WRONG MODEL/.test(ui.$("status").textContent), "Model hint mismatch not rejected");
    expect(ui.$("session-list").children.length===0, "Model mismatch changed session");
    ui.$("model").value="elite";
    await ui.choose(ui.file("owner.elite.chicago.dnpr",a));
    expect(ui.$("preview").hidden===false, "Matching model hint should allow inspection");
  });

  await check("explicit clear cancellation retains session; confirm clears", async () => {
    const ui=environment(), a=makePreset("808",2,0,2);
    await ui.choose(ui.file("kit.808.dnpr",a)); ui.click("keep");
    ui.setConfirmation(false);ui.click("clear");
    expect(ui.$("session-list").children.length===1, "Declined clear lost file");
    ui.setConfirmation(true);ui.click("clear");
    expect(ui.$("session-list").children.length===0, "Confirmed clear failed");
  });

  await check("drag/drop flow is non-destructive and validates bytes", async () => {
    const ui=environment(), bytes=makePreset("break");
    await ui.$("drop").emit("drop",{
      preventDefault(){},dataTransfer:{files:[ui.file("loop.break.dnpr",bytes)]}
    });
    expect(!ui.$("preview").hidden, "Drop did not preview file");
    expect(ui.$("session-list").children.length===0, "Drop committed without confirmation");
  });
  await check("unsupported dropped format is rejected without creating a preview", async () => {
    const ui=environment(), bytes=makePreset("chicago");
    await ui.choose(ui.file("unverified.aupreset",bytes));
    expect(/BAD FILE/.test(ui.$("status").textContent), "Unsupported format not rejected");
    expect(ui.$("preview").hidden===true, "Unsupported format generated a preview");
    expect(ui.$("session-list").children.length===0, "Unsupported format changed session");
  });
  return Object.freeze({ passed, failures, total:passed.length+failures.length });
}