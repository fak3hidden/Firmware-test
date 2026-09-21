/* Official Flipper-style 128x64 demo: 3-line framed main menu + desktop. */
(() => {
  const canvas = document.getElementById("emu");
  if (!canvas) return;
  const SCALE = 4;
  canvas.width = 128 * SCALE;
  canvas.height = 64 * SCALE;
  const ctx = canvas.getContext("2d");
  const fb = document.createElement("canvas");
  fb.width = 128; fb.height = 64;
  const f = fb.getContext("2d");
  f.imageSmoothingEnabled = false;

  const dolphin = new Image();
  dolphin.src = "img/dolphin-1bit.png";

  const items = [
    "Sub-GHz", "125 kHz RFID", "NFC", "Infrared",
    "GPIO", "iButton", "Bad USB", "U2F",
    "Bluetooth", "Applications", "Archive", "Settings"
  ];
  let screen = "desktop";
  let sel = 0;
  const font = new FontFace("Micro5", "url(fonts/Micro5-Regular.ttf)");
  font.load().then((ff) => document.fonts.add(ff)).catch(() => {});

  function ink() { f.fillStyle = "#000"; f.strokeStyle = "#000"; }
  function paper() { f.fillStyle = "#fff"; }
  function text(x, y, s, size) {
    f.font = `${size}px Micro5, monospace`;
    f.textBaseline = "alphabetic";
    f.fillText(s, x, y);
  }
  function frame(x, y, w, h) {
    ink();
    f.fillRect(x + 2, y, w - 4, 1);
    f.fillRect(x + 1, y + h - 1, w - 1, 1);
    f.fillRect(x + 2, y + h, w - 3, 1);
    f.fillRect(x, y + 2, 1, h - 4);
    f.fillRect(x + w - 1, y + 1, 1, h - 3);
    f.fillRect(x + w, y + 2, 1, h - 4);
    f.fillRect(x + 1, y + 1, 1, 1);
  }
  function scrollbar(pos, total) {
    paper(); f.fillRect(125, 0, 3, 64);
    ink();
    for (let i = 0; i < 64; i += 2) f.fillRect(126, i, 1, 1);
    const bh = Math.max(1, Math.floor(64 / total));
    f.fillRect(125, Math.floor(64 * pos / total), 3, bh);
  }
  function status() {
    paper(); f.fillRect(0, 0, 128, 13);
    ink();
    const rbox = (x, y, w, h, r) => {
      if (typeof f.roundRect === "function") {
        f.beginPath(); f.roundRect(x, y, w, h, r); f.fill();
      } else f.fillRect(x, y, w, h);
    };
    rbox(0, 0, 70, 11, 3);
    rbox(72, 0, 56, 11, 3);
    paper();
    f.fillRect(2, 2, 66, 7);
    f.fillRect(74, 2, 52, 7);
    ink();
    const d = new Date();
    text(4, 9, `${String(d.getHours()).padStart(2,"0")}:${String(d.getMinutes()).padStart(2,"0")}`, 14);
    f.strokeRect(109.5, 2.5, 14, 8);
    f.fillRect(111, 4, 9, 5);
    f.fillRect(124, 4, 2, 5);
  }

  function drawDesktop() {
    paper(); f.fillRect(0, 0, 128, 64);
    status();
    if (dolphin.complete) f.drawImage(dolphin, 8, 14);
  }

  function drawMenu() {
    paper(); f.fillRect(0, 0, 128, 64);
    ink();
    const n = items.length;
    const prev = (sel + n - 1) % n;
    const next = (sel + 1) % n;
    text(22, 14, items[prev], 14);
    text(22, 36, items[sel], 16);
    text(22, 58, items[next], 14);
    f.strokeRect(4, 3, 14, 14);
    f.strokeRect(4, 25, 14, 14);
    f.fillRect(4, 25, 14, 14);
    f.strokeRect(4, 47, 14, 14);
    frame(0, 21, 123, 21);
    scrollbar(sel, n);
  }

  function drawApp() {
    paper(); f.fillRect(0, 0, 128, 64);
    ink();
    text(4, 11, items[sel], 16);
    text(4, 28, "Connect a T-Embed", 14);
    text(4, 40, "to run this for real.", 14);
    /* Flipper left button */
    f.fillRect(0, 52, 40, 12);
    f.fillRect(40, 52, 1, 12);
    f.fillRect(41, 53, 1, 11);
    f.fillRect(42, 54, 1, 10);
    f.fillStyle = "#fff";
    text(8, 61, "Back", 14);
  }

  function blit() {
    if (screen === "desktop") drawDesktop();
    else if (screen === "menu") drawMenu();
    else drawApp();
    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(fb, 0, 0, canvas.width, canvas.height);
  }
  function onOk() {
    if (screen === "desktop") screen = "menu";
    else if (screen === "menu") screen = "app";
    blit();
  }
  function onBack() {
    if (screen === "app") screen = "menu";
    else if (screen === "menu") screen = "desktop";
    blit();
  }
  function onUp() { if (screen === "menu") sel = (sel + items.length - 1) % items.length; blit(); }
  function onDown() { if (screen === "menu") sel = (sel + 1) % items.length; blit(); }

  window.addEventListener("keydown", (e) => {
    if (["ArrowUp", "ArrowLeft"].includes(e.key)) { e.preventDefault(); onUp(); }
    if (["ArrowDown", "ArrowRight"].includes(e.key)) { e.preventDefault(); onDown(); }
    if (e.key === "Enter") onOk();
    if (e.key === "Escape" || e.key === "Backspace") onBack();
  });
  const knob = document.getElementById("knob");
  if (knob) {
    knob.addEventListener("click", onOk);
    knob.addEventListener("wheel", (e) => { e.preventDefault(); (e.deltaY > 0 ? onDown : onUp)(); }, { passive: false });
  }
  const back = document.getElementById("backbtn");
  if (back) back.addEventListener("click", onBack);
  setInterval(blit, 500);
  dolphin.onload = blit;
  blit();
})();
