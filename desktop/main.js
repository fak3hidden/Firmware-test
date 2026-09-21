const { app, BrowserWindow, ipcMain, dialog } = require("electron");
const path = require("path");
const { SerialPort } = require("serialport");
const { spawn } = require("child_process");
const fs = require("fs");

let win;
let port = null;

function create() {
  win = new BrowserWindow({
    width: 1100,
    height: 720,
    backgroundColor: "#0c0c0d",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
    },
  });
  win.loadFile(path.join(__dirname, "renderer", "index.html"));
}

app.whenReady().then(create);
app.on("window-all-closed", () => app.quit());

ipcMain.handle("list-ports", async () => {
  const ports = await SerialPort.list();
  return ports.map((p) => ({
    path: p.path,
    manufacturer: p.manufacturer || "",
    vendorId: p.vendorId || "",
    productId: p.productId || "",
  }));
});

ipcMain.handle("open-port", async (_e, pth) => {
  if (port) { try { await port.close(); } catch (_) {} port = null; }
  port = new SerialPort({ path: pth, baudRate: 115200 });
  await new Promise((res, rej) => port.on("open", res) && port.on("error", rej));
  return true;
});

ipcMain.handle("close-port", async () => {
  if (port) { try { await port.close(); } catch (_) {} port = null; }
  return true;
});

function crc8(bytes) {
  let c = 0;
  for (const b of bytes) {
    c ^= b;
    for (let i = 0; i < 8; i++) c = (c & 0x80) ? ((c << 1) ^ 0x07) & 0xff : (c << 1) & 0xff;
  }
  return c;
}
function encodeFrame(cmd, payload = Buffer.alloc(0)) {
  const n = payload.length;
  const out = Buffer.alloc(7 + n);
  out[0] = 0x54; out[1] = 0x46; out[2] = 0x01; out[3] = cmd;
  out.writeUInt16LE(n, 4);
  payload.copy(out, 6);
  const cr = crc8([cmd]) ^ crc8([out[4], out[5]]) ^ (n ? crc8(payload) : 0);
  out[6 + n] = cr;
  return out;
}

function readFor(ms) {
  return new Promise((resolve) => {
    const chunks = [];
    const on = (d) => chunks.push(d);
    if (!port) return resolve(Buffer.alloc(0));
    port.on("data", on);
    setTimeout(() => {
      port.off("data", on);
      resolve(Buffer.concat(chunks));
    }, ms);
  });
}

ipcMain.handle("tf-cmd", async (_e, cmd, payloadB64) => {
  if (!port) throw new Error("not connected");
  const payload = payloadB64 ? Buffer.from(payloadB64, "base64") : Buffer.alloc(0);
  port.write(encodeFrame(cmd, payload));
  const raw = await readFor(800);
  return raw.toString("base64");
});

ipcMain.handle("tf-text", async (_e, line) => {
  if (!port) throw new Error("not connected");
  port.write(line.endsWith("\n") ? line : line + "\n");
  const raw = await readFor(400);
  return raw.toString("utf8");
});

ipcMain.handle("pick-bin", async () => {
  const r = await dialog.showOpenDialog(win, { filters: [{ name: "bin", extensions: ["bin"] }] });
  if (r.canceled) return null;
  return r.filePaths[0];
});

ipcMain.handle("flash", async (_e, { portPath, binPath }) => {
  return await new Promise((resolve) => {
    const args = ["-m", "esptool", "--chip", "esp32s3", "-p", portPath, "-b", "921600",
      "write_flash", "-z", "--flash_mode", "qio", "--flash_size", "16MB", "0x0", binPath];
    const child = spawn("python3", args);
    let out = "";
    child.stdout.on("data", (d) => { out += d; win.webContents.send("flash-log", d.toString()); });
    child.stderr.on("data", (d) => { out += d; win.webContents.send("flash-log", d.toString()); });
    child.on("close", (c) => resolve({ code: c, out }));
  });
});

ipcMain.handle("read-file", async (_e, p) => fs.promises.readFile(p));
