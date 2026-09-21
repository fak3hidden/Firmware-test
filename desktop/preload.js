const { contextBridge, ipcRenderer } = require("electron");
contextBridge.exposeInMainWorld("qfin", {
  listPorts: () => ipcRenderer.invoke("list-ports"),
  openPort: (p) => ipcRenderer.invoke("open-port", p),
  closePort: () => ipcRenderer.invoke("close-port"),
  tfCmd: (cmd, b64) => ipcRenderer.invoke("tf-cmd", cmd, b64),
  tfText: (line) => ipcRenderer.invoke("tf-text", line),
  pickBin: () => ipcRenderer.invoke("pick-bin"),
  flash: (opts) => ipcRenderer.invoke("flash", opts),
  onFlashLog: (cb) => ipcRenderer.on("flash-log", (_e, s) => cb(s)),
});
