import { openSerial, ping, installApp } from "./tfproto.js";

const grid = document.getElementById("grid");
const logEl = document.getElementById("log");
function log(s) { logEl.textContent += s + "\n"; logEl.scrollTop = logEl.scrollHeight; }

let port = null;
let catalog = { apps: [] };

async function load() {
  const r = await fetch("apps/catalog.json");
  catalog = await r.json();
  render(catalog.apps);
}

function render(apps) {
  grid.innerHTML = "";
  for (const a of apps) {
    const el = document.createElement("article");
    el.className = "app";
    el.innerHTML = `
      <div><span class="tag">${a.category}</span></div>
      <h3>${a.name}</h3>
      <div class="meta">${a.author} · v${a.version}</div>
      <p>${a.description}</p>
      <div class="row">
        <button class="btn" data-install="${a.file}">Install to device</button>
        <a class="btn ghost" href="apps/${a.file}" download>Download</a>
      </div>`;
    grid.appendChild(el);
  }
  grid.querySelectorAll("[data-install]").forEach((b) => {
    b.onclick = () => install(b.getAttribute("data-install"));
  });
}

async function ensurePort() {
  if (port) return port;
  port = await openSerial();
  const p = await ping(port);
  log("Connected " + (p.text || ""));
  return port;
}

async function install(file) {
  try {
    await ensurePort();
    const r = await fetch("apps/" + file);
    const buf = new Uint8Array(await r.arrayBuffer());
    log("Installing " + file + " (" + buf.length + " bytes)");
    const res = await installApp(port, buf);
    log("→ " + res);
  } catch (e) {
    log("ERR " + e.message);
  }
}

document.getElementById("filter").oninput = (e) => {
  const q = e.target.value.toLowerCase();
  render(catalog.apps.filter(a =>
    (a.name + a.description + a.category + a.author).toLowerCase().includes(q)));
};

document.getElementById("connect").onclick = () => ensurePort().catch(e => log(e.message));

document.getElementById("upload").addEventListener("change", async (ev) => {
  const f = ev.target.files[0];
  if (!f) return;
  const text = await f.text();
  let json;
  try { json = JSON.parse(text); }
  catch { log("Not JSON"); return; }
  if (!json.name) { log("Missing name"); return; }
  try {
    await ensurePort();
    const buf = new TextEncoder().encode(text);
    const res = await installApp(port, buf);
    log("Sideload " + json.name + " → " + res);
  } catch (e) { log(e.message); }
});

document.getElementById("submit").onclick = () => {
  const body = encodeURIComponent(
    "### App submission\n\nPaste the .tapp JSON below.\n\n```json\n{\n  \"id\": \"my-app\",\n  \"name\": \"My App\",\n  \"version\": \"1.0.0\",\n  \"author\": \"you\",\n  \"category\": \"Tools\",\n  \"description\": \"…\",\n  \"items\": [\"Hello\"]\n}\n```\n"
  );
  window.open("https://github.com/fak3hidden/Firmware-test/issues/new?title=App%20store%20submission&body=" + body);
};

load().catch((e) => { grid.textContent = "Failed to load catalog: " + e.message; });
