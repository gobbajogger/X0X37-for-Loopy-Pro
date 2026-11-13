(function () {
  "use strict";
  var sections = Array.prototype.slice.call(document.querySelectorAll("[data-s]"));
  var stageItems = Array.prototype.slice.call(document.querySelectorAll("#stage li"));
  var map = {
    browse: "browse",
    download: "download",
    upload: "upload",
    valid: "valid",
    confirm: "confirm",
    result: "result",
    "err-badfile": "result",
    "err-newer": "result",
    "err-wrongengine": "result",
    "err-memory": "result",
    "err-factory": "result",
    "err-sample": "valid"
  };

  function current() {
    var m = /(?:[#?&]s=)([a-z0-9-]+)/i.exec(location.hash + location.search);
    var s = m ? m[1] : "browse";
    if (!document.querySelector('[data-s="' + s + '"]')) s = "browse";
    return s;
  }
  function fileFromHash() {
    var m = /(?:[#?&]file=)([^&]+)/i.exec(location.hash + location.search);
    return m ? decodeURIComponent(m[1]) : "";
  }
  function show(s) {
    sections.forEach(function (el) {
      el.hidden = el.getAttribute("data-s") !== s;
    });
    var stageName = map[s] || "browse";
    stageItems.forEach(function (li) {
      var name = li.getAttribute("data-stages");
      if (name === stageName) li.setAttribute("aria-current", "step");
      else li.removeAttribute("aria-current");
    });
    var nm = document.getElementById("dl-name");
    var f = fileFromHash();
    if (nm && f) nm.textContent = f + ".dnpr";
    var done = document.getElementById("dl-done");
    if (done && s === "download") done.hidden = true;
  }

  document.addEventListener("click", function (e) {
    var dl = e.target.closest("#fake-dl");
    if (dl) {
      e.preventDefault();
      var done = document.getElementById("dl-done");
      if (done) { done.hidden = false; done.focus(); }
      return;
    }
    var conf = e.target.closest("#do-confirm");
    if (conf) {
      e.preventDefault();
      var box = document.getElementById("bak-slot");
      if (!box || !box.checked) {
        var wrap = box && box.closest(".callout");
        if (wrap) wrap.scrollIntoView({ block: "center" });
        return;
      }
      location.hash = "s=result";
    }
  });

  var pick = document.getElementById("filepick");
  if (pick) {
    pick.addEventListener("change", function () {
      var note = document.getElementById("file-note");
      if (!note) return;
      var f = pick.files && pick.files[0];
      if (!f) { note.textContent = "No file chosen."; return; }
      var extra = "";
      if (f.size > 548 && !/\.zip$/i.test(f.name)) {
        extra = " This file is larger than 548 bytes, the maximum in research §4, so a real check would already reject it as BAD FILE — but this prototype is not running the codec.";
      }
      note.textContent = "Chosen: " + f.name + " (" + f.size + " bytes)." + extra + " Use the simulated result buttons for the contract states. This page does not compute the plan CRC.";
    });
  }

  window.addEventListener("hashchange", function () { show(current()); });
  show(current());
})();
