#!/usr/bin/env python3
from __future__ import annotations

import csv
from datetime import datetime, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

FIELDNAMES = [
    "timestamp_utc",
    "client_ip",
    "t1",
    "h1",
    "p1",
    "a1",
    "uv1",
    "als1",
    "t2",
    "h2",
    "p2",
    "a2",
    "uv2",
    "als2",
    "rssi",
    "voltage",
    "current",
    "boot",
    "payload_raw",
]

HOST = "0.0.0.0"
PORT = 5000
CSV_PATH = Path(__file__).with_name("sensor_data.csv")


def ensure_csv_header() -> None:
    if not CSV_PATH.exists() or CSV_PATH.stat().st_size == 0:
        with CSV_PATH.open("w", newline="", encoding="utf-8") as file:
            writer = csv.DictWriter(file, fieldnames=FIELDNAMES)
            writer.writeheader()
        return

    with CSV_PATH.open("r", newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)
        current_header = reader.fieldnames or []
        if current_header == FIELDNAMES:
            return
        rows = list(reader)

    # Re-write old CSV with the new schema and keep existing data.
    with CSV_PATH.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=FIELDNAMES)
        writer.writeheader()
        for old_row in rows:
            new_row = {field: "" for field in FIELDNAMES}
            for key, value in old_row.items():
                if key in new_row:
                    new_row[key] = value
            writer.writerow(new_row)


def parse_payload(payload: str) -> dict[str, str]:
    parsed: dict[str, str] = {}
    if not payload:
        return parsed

    for part in payload.split(";"):
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        parsed[key.strip()] = value.strip()
    return parsed


def append_row(payload: str, client_ip: str) -> None:
    timestamp = datetime.now(timezone.utc).isoformat(timespec="seconds")
    parsed = parse_payload(payload)

    row = {key: "" for key in FIELDNAMES}
    row["timestamp_utc"] = timestamp
    row["client_ip"] = client_ip
    row["payload_raw"] = payload

    for key in FIELDNAMES:
        if key in parsed:
            row[key] = parsed[key]

    with CSV_PATH.open("a", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=FIELDNAMES)
        writer.writerow(row)


class IngestHandler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        if self.path != "/health":
            self.send_error(HTTPStatus.NOT_FOUND, "Use POST /ingest")
            return
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.end_headers()
        self.wfile.write(b"ok")

    def do_POST(self) -> None:
        if self.path != "/ingest":
            self.send_error(HTTPStatus.NOT_FOUND, "Use POST /ingest")
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(content_length)
        payload = raw.decode("utf-8", errors="replace").strip()
        client_ip = self.client_address[0]

        append_row(payload, client_ip)
        print(f"saved from {client_ip}: {payload}")

        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.end_headers()
        self.wfile.write(b"saved")

    def log_message(self, fmt: str, *args) -> None:
        # Keep console output short and focused on received payloads.
        return


def main() -> None:
    ensure_csv_header()
    server = ThreadingHTTPServer((HOST, PORT), IngestHandler)
    print(f"Listening on http://{HOST}:{PORT}")
    print(f"Writing CSV to: {CSV_PATH}")
    print("Endpoint: POST /ingest")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping server...")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
