const $ = (id) => document.getElementById(id);

const canvas = $("mirror");
const ctx = canvas.getContext("2d");
const img = ctx.createImageData(canvas.width, canvas.height);

function rgbFromHex(hex) {
  const v = parseInt(hex.replace("#",""), 16);
  return { r: (v>>16)&255, g: (v>>8)&255, b: v&255 };
}

async function fetchState() {
  const r = await fetch("/api/state", { cache: "no-store" });
  return r.json();
}

function setMsg(s, ok=true) {
  const el = $("msg");
  el.textContent = s;
  el.style.opacity = "0.9";
  el.style.color = ok ? "#b6f2c2" : "#ffb3b3";
  setTimeout(()=>{ el.style.opacity = "0.6"; }, 2500);
}

function setControls(state) {
  $("time").textContent = state.time;
  $("date").textContent = state.date;
  $("wifi").textContent = state.wifi;
  $("ip").textContent = state.ip;

  $("tz").value = state.tz || "";
  $("ntp").value = state.ntp || "";
  $("use24h").value = String(state.use24h);

  $("ledd").value = state.ledDiameter;
  $("ledg").value = state.ledGap;

  const col = (state.ledColor >>> 0).toString(16).padStart(6,"0");
  $("col").value = "#" + col;
  $("bl").value = state.brightness;
}

async function fetchMirror() {
  const r = await fetch("/api/mirror", { cache: "no-store" });
  const buf = new Uint8Array(await r.arrayBuffer()); // 4096 bytes
  // Draw intensity as red pixels on the canvas
  for (let i = 0; i < canvas.width * canvas.height; i++) {
    const v = buf[i]; // 0..255
    const off = i * 4;
    img.data[off+0] = v; // red channel
    img.data[off+1] = 0;
    img.data[off+2] = 0;
    img.data[off+3] = 255;
  }
  ctx.putImageData(img, 0, 0);
}

async function saveConfig() {
  const state = await fetchState();

  const tz = $("tz").value.trim() || state.tz;
  const ntp = $("ntp").value.trim() || state.ntp;
  const use24h = $("use24h").value === "true";

  const ledDiameter = parseInt($("ledd").value, 10);
  const ledGap = parseInt($("ledg").value, 10);
  const brightness = parseInt($("bl").value, 10);

  const { r, g, b } = rgbFromHex($("col").value);
  const ledColor = (r<<16) | (g<<8) | b;

  const payload = { tz, ntp, use24h, ledDiameter, ledGap, ledColor, brightness };

  const res = await fetch("/api/config", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  });

  if (!res.ok) {
    setMsg("Save failed: " + (await res.text()), false);
    return;
  }
  setMsg("Saved. NTP/timezone updated.");
}

$("save").addEventListener("click", () => saveConfig().catch(e => setMsg(String(e), false)));

async function tick() {
  try {
    const state = await fetchState();
    setControls(state);
    await fetchMirror();
  } catch (e) {
    console.warn(e);
  } finally {
    setTimeout(tick, 1000);
  }
}

tick();
