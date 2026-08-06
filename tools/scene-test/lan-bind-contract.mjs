#!/usr/bin/env node
// 리스너 LAN 바인딩 계약 검증 (플러그인 v0.1.14).
// 독립 실행형(의존성 0, node >= 18). 실행 중 sim 필요. 카메라를 하나 스폰했다 지운다.
//
// 사용: node tools/scene-test/lan-bind-contract.mjs [--host <sim의 LAN IP>] [--port 8095]
//   --host 를 생략하면 이 PC 의 비루프백 IPv4 를 자동으로 고른다(sim 이 같은 PC 에서 돌 때).
//
// 왜 따로 있나:
//   UE 의 HTTP 리스너는 포트별 ini 오버라이드가 없으면 **localhost 에만** 바인드한다. 그래서
//   "CGI 는 원격에서 안 열리는데 MJPEG(자체 FTcpListener, 항상 0.0.0.0)만 열린" 상태가 생기고,
//   **localhost 로 도는 다른 계약 테스트는 이걸 전부 통과시킨다**(2026-08-06 실제 사고).
//   이 테스트만 루프백이 아닌 주소로 접속하므로, 반드시 이걸로 판정한다.
//
// 검증 항목:
//   [1] 씬 제어 포트가 LAN 주소로 응답
//   [2] 부팅 카메라(레벨/config)의 Hucoms CGI 가 LAN 주소로 응답
//   [3] **런타임 스폰 카메라**의 CGI·MJPEG 가 LAN 주소로 응답 (이번 결함의 재현 지점)
//   [4] 삭제 후 그 포트가 닫힘

import { networkInterfaces } from "node:os";
import { Socket } from "node:net";

const args = process.argv.slice(2);
const opt = (name, dflt) => {
  const i = args.indexOf(`--${name}`);
  return i >= 0 && args[i + 1] ? args[i + 1] : dflt;
};

function autoLanHost() {
  for (const addrs of Object.values(networkInterfaces())) {
    for (const a of addrs ?? []) {
      // 링크로컬(169.254.*)은 끊긴 어댑터라 제외 — HUD 주소 표시에서 겪은 함정과 같다.
      if (a.family === "IPv4" && !a.internal && !a.address.startsWith("169.254.")) { return a.address; }
    }
  }
  return null;
}

const HOST = opt("host", autoLanHost());
const PORT = opt("port", "8095");

if (!HOST) {
  console.error("비루프백 IPv4 를 못 찾았습니다. --host <sim의 LAN IP> 로 지정하세요.");
  process.exit(2);
}
if (HOST === "127.0.0.1" || HOST === "localhost") {
  console.error("이 테스트는 루프백으로 돌리면 의미가 없습니다(그 경로는 결함이 있어도 통과합니다).");
  process.exit(2);
}

const BASE = `http://${HOST}:${PORT}`;
const CAM = { http: 8287, mjpeg: 8297 };

let failures = 0;
function check(ok, label, detail = "") {
  console.log(`  ${ok ? "OK  " : "FAIL"} ${label}${ok || !detail ? "" : ` — ${detail}`}`);
  if (!ok) failures++;
}

async function rpc(method, path, body) {
  const res = await fetch(BASE + path, {
    method,
    headers: body !== undefined ? { "content-type": "application/json" } : undefined,
    body: body !== undefined ? Buffer.from(JSON.stringify(body), "utf8") : undefined,
    signal: AbortSignal.timeout(20000),
  });
  const text = await res.text();
  let json; try { json = JSON.parse(text); } catch { json = { text }; }
  return { status: res.status, json };
}

/**
 * TCP 연결만 확인한다(응답 본문을 읽지 않음). MJPEG 처럼 끝나지 않는 스트림은 HTTP 로 읽으면
 * 영원히 안 끝나므로, 바인드 주소 판정에는 연결 성립 여부만 본다.
 */
function canConnect(port, timeoutMs = 5000) {
  return new Promise((resolve) => {
    const sock = new Socket();
    const done = (ok, error) => { sock.destroy(); resolve({ ok, error }); };
    sock.setTimeout(timeoutMs);
    sock.once("connect", () => done(true));
    sock.once("timeout", () => done(false, "timeout"));
    sock.once("error", (e) => done(false, e.code ?? e.message));
    sock.connect(port, HOST);
  });
}

/** 루프백이 아닌 주소로 접속한다. 연결 거부면 바인드 주소 결함(=이 테스트의 표적)이다. */
async function reach(port, path, timeoutMs = 15000) {
  try {
    const res = await fetch(`http://${HOST}:${port}${path}`, { signal: AbortSignal.timeout(timeoutMs) });
    return { ok: res.ok, status: res.status, bytes: (await res.arrayBuffer()).byteLength };
  } catch (e) { return { ok: false, error: e.message }; }
}

console.log(`대상 ${HOST} (루프백 아님) — 씬 :${PORT}\n`);

console.log("[1] 씬 제어 포트");
const list = await rpc("GET", "/scene/cameras");
check(list.status === 200, `GET /scene/cameras 가 ${HOST} 로 200`, `status=${list.status}`);
if (list.status !== 200) {
  console.log("\n씬 포트가 LAN 에서 안 열립니다 — 이후 항목은 판정 불가.");
  process.exit(1);
}

console.log("[2] 부팅 카메라 CGI");
const sample = (list.json.cameras ?? [])[0];
check(!!sample, "카메라 목록 비어있지 않음");
if (sample) {
  const r = await reach(sample.hucomsPort, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos");
  check(r.ok, `카메라 ${sample.id} CGI :${sample.hucomsPort} 응답`, r.error ?? `status=${r.status}`);
}

console.log("[3] 런타임 스폰 카메라 (이번 결함의 재현 지점)");
const spawn = await rpc("POST", "/scene/cameras", {
  location: { x: -200, y: 300, z: 1600 }, yawDeg: 45, pitchDeg: -30,
  httpPort: CAM.http, mjpegPort: CAM.mjpeg,
});
check(spawn.status === 200, "스폰 200", `status=${spawn.status} ${JSON.stringify(spawn.json).slice(0, 160)}`);
const spawnedId = spawn.json?.camera?.id;

if (spawnedId) {
  const ptz = await reach(CAM.http, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos");
  check(ptz.ok, `스폰 카메라 CGI :${CAM.http} 가 LAN 주소로 응답`, ptz.error ?? `status=${ptz.status}`);

  const jpg = await reach(CAM.http, "/cgi-bin/image/jpeg.cgi", 30000);
  check(jpg.ok && jpg.bytes > 10000, `스폰 카메라 스냅샷 :${CAM.http}/jpeg.cgi`, jpg.error ?? `bytes=${jpg.bytes}`);

  // MJPEG 은 플러그인 자체 FTcpListener 라 원래부터 0.0.0.0 이다 — 대조군.
  // (결함 당시에도 이쪽만 살아 있어서 "CGI 서버를 안 띄웠다"는 오진이 나왔다.)
  const mjpeg = await canConnect(CAM.mjpeg);
  check(mjpeg.ok, `스폰 카메라 MJPEG :${CAM.mjpeg} 연결(대조군)`, mjpeg.error ?? "");

  console.log("[4] 삭제 후 정리");
  const del = await rpc("DELETE", `/scene/cameras/${spawnedId}`);
  check(del.status === 200, "DELETE 200", `status=${del.status}`);
  const after = await reach(CAM.http, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos", 4000);
  check(!after.ok, `CGI :${CAM.http} 응답 중단`);
}

console.log(failures === 0 ? "\n전부 통과" : `\n실패 ${failures}건`);
process.exit(failures === 0 ? 0 : 1);
