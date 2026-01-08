/**
 * CYD RGB LED Matrix (HUB75) Retro Clock - Web Interface JavaScript
 *
 * Features:
 * - Live display mirror rendering with RGB LED Matrix (HUB75) emulation
 * - Real-time system state polling (1-second interval)
 * - Instant auto-apply for all configuration changes
 * - Timezone dropdown with 88 options across 13 regions
 * - NTP server dropdown with 9 preset servers
 * - Date format selection (5 formats)
 * - Debug level runtime adjustment
 * - System diagnostics with formatted uptime and memory usage (includes firmware version)
 * - Color picker with dirty input tracking to prevent override
 * - Human-readable formatting utilities
 */

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

const dirtyInputs = new Set();  // Tracks user-modified fields to prevent override
let timezonesLoaded = false;

function rgbFromHex(hex) {
  const v = parseInt(hex.replace("#",""), 16);
  return { r: (v>>16)&255, g: (v>>8)&255, b: v&255 };
}

async function fetchState() {
  const r = await fetch("/api/state", { cache: "no-store" });
  return r.json();
}

async function fetchTimezones() {
  const r = await fetch("/api/timezones", { cache: "no-store" });
  return r.json();
}

async function populateTimezones() {
  if (timezonesLoaded) return;

  try {
    const data = await fetchTimezones();
    const select = $("tz");
    select.innerHTML = "";  // Clear loading message

    // Populate with optgroups for each region
    data.regions.forEach(region => {
      const optgroup = document.createElement("optgroup");
      optgroup.label = region.name;

      region.timezones.forEach(tz => {
        const option = document.createElement("option");
        option.value = tz.name;
        option.textContent = tz.name;
        optgroup.appendChild(option);
      });

      select.appendChild(optgroup);
    });

    timezonesLoaded = true;
    console.log(`Loaded ${data.count} timezones in ${data.regions.length} regions`);
  } catch (e) {
    console.error("Failed to load timezones:", e);
    $("tz").innerHTML = '<option value="">Failed to load timezones</option>';
  }
}

function setMsg(s, ok=true) {
  const el = $("msg");
  el.textContent = s;
  el.style.opacity = "0.9";
  el.style.color = ok ? "#b6f2c2" : "#ffb3b3";
  setTimeout(()=>{ el.style.opacity = "0.6"; }, 2500);
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;

  if (days > 0) return `${days}d ${hours}h ${mins}m`;
  if (hours > 0) return `${hours}h ${mins}m ${secs}s`;
  if (mins > 0) return `${mins}m ${secs}s`;
  return `${secs}s`;
}

function formatBytes(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

async function setControls(state) {
  // Time & Network
  $("time").textContent = state.time;
  $("date").textContent = state.date;
  $("wifi").textContent = state.wifi;
  $("ip").textContent = state.ip;

  // Hardware (static, but included for completeness)
  if (state.board) $("board").textContent = state.board;
  if (state.display) $("display").textContent = state.display;
  if (state.sensors) $("sensors").textContent = state.sensors;
  if (state.firmware) {
    $("firmware").textContent = state.firmware;
  }
  if (state.otaEnabled !== undefined) {
    $("ota").textContent = state.otaEnabled ? "Enabled" : "Disabled";
  }

  // System Resources
  if (state.uptime !== undefined) {
    $("uptime").textContent = formatUptime(state.uptime);
  }
  if (state.freeHeap !== undefined) {
    $("freeHeap").textContent = formatBytes(state.freeHeap);
  }
  if (state.heapSize !== undefined && state.freeHeap !== undefined) {
    const usedHeap = state.heapSize - state.freeHeap;
    const usagePercent = ((usedHeap / state.heapSize) * 100).toFixed(1);
    $("heapUsage").textContent = `${formatBytes(usedHeap)} / ${formatBytes(state.heapSize)} (${usagePercent}%)`;
  }
  if (state.cpuFreq !== undefined) {
    $("cpuFreq").textContent = `${state.cpuFreq} MHz`;
  }

  // Debug Level
  if (document.activeElement !== $("debugLevel") && state.debugLevel !== undefined) {
    $("debugLevel").value = String(state.debugLevel);
  }

  // Load timezones on first run
  await populateTimezones();

  // Config fields
  if (document.activeElement !== $("tz")) $("tz").value = state.tz || "";
  if (document.activeElement !== $("ntp")) $("ntp").value = state.ntp || "";
  if (document.activeElement !== $("use24h")) $("use24h").value = String(state.use24h);
  if (document.activeElement !== $("dateFormat")) $("dateFormat").value = String(state.dateFormat || 0);

  if (!dirtyInputs.has("ledd")) $("ledd").value = state.ledDiameter;
  if (!dirtyInputs.has("ledg")) $("ledg").value = state.ledGap;

  // Don't update color picker if user is actively selecting or has made changes
  if (document.activeElement !== $("col") && !dirtyInputs.has("col")) {
    const col = (state.ledColor >>> 0).toString(16).padStart(6,"0");
    $("col").value = "#" + col;
  }

  if (!dirtyInputs.has("bl")) $("bl").value = state.brightness;

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
  const dateFormat = parseInt($("dateFormat").value, 10) || 0;

  const ledDiameterRaw = parseInt($("ledd").value, 10);
  const ledGapRaw = parseInt($("ledg").value, 10);
  const brightness = parseInt($("bl").value, 10);
  const debugLevel = parseInt($("debugLevel").value, 10);

  const { r, g, b } = rgbFromHex($("col").value);
  const ledColor = (r<<16) | (g<<8) | b;

  const ledDiameter = Number.isFinite(ledDiameterRaw) ? ledDiameterRaw : state.ledDiameter;
  const ledGap = Number.isFinite(ledGapRaw) ? ledGapRaw : state.ledGap;
  const payload = { tz, ntp, use24h, dateFormat, ledDiameter, ledGap, ledColor, brightness, debugLevel };

  const res = await fetch("/api/config", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload)
  });

  if (!res.ok) {
    setMsg("Save failed: " + (await res.text()), false);
    return;
  }
  setMsg("Saved");
  dirtyInputs.clear();
}

$("save").addEventListener("click", () => {
  saveConfig().catch(e => setMsg(String(e), false));
});

// Flip display button handler
$("flipBtn").addEventListener("click", async () => {
  try {
    // Fetch current state to get current flip setting
    const currentState = await fetchState();
    const currentFlip = currentState.flipDisplay || false;
    const newFlip = !currentFlip;

    const res = await fetch("/api/config", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ flipDisplay: newFlip })
    });

    if (!res.ok) {
      setMsg("Flip failed: " + (await res.text()), false);
      return;
    }

    setMsg(newFlip ? "Display flipped 180°" : "Display orientation normal");
  } catch (e) {
    setMsg(String(e), false);
  }
});

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

  // Apply display flip transform if needed
  ctx.save();  // Save current context state
  if (state.flipDisplay) {
    // Rotate 180° around center for flipped display
    ctx.translate(TFT_W, TFT_H);
    ctx.rotate(Math.PI);
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

      const ipStatus = state.wifi && state.wifi !== "DISCONNECTED"
        ? `IP: ${state.ip}`
        : "IP: Not connected (AP mode)";
      ctx.fillStyle = "#8ef1ff";
      ctx.font = "14px monospace";
      ctx.textBaseline = "top";
      ctx.fillText(ipStatus, 6, barY + 6);
      ctx.fillStyle = "#c8d6e6";
      ctx.fillText(`${state.date}  ${state.tz}`, 6, barY + 26);
    }
  }

  // Restore context state (removes flip transform if applied)
  ctx.restore();
}

// Auto-apply on any config field change (instant feedback)
["tz", "ntp", "use24h", "dateFormat", "ledd", "ledg", "col", "bl", "debugLevel"].forEach((id) => {
  const el = $(id);

  // Immediate save on change for dropdowns and text inputs
  el.addEventListener("change", () => {
    dirtyInputs.add(id);
    saveConfig().catch(e => setMsg(String(e), false));
  });

  // For number/color inputs, also apply on input (real-time updates as you drag/type)
  if (["ledd", "ledg", "col", "bl"].includes(id)) {
    el.addEventListener("input", () => {
      dirtyInputs.add(id);
      saveConfig().catch(e => setMsg(String(e), false));
    });
  }
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
