/* DnBOX DESIGN PREVIEW. All transitions are simulated; no device APIs. */
(function () {
  "use strict";
  var sections = Array.prototype.slice.call(document.querySelectorAll("[data-s]"));
  var stageItems = Array.prototype.slice.call(document.querySelectorAll("#stage li"));
  var order = ["device", "version", "path", "permission", "backup", "writing", "transfer", "restart", "boot"];
  var help = {
    device: "Choose the hardware model printed on the case. A USB name is not model evidence. The real installer must enforce its own match check before a write.",
    version: "0.0.25.1 is the fallback. The owner reports Elite installs of 0.0.25.1 and 0.0.26, and a restart/boot on 0.0.26. Exact installed hash, Pro and full-function qualification remain unconfirmed.",
    path: "The computer Chrome/Edge route is documented. The iPhone route is UNVERIFIED: no app, OS, connection method or actual install result has been validated.",
    "path-unverified": "No iPhone support claim can be made from an untested proposed route. This is a knowledge gap, not a universal impossibility.",
    permission: "A real installer asks for Web MIDI with a user gesture. Denied, unavailable, unrecognized-device and permitted states are different. This preview requests no permissions.",
    "err-midi-denied": "No transfer began. Review site permissions in the browser; retry only through the real installer's user-controlled connection step.",
    "err-midi-unavailable": "No transfer began. Use the real installer's documented secure browser environment and read its specific error.",
    "err-midi-device": "Permission does not identify the hardware. Check the physical connection and let the real installer refuse model mismatches.",
    backup: "Export important slots. Automatic backup failure is warning-only under c237b0d1, never an obligatory install blocker. Overwriting an existing preset has a separate backup-first rule.",
    "backup-warning": "A backup is missing or failed. Continuing risks personal sample data, but installer permission to proceed is not dependent on a successful slot backup.",
    writing: "This preview sends nothing. A real transfer requires correct model, explicit browser permission, USB continuity, and device-specific protection.",
    transfer: "An acknowledged byte transfer is not a restart, a boot or a musical functional test.",
    restart: "A restarted keyboard is not automatically running the new firmware. Confirm an actual boot and functionality separately.",
    boot: "This is a visitor-selected simulated observation, not owner, Pro, firmware hash or full-function proof.",
    "err-mismatch": "No write should start on a model mismatch. Select using the printed device name.",
    "err-usb": "Transfer stopped. Follow the real installer's recovery steps for the same model/package. Do not guess readiness.",
    "err-stopped": "This may leave an incomplete write. Keep exact package information and follow model-specific recovery.",
    "err-nostart": "No confirmed boot. Retain logs and package details and use the reviewed recovery procedure."
  };
  function stageFor(s) {
    if (order.indexOf(s) !== -1) return s;
    if (s === "path-unverified") return "path";
    if (s.indexOf("err-midi-") === 0) return "permission";
    if (s === "backup-warning") return "backup";
    if (s === "err-nostart") return "boot";
    return "writing";
  }
  function fromLocation() {
    var m = /(?:[#?&]s=)([a-z0-9-]+)/i.exec(location.hash + location.search);
    return m && document.querySelector('[data-s="' + m[1] + '"]') ? m[1] : "device";
  }
  function chosen(name) { return document.querySelector('input[name="' + name + '"]:checked'); }
  function syncAck() {
    var box = document.getElementById("ack026");
    var version = chosen("ver");
    if (box) box.hidden = !(version && version.value === "026");
  }
  function show(s) {
    if (!document.querySelector('[data-s="' + s + '"]')) s = "device";
    sections.forEach(function (el) { el.hidden = el.getAttribute("data-s") !== s; });
    var active = stageFor(s), idx = order.indexOf(active);
    stageItems.forEach(function (li) {
      var at = li.getAttribute("data-stages"), n = order.indexOf(at);
      li.classList.toggle("is-done", n >= 0 && n < idx);
      li.removeAttribute("aria-current");
      if (at === active) li.setAttribute("aria-current", "step");
    });
    var helpBody = document.getElementById("help-body");
    if (helpBody) helpBody.textContent = help[s] || help.device;
    if (location.hash !== "#s=" + s) history.replaceState(null, "", "#s=" + s);
    if (s === "version") syncAck();
    if (s === "writing") {
      var model = chosen("model"), ver = chosen("ver"), path = chosen("path"), bak = chosen("bak");
      var summary = document.getElementById("write-summary");
      if (summary) summary.textContent = "Simulation only: " + (model ? model.value : "unselected") +
        ", " + (ver && ver.value === "026" ? "0.0.26 candidate" : "0.0.25.1 fallback") +
        ", " + (path ? path.value : "unselected") + ". Backup: " +
        (bak ? bak.value : "no backup confirmed") + ". No device write.";
    }
    if (show.scroll) {
      var card = document.getElementById("card");
      if (card) card.scrollIntoView({ block: "start" });
    }
  }
  function warnMissing(name) {
    if (chosen(name)) return false;
    var field = document.querySelector('[data-s]:not([hidden]) fieldset');
    if (field) { field.classList.add("callout", "warn"); field.scrollIntoView({ block: "center" }); }
    return true;
  }
  document.addEventListener("change", function (e) {
    if (e.target && e.target.name === "ver") syncAck();
  });
  document.addEventListener("click", function (e) {
    var btn = e.target.closest("[data-next]");
    if (!btn) return;
    e.preventDefault();
    var need = btn.getAttribute("data-need");
    if (need && warnMissing(need)) return;
    if (btn.id === "ver-next") {
      var v = chosen("ver"), box = document.getElementById("ack026box");
      if (v && v.value === "026" && (!box || !box.checked)) {
        var warn = document.getElementById("ack026");
        if (warn) { warn.hidden = false; warn.classList.add("callout", "warn"); warn.scrollIntoView({ block: "center" }); }
        return;
      }
    }
    var next = btn.getAttribute("data-next");
    if (btn.id === "path-next") next = chosen("path").value === "iphone" ? "path-unverified" : "permission";
    if (btn.id === "backup-next") next = chosen("bak") && chosen("bak").value === "done" ? "writing" : "backup-warning";
    show.scroll = true;
    show(next);
  });
  window.addEventListener("hashchange", function () { show(fromLocation()); });
  show.scroll = false;
  show(fromLocation());
})();
