import { openSerial, ping, info, installApp, writeBytes } from "./tfproto.js";

const logEl = document.getElementById("log");
const statusEl = document.getElementById("status");
function log(s) {
  logEl.textContent += s + "\n";
  logEl.scrollTop = logEl.scrollHeight;
}
function setStatus(s, ok) {
  statusEl.textContent = s;
  statusEl.style.color = ok === false ? "var(--bad)" : ok ? "var(--ok)" : "var(--muted)";
}

const FW = {
  plus: "firmware/finos-t-embed-cc1101-plus.bin",
  base: "firmware/finos-t-embed-cc1101.bin",
};

let port = null;

document.getElementById("connect").onclick = async () => {
  try {
    port = await openSerial();
    setStatus("Port open", true);
    log("Serial open at 115200. Probing FinOS…");
    try {
      const p = await ping(port);
      log("PING " + (p.text || JSON.stringify(p)));
      const inf = await info(port);
      if (inf) log("INFO " + inf);
      setStatus("FinOS talking", true);
    } catch (e) {
      log("No FinOS banner (device may be in bootloader). " + e.message);
    }
  } catch (e) {
    setStatus(e.message, false);
    log("ERR " + e.message);
  }
};

document.getElementById("ping").onclick = async () => {
  if (!port) return log("Connect first");
  const p = await ping(port);
  log("PING " + (p.text || ""));
};

async function flashBin(url) {
  log("Fetching " + url);
  const res = await fetch(url);
  if (!res.ok) throw new Error("Firmware binary not published yet (" + res.status + "). A GitHub Action build uploads it here. You can also pick a local .bin.");
  const buf = new Uint8Array(await res.arrayBuffer());
  log("Image " + buf.length + " bytes. Opening esptool-js…");
  await flashBytes(buf);
}

async function flashBytes(u8) {
  if (!navigator.serial) throw new Error("Web Serial required");
  /* Close CLI port so esptool can claim it. */
  try { if (port) { await port.close(); port = null; } } catch (_) {}
  const filters = [{ usbVendorId: 0x303a }, { usbVendorId: 0x10c4 }, { usbVendorId: 0x1a86 }];
  const p = await navigator.serial.requestPort({ filters }).catch(() => navigator.serial.requestPort());
  log("Hold the encoder knob (BOOT) and tap RESET if the stub does not sync.");
  const mod = await import("https://unpkg.com/esptool-js@0.5.3/lib/index.js");
  const Transport = mod.Transport || mod.default?.Transport;
  const ESPLoader = mod.ESPLoader || mod.default?.ESPLoader;
  if (!ESPLoader) {
    /* fallback bundle */
    await flashViaBundle(p, u8);
    return;
  }
  const transport = new Transport(p, true);
  const loader = new ESPLoader({
    transport,
    baudrate: 115200,
    romBaudrate: 115200,
    terminal: { clean() {}, writeLine: log, write: log },
  });
  const chip = await loader.main();
  log("Chip: " + chip);
  const binStr = uint8ToBinaryString(u8);
  await loader.writeFlash({
    fileArray: [{ data: binStr, address: 0x0 }],
    flashSize: "16MB",
    flashMode: "qio",
    flashFreq: "80m",
    eraseAll: false,
    compress: true,
  });
  log("Flash done. Resetting…");
  try { await loader.after("hard_reset"); } catch (_) {}
  setStatus("Flashed", true);
}

function uint8ToBinaryString(u8) {
  let s = "";
  const chunk = 0x8000;
  for (let i = 0; i < u8.length; i += chunk) {
    s += String.fromCharCode.apply(null, u8.subarray(i, i + chunk));
  }
  return s;
}

async function flashViaBundle(p, u8) {
  log("Using esptool-js bundle fallback");
  const m = await import("https://cdn.jsdelivr.net/npm/esptool-js@0.4.5/+esm");
  log("Loaded " + Object.keys(m).join(", "));
  throw new Error("Could not find ESPLoader. Use qFin desktop to flash, or retry.");
}

document.getElementById("flash-plus").onclick = () => flashBin(FW.plus).catch(e => { setStatus(e.message, false); log("ERR " + e.message); });
document.getElementById("flash-base").onclick = () => flashBin(FW.base).catch(e => { setStatus(e.message, false); log("ERR " + e.message); });

document.getElementById("flash-file").addEventListener("change", async (ev) => {
  const f = ev.target.files[0];
  if (!f) return;
  const buf = new Uint8Array(await f.arrayBuffer());
  log("Local file " + f.name + " " + buf.length + " bytes");
  flashBytes(buf).catch(e => { setStatus(e.message, false); log("ERR " + e.message); });
});

document.getElementById("install-app").addEventListener("change", async (ev) => {
  const f = ev.target.files[0];
  if (!f) return;
  if (!port) { log("Connect to a running FinOS device first"); return; }
  const buf = new Uint8Array(await f.arrayBuffer());
  log("Installing " + f.name + " (" + buf.length + " bytes)");
  try {
    const r = await installApp(port, buf);
    log("INSTALL " + r);
    setStatus(r.includes("OK") ? "App installed" : r, r.includes("OK"));
  } catch (e) {
    log("ERR " + e.message);
  }
});
