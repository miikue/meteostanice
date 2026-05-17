#!/usr/bin/env python3
"""Standalone dashboard viewer for sensor_data.csv — http://localhost:5001"""
from __future__ import annotations

import csv
import json
from datetime import datetime, timedelta, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

CSV_PATH = Path(__file__).with_name("sensor_data.csv")
HOST = "0.0.0.0"
PORT = 5001
DAYS = 3


def read_rows(days: int = DAYS) -> list[dict]:
    if not CSV_PATH.exists():
        return []
    cutoff = datetime.now(timezone.utc) - timedelta(days=days)
    rows = []
    with CSV_PATH.open("r", newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            try:
                ts = datetime.fromisoformat(row["timestamp_utc"])
                if ts.tzinfo is None:
                    ts = ts.replace(tzinfo=timezone.utc)
                if ts >= cutoff:
                    rows.append(row)
            except (ValueError, KeyError):
                continue
    return rows


def data_json() -> bytes:
    rows = read_rows()
    latest = rows[-1] if rows else {}
    return json.dumps({"latest": latest, "rows": rows}, ensure_ascii=False).encode()


HTML = r"""<!DOCTYPE html>
<html lang="cs">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Meteostanice</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js@4/dist/chart.umd.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-date-fns@3/dist/chartjs-adapter-date-fns.bundle.min.js"></script>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: system-ui, sans-serif; background: #0f172a; color: #e2e8f0; padding: 1.2rem; }
  h1 { font-size: 1.25rem; color: #94a3b8; margin-bottom: .4rem; }
  .sub { font-size: .75rem; color: #475569; margin-bottom: 1.5rem; }
  .cards { display: flex; flex-wrap: wrap; gap: .6rem; margin-bottom: 2rem; }
  .card { background: #1e293b; border-radius: 10px; padding: .8rem 1rem; min-width: 105px; }
  .card .lbl { font-size: .65rem; color: #64748b; text-transform: uppercase; letter-spacing: .06em; }
  .card .val { font-size: 1.35rem; font-weight: 600; margin-top: .15rem; }
  .card .unit { font-size: .75rem; color: #94a3b8; font-weight: 400; }
  .charts { display: grid; grid-template-columns: repeat(auto-fit, minmax(480px, 1fr)); gap: 1.25rem; }
  .box { background: #1e293b; border-radius: 10px; padding: 1rem 1.2rem; }
  .box h2 { font-size: .75rem; color: #64748b; text-transform: uppercase; letter-spacing: .06em; margin-bottom: .6rem; }
  canvas { max-height: 200px; }
</style>
</head>
<body>
<h1>Meteostanice</h1>
<div class="sub" id="sub">Načítání…</div>
<div class="cards" id="cards"></div>
<div class="charts">
  <div class="box"><h2>Teplota</h2><canvas id="cTemp"></canvas></div>
  <div class="box"><h2>Světlo (ALS + UV)</h2><canvas id="cLight"></canvas></div>
  <div class="box"><h2>Napětí</h2><canvas id="cVolt"></canvas></div>
  <div class="box"><h2>Proud solárního panelu</h2><canvas id="cCurr"></canvas></div>
</div>

<script>
const META = {
  t1:      ["Teplota 1",   "°C",   "#f97316"],
  t2:      ["Teplota 2",   "°C",   "#38bdf8"],
  h1:      ["Vlhkost 1",   "%",    "#34d399"],
  h2:      ["Vlhkost 2",   "%",    "#6ee7b7"],
  p1:      ["Tlak 1",      "hPa",  "#a78bfa"],
  p2:      ["Tlak 2",      "hPa",  "#c4b5fd"],
  voltage: ["Baterie",     "V",    "#4ade80"],
  panel_v: ["Panel V",     "V",    "#facc15"],
  panel_i: ["Panel mA",    "mA",   "#fb923c"],
  rssi:    ["RSSI",        "dBm",  "#94a3b8"],
  uv1:     ["UV 1",        "",     "#e879f9"],
  als1:    ["ALS 1",       "lux",  "#f0abfc"],
  uv2:     ["UV 2",        "",     "#d946ef"],
  als2:    ["ALS 2",       "lux",  "#c026d3"],
};

const AXIS_OPTS = {
  x: { type:"time", time:{ tooltipFormat:"dd.MM HH:mm" }, ticks:{ color:"#94a3b8", maxRotation:0 }, grid:{ color:"#1e3a5f" } },
  y: { ticks:{ color:"#94a3b8" }, grid:{ color:"#1e3a5f" } },
};

function ds(rows, field, label, color) {
  return {
    label,
    data: rows.filter(r => r[field] !== "").map(r => ({ x: new Date(r.timestamp_utc), y: +r[field] })),
    borderColor: color, backgroundColor: color + "18",
    borderWidth: 1.5, pointRadius: 0, tension: 0.3,
  };
}

function mkChart(id, datasets) {
  return new Chart(document.getElementById(id), {
    type: "line",
    data: { datasets },
    options: {
      animation: false, parsing: false,
      interaction: { mode:"index", intersect:false },
      scales: JSON.parse(JSON.stringify(AXIS_OPTS)),
      plugins: { legend: { labels: { color:"#cbd5e1", boxWidth:10, padding:12 } } },
    },
  });
}

let charts = {};

function renderCards(latest) {
  document.getElementById("cards").innerHTML = Object.entries(META)
    .filter(([k]) => latest[k] !== undefined && latest[k] !== "")
    .map(([k, [lbl, unit, color]]) => {
      const v = parseFloat(latest[k]);
      const fmt = Number.isInteger(v) ? v : v.toFixed(2).replace(/\.?0+$/, "");
      return `<div class="card">
        <div class="lbl">${lbl}</div>
        <div class="val" style="color:${color}">${fmt}<span class="unit"> ${unit}</span></div>
      </div>`;
    }).join("");
}

function renderCharts(rows) {
  Object.values(charts).forEach(c => c.destroy());
  charts.temp  = mkChart("cTemp",  [ds(rows,"t1","Teplota 1","#f97316"), ds(rows,"t2","Teplota 2","#38bdf8")]);
  charts.light = mkChart("cLight", [ds(rows,"als1","ALS 1","#f0abfc"), ds(rows,"als2","ALS 2","#c026d3"), ds(rows,"uv1","UV 1","#a78bfa"), ds(rows,"uv2","UV 2","#7c3aed")]);
  charts.volt  = mkChart("cVolt",  [ds(rows,"voltage","Baterie V","#4ade80"), ds(rows,"panel_v","Panel V","#facc15")]);
  charts.curr  = mkChart("cCurr",  [ds(rows,"panel_i","Panel mA","#fb923c")]);
}

async function refresh() {
  const res = await fetch("/data").catch(() => null);
  if (!res || !res.ok) { document.getElementById("sub").textContent = "Chyba načítání dat"; return; }
  const { latest, rows } = await res.json();
  renderCards(latest);
  renderCharts(rows);
  const ts = latest?.timestamp_utc ? new Date(latest.timestamp_utc).toLocaleString("cs-CZ") : "–";
  document.getElementById("sub").textContent =
    `Poslední měření: ${ts} · ${rows.length} záznamů (posl. 3 dny) · obnovuje se každých 60 s`;
}

refresh();
setInterval(refresh, 60_000);
</script>
</body>
</html>"""


class ViewerHandler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if self.path == "/data":
            body = data_json()
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif self.path in ("/", "/index.html"):
            body = HTML.encode()
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_error(HTTPStatus.NOT_FOUND)

    def log_message(self, fmt: str, *args) -> None:
        return


def main() -> None:
    if not CSV_PATH.exists():
        print(f"WARN: CSV nenalezeno na {CSV_PATH}")
    server = ThreadingHTTPServer((HOST, PORT), ViewerHandler)
    print(f"Dashboard: http://localhost:{PORT}")
    print(f"CSV:       {CSV_PATH}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nKonec.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
