const $ = (id) => document.getElementById(id);

const canvas = $("mirror");
const ctx = canvas.getContext("2d");
ctx.imageSmoothingEnabled = false;
let mirrorPitch = 1;

const TFT_W = 320;
const TFT_H = 240;
const STATUS_BAR_H = 50;  // Must match config.h (bottom status bar)
const LED_W = 64;
const LED_H = 32;

const dirtyInputs = new Set();

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

  if (document.activeElement !== $("tz")) $("tz").value = state.tz || "";
  if (document.activeElement !== $("ntp")) $("ntp").value = state.ntp || "";
  if (document.activeElement !== $("use24h")) $("use24h").value = String(state.use24h);

  if (!dirtyInputs.has("ledd")) $("ledd").value = state.ledDiameter;
  if (!dirtyInputs.has("ledg")) $("ledg").value = state.ledGap;

  const col = (state.ledColor >>> 0).toString(16).padStart(6,"0");
  $("col").value = "#" + col;
  $("bl").value = state.brightness;

}

async function fetchMirror() {
  const r = await fetch("/api/mirror", { cache: "no-store" });
  const buf = new Uint8Array(await r.arrayBuffer());
  console.log(`[Fetch] Buffer size: ${buf.length} bytes (expected ${LED_W * LED_H})`);
  // Log first few values
  console.log(`[Fetch] First 10 bytes: ${Array.from(buf.slice(0, 10)).join(',')}`);
  return buf;
}

async function saveConfig() {
  const state = await fetchState();

  const tz = $("tz").value.trim() || state.tz;
  const ntp = $("ntp").value.trim() || state.ntp;
  const use24h = $("use24h").value === "true";

  const ledDiameterRaw = parseInt($("ledd").value, 10);
  const ledGapRaw = parseInt($("ledg").value, 10);
  const brightness = parseInt($("bl").value, 10);

  const { r, g, b } = rgbFromHex($("col").value);
  const ledColor = (r<<16) | (g<<8) | b;

  const ledDiameter = Number.isFinite(ledDiameterRaw) ? ledDiameterRaw : state.ledDiameter;
  const ledGap = Number.isFinite(ledGapRaw) ? ledGapRaw : state.ledGap;
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
  dirtyInputs.clear();
}

$("save").addEventListener("click", () => saveConfig().catch(e => setMsg(String(e), false)));

function renderMirror(buf, state) {
  // IMPORTANT: This must exactly match the TFT rendering logic in main.cpp:394-475
  // The TFT uses cfg.ledDiameter and cfg.ledGap to determine dot size and spacing

  console.log(`[State] ledDiameter=${state.ledDiameter} ledGap=${state.ledGap} type=${typeof state.ledDiameter}`);

  let ledDiameter = parseInt(state.ledDiameter, 10);
  let ledGap = parseInt(state.ledGap, 10);
  if (isNaN(ledDiameter)) ledDiameter = 5;
  if (isNaN(ledGap)) ledGap = 0;

  const matrixAreaH = TFT_H - STATUS_BAR_H;
  let maxPitch = Math.min(Math.floor(TFT_W / LED_W), Math.floor(matrixAreaH / LED_H));
  if (maxPitch < 1) maxPitch = 1;
  const pitch = maxPitch;
  mirrorPitch = pitch;

  if (canvas.width !== TFT_W || canvas.height !== TFT_H) {
    canvas.width = TFT_W;
    canvas.height = TFT_H;
  }

  // Match the TFT rendering logic exactly (main.cpp:405-419)
  let gapWanted = ledGap;
  if (gapWanted < 0) gapWanted = 0;
  if (gapWanted > pitch - 1) gapWanted = pitch - 1;

  let dot = pitch - gapWanted;
  let maxDot = ledDiameter;
  if (maxDot < 1) maxDot = 1;
  if (dot > maxDot) dot = maxDot;
  if (dot < 1) dot = 1;

  const gap = pitch - dot;
  const inset = Math.floor((pitch - dot) / 2);

  // Debug logging
  console.log(`[Mirror] pitch=${pitch} dot=${dot} gap=${gap} inset=${inset} ledD=${ledDiameter} ledG=${ledGap}`);

  const base = state.ledColor >>> 0;
  const baseR = (base >> 16) & 255;
  const baseG = (base >> 8) & 255;
  const baseB = base & 255;

  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, TFT_W, TFT_H);

  const sprW = LED_W * pitch;
  const sprH = LED_H * pitch;
  const x0 = Math.floor((TFT_W - sprW) / 2);
  const y0 = Math.floor((matrixAreaH - sprH) / 2);

  // CRITICAL: The C++ framebuffer is declared as fb[LED_MATRIX_H][LED_MATRIX_W]
  // which means fb[y][x], so in linear memory it's row-major: [row0][row1][row2]...
  // Index calculation: buf[y * LED_W + x]

  let nonZeroCount = 0;
  for (let y = 0; y < LED_H; y++) {
    for (let x = 0; x < LED_W; x++) {
      const idx = y * LED_W + x;
      const v = buf[idx];
      if (!v) continue;

      nonZeroCount++;

      const r = (baseR * v / 255) | 0;
      const g = (baseG * v / 255) | 0;
      const b = (baseB * v / 255) | 0;
      ctx.fillStyle = `rgb(${r},${g},${b})`;
      ctx.fillRect(x0 + x * pitch + inset, y0 + y * pitch + inset, dot, dot);
    }
  }
  console.log(`[Render] Drew ${nonZeroCount} non-zero LEDs`);

  // Draw status bar only if STATUS_BAR_H > 0
  if (STATUS_BAR_H > 0) {
    const barY = TFT_H - STATUS_BAR_H;
    if (barY >= 0) {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, barY, TFT_W, STATUS_BAR_H);
      ctx.strokeStyle = "#2a3a52";
      ctx.beginPath();
      ctx.moveTo(0, barY + 0.5);
      ctx.lineTo(TFT_W, barY + 0.5);
      ctx.stroke();

      const status = state.wifi && state.wifi !== "DISCONNECTED"
        ? `WIFI: ${state.wifi}  IP: ${state.ip}`
        : "WIFI: AP MODE";
      ctx.fillStyle = "#8ef1ff";
      ctx.font = "14px monospace";
      ctx.textBaseline = "top";
      ctx.fillText(status, 6, barY + 6);
      ctx.fillStyle = "#c8d6e6";
      ctx.fillText(`${state.date}  LED: d${state.ledDiameter} g${state.ledGap} p${pitch} (dot${dot} gap${gap})`, 6, barY + 26);
    }
  }
}

["ledd", "ledg"].forEach((id) => {
  const el = $(id);
  el.addEventListener("input", () => dirtyInputs.add(id));
});

async function tick() {
  try {
    const state = await fetchState();
    setControls(state);
    const buf = await fetchMirror();
    renderMirror(buf, state);
  } catch (e) {
    console.warn(e);
  } finally {
    setTimeout(tick, 1000);
  }
}

tick();
