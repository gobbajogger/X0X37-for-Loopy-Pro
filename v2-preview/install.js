(function () {
  "use strict";
  var sections = Array.prototype.slice.call(document.querySelectorAll("[data-s]"));
  var stageItems = Array.prototype.slice.call(document.querySelectorAll("#stage li"));
  var order = ["device", "version", "path", "backup", "writing", "done"];
  var help = {
    device: "Use the name printed on the case. A USB or port name is not proof of the board model (nightly manual). The real installer also checks the connected device and must stop on a mismatch. Starrykey 37 Play uses the Pro package.",
    version: "0.0.25.1 is the known-good fallback. 0.0.26 at 94197fc8 is a hardware-test candidate: physical startup is not verified, and it is not a stable recommendation. The withdrawn original at 600b3b45 is not offered. If a write goes wrong, recovery is already linked from this card.",
    path: "Chrome or Edge on a computer is the path in the READMEs. Installing from an iPhone with a compatible app is also supported. The exact app and steps are UNVERIFIED: route pending Installer QA. This page does not name an app, and does not say that iPhone or Safari cannot install.",
    backup: "Back up sample slots before you install. Keep the file. The real installer can do this; this prototype cannot. A backup of your own is still the safe option even if an install would try to save and restore loops.",
    writing: "In a real write, keep USB connected. A dark display can be normal; do not power-cycle. This prototype is not writing. The other buttons on this card only show failure states, so they can be read before they happen.",
    done: "A simulated finish is not a confirmed restart. If the screen stays black, that is not success — open recovery, retain the log, package, and model, and restore stock if needed.",
    "err-mismatch": "Nothing should have been written. Choose the package that matches the printed name and the installer’s device check.",
    "err-usb": "Replug and finish the same package. This page cannot tell whether the keyboard will start. Do not switch packages mid-repair.",
    "err-stopped": "Same as a lost cable: finish the same package, or go to recovery if the screen stays dark.",
    "err-nostart": "This is not a successful install. Retain the log, package, and model. Restore stock for this model. Do not reinstall withdrawn 0.0.26 (600b3b45)."
  };

  function currentFromHash() {
    var m = /(?:[#?&]s=)([a-z0-9-]+)/i.exec(location.hash + location.search);
    var s = m ? m[1] : "device";
    if (!document.querySelector('[data-s="' + s + '"]')) s = "device";
    return s;
  }

  function show(s) {
    sections.forEach(function (el) {
      var on = el.getAttribute("data-s") === s;
      el.hidden = !on;
    });
    var idx = order.indexOf(s);
    stageItems.forEach(function (li) {
      var name = li.getAttribute("data-stages");
      var i = order.indexOf(name);
      li.removeAttribute("aria-current");
      li.classList.toggle("is-done", idx > i && idx >= 0);
      if (name === s || (idx === -1 && name === "writing")) {
        li.setAttribute("aria-current", "step");
      }
    });
    var hb = document.getElementById("help-body");
    if (hb) hb.textContent = help[s] || help.device;
    var title = document.querySelector('[data-s="' + s + '"] h1');
    if (title && location.hash !== "#s=" + s) {
      history.replaceState(null, "", "#s=" + s);
    }
    var ack = document.getElementById("ack026");
    if (ack && s === "version") syncAck();
    var sum = document.getElementById("write-summary");
    if (sum && s === "writing") {
      var model = (document.querySelector('input[name="model"]:checked') || {}).value || "unspecified";
      var ver = (document.querySelector('input[name="ver"]:checked') || {}).value;
      var path = (document.querySelector('input[name="path"]:checked') || {}).value || "unspecified";
      var vlabel = ver === "026" ? "0.0.26 candidate 94197fc8" : "0.0.25.1";
      sum.textContent = "Chosen in this simulation: " + model + " package, " + vlabel + ", path “" + path + ".”";
    }
    if (show.doScroll) {
      var card = document.getElementById("card");
      if (card) card.scrollIntoView({ block: "start" });
    }
  }

  function required(name) {
    return document.querySelector('input[name="' + name + '"]:checked');
  }

  function syncAck() {
    var v = document.querySelector('input[name="ver"]:checked');
    var ack = document.getElementById("ack026");
    if (ack) ack.hidden = !(v && v.value === "026");
  }

  document.addEventListener("change", function (e) {
    if (e.target && e.target.name === "ver") syncAck();
  });

  document.addEventListener("click", function (e) {
    var btn = e.target.closest("[data-next]");
    if (!btn) return;
    e.preventDefault();
    var need = btn.getAttribute("data-need");
    if (need && !required(need)) {
      var fs = document.querySelector('[data-s]:not([hidden]) fieldset');
      if (fs) {
        fs.classList.add("callout", "warn");
        var legend = fs.querySelector("legend");
        if (legend) legend.focus ? legend.scrollIntoView({ block: "center" }) : null;
      }
      return;
    }
    if (btn.id === "ver-next") {
      var v = document.querySelector('input[name="ver"]:checked');
      if (v && v.value === "026") {
        var box = document.getElementById("ack026box");
        if (!box || !box.checked) {
          var ack = document.getElementById("ack026");
          if (ack) { ack.hidden = false; ack.classList.add("callout", "warn"); }
          return;
        }
      }
    }
    show.doScroll = true;
    show(btn.getAttribute("data-next"));
  });

  window.addEventListener("hashchange", function () { show(currentFromHash()); });
  show.doScroll = false;
  show(currentFromHash());
})();
