const $ = (id) => document.getElementById(id);
let binPath = null;
let portPath = null;

function decodeFrames(b64) {
  const buf = Uint8Array.from(atob(b64), (c) => c.charCodeAt(0));
  const frames = [];
  let i = 0;
  while (i + 7 <= buf.length) {
    if (buf[i] !== 0x54 || buf[i+1] !== 0x46 || buf[i+2] !== 0x01) { i++; continue; }
    const cmd = buf[i+3];
    const n = buf[i+4] | (buf[i+5] << 8);
    if (i + 7 + n > buf.length) break;
    const payload = buf.slice(i+6, i+6+n);
    frames.push({ cmd, text: new TextDecoder().decode(payload) });
    i += 7 + n;
  }
  return frames;
}

async function refreshPorts() {
  const ports = await window.qfin.listPorts();
  $("ports").innerHTML = ports.map((p) =>
    `<option value="${p.path}">${p.path} ${p.manufacturer}</option>`).join("");
}
$("refresh").onclick = refreshPorts;
refreshPorts();

$("connect").onclick = async () => {
  portPath = $("ports").value;
  await window.qfin.openPort(portPath);
  const raw = await window.qfin.tfCmd(0x02, null);
  const f = decodeFrames(raw)[0];
  $("devname").textContent = "FinOS";
  $("devmeta").textContent = f ? f.text : portPath;
};

document.querySelectorAll(".tabs button").forEach((b) => {
  b.onclick = () => {
    document.querySelectorAll(".tabs button").forEach((x) => x.classList.remove("on"));
    b.classList.add("on");
    document.querySelectorAll(".pane").forEach((p) => p.classList.add("hidden"));
    $(b.dataset.tab).classList.remove("hidden");
  };
});

$("pick").onclick = async () => { binPath = await window.qfin.pickBin(); $("flog").textContent += (binPath || "") + "\n"; };
window.qfin.onFlashLog((s) => { $("flog").textContent += s; $("flog").scrollTop = 1e9; });
$("flash").onclick = async () => {
  if (!binPath) binPath = await window.qfin.pickBin();
  if (!binPath) return;
  if (!portPath) portPath = $("ports").value;
  await window.qfin.closePort();
  $("flog").textContent += "Flashing…\n";
  const r = await window.qfin.flash({ portPath, binPath });
  $("flog").textContent += "exit " + r.code + "\n";
};

$("ls").onclick = async () => {
  const path = $("path").value || "/ext";
  const b64 = btoa(path);
  const raw = await window.qfin.tfCmd(0x03, b64);
  const f = decodeFrames(raw)[0];
  $("listing").textContent = f ? f.text : "";
};

$("clilin").addEventListener("keydown", async (e) => {
  if (e.key !== "Enter") return;
  const line = $("clilin").value;
  $("clilin").value = "";
  const out = await window.qfin.tfText(line);
  $("cliout").textContent += "> " + line + "\n" + out + "\n";
});

$("pick2").onclick = $("pick").onclick;
$("flash2").onclick = $("flash").onclick;

$("appsls").onclick = async () => {
  const raw = await window.qfin.tfCmd(0x0E, null);
  const f = decodeFrames(raw)[0];
  $("appsout").textContent = f ? f.text : "";
};
