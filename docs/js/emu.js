/* Interactive 128x64 FinOS demo (browser). Pixel-perfect nearest-neighbour scale. */
(() => {
  const canvas = document.getElementById("emu");
  if (!canvas) return;
  const SCALE = 4;
  canvas.width = 128 * SCALE;
  canvas.height = 64 * SCALE;
  const ctx = canvas.getContext("2d");
  ctx.imageSmoothingEnabled = false;

  const fb = document.createElement("canvas");
  fb.width = 128; fb.height = 64;
  const f = fb.getContext("2d");
  f.imageSmoothingEnabled = false;

  const dolphin = new Image();
  dolphin.src = "img/dolphin-1bit.png";

  const items = [
    "Sub-GHz", "125 kHz RFID", "NFC", "Infrared",
    "GPIO", "iButton", "Bad USB", "U2F",
    "nRF24", "Wi-Fi", "Applications", "Archive", "Settings"
  ];
  let screen = "desktop";
  let sel = 0;
  let t = 0;
  let fontReady = false;

  const font = new FontFace("Micro5", "url(fonts/Micro5-Regular.ttf)");
  font.load().then((ff) => { document.fonts.add(ff); fontReady = true; }).catch(() => { fontReady = true; });

  function ink() { f.fillStyle = "#000"; f.strokeStyle = "#000"; }
  function paper() { f.fillStyle = "#fff"; }

  function text(x, y, s, size = 15) {
    f.font = `${size}px Micro5, monospace`;
    f.textBaseline = "top";
    f.fillText(s, x, y);
  }

  function rrect(x, y, w, h, r, fill) {
    f.beginPath();
    f.moveTo(x + r, y);
    f.arcTo(x + w, y, x + w, y + h, r);
    f.arcTo(x + w, y + h, x, y + h, r);
    f.arcTo(x, y + h, x, y, r);
    f.arcTo(x, y, x + w, y, r);
    if (fill) f.fill(); else f.stroke();
  }

  function status(title) {
    ink();
    text(2, 1, title, 15);
    f.strokeRect(109.5, 2.5, 14, 8);
    f.fillRect(111, 4, 9, 5);
    f.fillRect(124, 4, 2, 5);
    f.fillRect(0, 11, 128, 1);
  }

  function drawDesktop() {
    paper(); f.fillRect(0, 0, 128, 64);
    ink();
    const now = new Date();
    const hh = String(now.getHours()).padStart(2, "0");
    const mm = String(now.getMinutes()).padStart(2, "0");
    status(`${hh}:${mm}`);
    if (dolphin.complete) {
      f.drawImage(dolphin, 28, 13);
    }
    text(2, 55, "OK menu", 14);
    text(90, 55, "Plus", 14);
  }

  function drawMenu() {
    paper(); f.fillRect(0, 0, 128, 64);
    ink();
    status("Main Menu");
    const vis = 6;
    let win = 0;
    if (sel >= vis) win = sel - vis + 1;
    /* icon box */
    f.strokeRect(8, 22, 32, 32);
    f.beginPath(); f.arc(24, 36, 10, 0, Math.PI * 2); f.stroke();
    f.fillRect(22, 46, 4, 8); f.fillRect(16, 54, 16, 2);
    for (let i = 0; i < vis; i++) {
      const idx = win + i;
      if (idx >= items.length) break;
      const y = 14 + i * 8;
      if (idx === sel) {
        f.fillStyle = "#000";
        rrect(50, y - 1, 76, 8, 2, true);
        f.fillStyle = "#fff";
        text(54, y, items[idx], 14);
        ink();
      } else {
        text(54, y, items[idx], 14);
      }
    }
  }

  function drawApp() {
    paper(); f.fillRect(0, 0, 128, 64);
    ink();
    status(items[sel]);
    text(6, 20, items[sel], 16);
    text(6, 34, "Connect a T-Embed", 14);
    text(6, 44, "to run this for real.", 14);
    text(6, 55, "Back = desktop", 14);
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
  function onUp() {
    if (screen === "menu") sel = (sel + items.length - 1) % items.length;
    blit();
  }
  function onDown() {
    if (screen === "menu") sel = (sel + 1) % items.length;
    blit();
  }

  window.addEventListener("keydown", (e) => {
    if (["ArrowUp", "ArrowLeft"].includes(e.key)) { e.preventDefault(); onUp(); }
    if (["ArrowDown", "ArrowRight"].includes(e.key)) { e.preventDefault(); onDown(); }
    if (e.key === "Enter") onOk();
    if (e.key === "Escape" || e.key === "Backspace") onBack();
  });

  const knob = document.getElementById("knob");
  if (knob) {
    knob.addEventListener("click", onOk);
    knob.addEventListener("wheel", (e) => { e.preventDefault(); if (e.deltaY > 0) onDown(); else onUp(); }, { passive: false });
  }
  const back = document.getElementById("backbtn");
  if (back) back.addEventListener("click", onBack);

  setInterval(() => { t++; blit(); }, 400);
  dolphin.onload = blit;
  blit();
})();
