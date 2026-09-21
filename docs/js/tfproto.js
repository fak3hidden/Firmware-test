/* FinOS USB protocol (text CLI + framed TF\x01). */
export function crc8(bytes) {
  let c = 0;
  for (const b of bytes) {
    c ^= b;
    for (let i = 0; i < 8; i++) c = (c & 0x80) ? ((c << 1) ^ 0x07) & 0xff : (c << 1) & 0xff;
  }
  return c;
}

export function encodeFrame(cmd, payload = new Uint8Array()) {
  const n = payload.length;
  const out = new Uint8Array(7 + n);
  out[0] = 0x54; out[1] = 0x46; out[2] = 0x01; out[3] = cmd;
  out[4] = n & 0xff; out[5] = (n >> 8) & 0xff;
  out.set(payload, 6);
  const cr = crc8([cmd]) ^ crc8([out[4], out[5]]) ^ (n ? crc8(payload) : 0);
  out[6 + n] = cr;
  return out;
}

export function decodeFrames(buf) {
  const frames = [];
  let i = 0;
  while (i + 7 <= buf.length) {
    if (buf[i] !== 0x54 || buf[i + 1] !== 0x46 || buf[i + 2] !== 0x01) { i++; continue; }
    const cmd = buf[i + 3];
    const n = buf[i + 4] | (buf[i + 5] << 8);
    if (i + 7 + n > buf.length) break;
    const payload = buf.slice(i + 6, i + 6 + n);
    frames.push({ cmd, payload, text: new TextDecoder().decode(payload) });
    i += 7 + n;
  }
  return { frames, rest: buf.slice(i) };
}

export async function openSerial() {
  if (!navigator.serial) throw new Error("Web Serial is not available in this browser. Use Chrome or Edge.");
  const port = await navigator.serial.requestPort();
  await port.open({ baudRate: 115200 });
  return port;
}

export async function writeBytes(port, bytes) {
  const w = port.writable.getWriter();
  try { await w.write(bytes); }
  finally { w.releaseLock(); }
}

export async function readTimeout(port, ms = 800) {
  const r = port.readable.getReader();
  const chunks = [];
  const t = setTimeout(() => { try { r.cancel(); } catch (_) {} }, ms);
  try {
    for (;;) {
      const { value, done } = await r.read();
      if (done) break;
      if (value) chunks.push(value);
    }
  } catch (_) {}
  clearTimeout(t);
  try { r.releaseLock(); } catch (_) {}
  let n = 0; for (const c of chunks) n += c.length;
  const out = new Uint8Array(n);
  let o = 0; for (const c of chunks) { out.set(c, o); o += c.length; }
  return out;
}

export async function ping(port) {
  await writeBytes(port, encodeFrame(0x01));
  const raw = await readTimeout(port, 600);
  const { frames } = decodeFrames(raw);
  return frames.find(f => f.cmd === 0x81) || frames[0] || { text: new TextDecoder().decode(raw) };
}

export async function info(port) {
  await writeBytes(port, encodeFrame(0x02));
  const raw = await readTimeout(port, 800);
  const { frames } = decodeFrames(raw);
  const f = frames.find(x => x.cmd === 0x82);
  return f ? f.text : "";
}

export async function installApp(port, bytes) {
  await writeBytes(port, encodeFrame(0x08, bytes));
  const raw = await readTimeout(port, 1500);
  const { frames } = decodeFrames(raw);
  const f = frames.find(x => x.cmd === 0x88);
  return f ? f.text : new TextDecoder().decode(raw);
}

export async function screenshot(port) {
  await writeBytes(port, encodeFrame(0x0B));
  const raw = await readTimeout(port, 1000);
  const { frames } = decodeFrames(raw);
  const f = frames.find(x => x.cmd === 0x8B);
  return f ? f.payload : null;
}

export async function putFile(port, path, data) {
  const p = new TextEncoder().encode(path);
  const payload = new Uint8Array(p.length + 1 + data.length);
  payload.set(p, 0);
  payload.set(data, p.length + 1);
  await writeBytes(port, encodeFrame(0x05, payload));
  const raw = await readTimeout(port, 1500);
  return new TextDecoder().decode(raw);
}
