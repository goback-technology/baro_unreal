#!/usr/bin/env node
// /scene/cameras 런타임 생명주기 + /scene/snapshot 계약 검증 (플러그인 v0.1.13).
// 독립 실행형(의존성 0, node >= 18). 실행 중 sim 필요. 씬을 reset/변경한다.
//
// 사용: node tools/scene-test/camera-snapshot-contract.mjs [--host 127.0.0.1] [--port 8095]
// 검증 항목:
//   [1] 카메라 스폰 — 명시 포트 필수·충돌 400, 스폰 즉시 CGI(jpeg/getptzfpos) 응답, pitch→tilt 이관
//   [2] 카메라 이동 — mount 반영·tilt 갱신, 레벨 저작 카메라는 403
//   [3] 카메라 삭제 — 목록 제거 + 포트 닫힘
//   [4] 스냅샷 — 저장/변형/복원 왕복: 차량 transform 완전 일치, 카메라 reconcile(이동/스폰/제거)
//   [5] 레벨 불일치 409 (force 로 통과)

const args = process.argv.slice(2);
const opt = (name, dflt) => {
  const i = args.indexOf(`--${name}`);
  return i >= 0 && args[i + 1] ? args[i + 1] : dflt;
};
const HOST = opt("host", "127.0.0.1");
const BASE = `http://${HOST}:${opt("port", "8095")}`;

// 테스트 전용 포트 — 기존 채널(8081..8086/8091..)과 겹치지 않게 높게 잡는다.
const CAM_A = { http: 8287, mjpeg: 8297 };
const CAM_B = { http: 8288, mjpeg: 8298 };

async function rpc(method, path, body) {
  const res = await fetch(BASE + path, {
    method,
    headers: body !== undefined ? { "content-type": "application/json" } : undefined,
    body: body !== undefined ? Buffer.from(JSON.stringify(body), "utf8") : undefined,
  });
  const text = await res.text();
  let json; try { json = JSON.parse(text); } catch { json = { text }; }
  return { status: res.status, json };
}

let failures = 0;
function check(ok, label, detail = "") {
  console.log(`  ${ok ? "OK  " : "FAIL"} ${label}${ok || !detail ? "" : ` — ${detail}`}`);
  if (!ok) failures++;
}
const near = (a, b, tol = 1e-6) => Math.abs(a - b) <= tol;

async function cgi(port, path) {
  try {
    const res = await fetch(`http://${HOST}:${port}${path}`, { signal: AbortSignal.timeout(15000) });
    return { ok: res.ok, buf: Buffer.from(await res.arrayBuffer()) };
  } catch (e) { return { ok: false, error: e.message }; }
}

const { json: catalog } = await rpc("GET", "/scene/catalog");
if (!catalog.pluginVersion) { console.error(`sim 무응답: ${BASE}`); process.exit(1); }
console.log(`sim: ${catalog.level} / plugin v${catalog.pluginVersion}\n`);
await rpc("POST", "/scene/reset");

// ---- [1] 스폰 ----------------------------------------------------------------------
console.log("[1] 카메라 런타임 스폰");
{
  const noPort = await rpc("POST", "/scene/cameras", { location: { x: 0, y: 0, z: 800 } });
  check(noPort.status === 400, "포트 미지정은 400", `got ${noPort.status}`);
  const scenePortClash = await rpc("POST", "/scene/cameras",
    { location: { x: 0, y: 0, z: 800 }, httpPort: 8095, mjpegPort: CAM_A.mjpeg });
  check(scenePortClash.status === 400, "씬 포트와 충돌은 400", `got ${scenePortClash.status}`);

  const { json: before } = await rpc("GET", "/scene/cameras");
  const usedPort = before.cameras[0]?.hucomsPort;
  const clash = await rpc("POST", "/scene/cameras",
    { location: { x: 0, y: 0, z: 800 }, httpPort: usedPort, mjpegPort: CAM_A.mjpeg });
  check(clash.status === 400 && /사용 중/.test(clash.json.error || ""), "기존 채널 포트 충돌은 400+원인", `got ${clash.status} ${clash.json.error ?? ""}`);

  const r = await rpc("POST", "/scene/cameras", {
    location: { x: 73, y: -2015, z: 1000 }, yawDeg: 90, pitchDeg: -30,
    httpPort: CAM_A.http, mjpegPort: CAM_A.mjpeg, note: "contract-A",
  });
  check(r.status === 200, "스폰 200", `got ${r.status} ${r.json.error ?? ""}`);
  const cam = r.json.camera;
  check(cam && cam.hucomsPort === CAM_A.http && cam.mjpegPort === CAM_A.mjpeg, "응답 포트 일치");
  check(cam && near(cam.mount.location.z, 1000) && near(cam.mount.location.y, -2015), "mount = 요청 위치(레버암 0)");

  const { json: after } = await rpc("GET", "/scene/cameras");
  const mine = after.cameras.find((c) => c.hucomsPort === CAM_A.http);
  check(!!mine && mine.spawned === true, "목록에 등장 + spawned:true");
  check(after.cameras.length === before.cameras.length + 1, "카메라 수 +1");

  const pos = await cgi(CAM_A.http, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos");
  const tilt = pos.ok ? Number((pos.buf.toString().match(/tiltpos\s*=\s*(-?\d+)/) || [])[1]) : NaN;
  check(pos.ok && tilt === 3000, "스폰 즉시 CGI 응답 + pitch -30 → tiltpos 3000", `tilt=${tilt}`);
  const jpg = await cgi(CAM_A.http, "/cgi-bin/image/jpeg.cgi");
  check(jpg.ok && jpg.buf.length > 10000 && jpg.buf[0] === 0xff && jpg.buf[1] === 0xd8,
    "스폰 카메라 실렌더 JPEG", `${jpg.buf?.length ?? 0}B`);
}

// ---- [2] 이동 ----------------------------------------------------------------------
console.log("[2] 카메라 이동");
{
  const r = await rpc("PATCH", `/scene/cameras/${CAM_A.http}`, {
    location: { x: 73, y: -2015, z: 1500 }, yawDeg: 45, pitchDeg: -45,
  });
  check(r.status === 200, "PATCH 200(포트로 지정)", `got ${r.status} ${r.json.error ?? ""}`);
  check(r.json.camera && near(r.json.camera.mount.location.z, 1500), "mount.z 1000→1500 반영");
  const pos = await cgi(CAM_A.http, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos");
  const tilt = pos.ok ? Number((pos.buf.toString().match(/tiltpos\s*=\s*(-?\d+)/) || [])[1]) : NaN;
  check(tilt === 4500, "pitch -45 → tiltpos 4500", `tilt=${tilt}`);

  const empty = await rpc("PATCH", `/scene/cameras/${CAM_A.http}`, {});
  check(empty.status === 400, "빈 PATCH 는 400", `got ${empty.status}`);

  const { json: all } = await rpc("GET", "/scene/cameras");
  const authored = all.cameras.find((c) => c.spawned === false);
  if (authored) {
    const deny = await rpc("PATCH", `/scene/cameras/${authored.id}`, { yawDeg: 10 });
    check(deny.status === 403, `레벨 저작 카메라(${authored.id}) 이동은 403`, `got ${deny.status}`);
  } else {
    check(true, "레벨 저작 카메라 없음(이 레벨엔 스폰 카메라뿐) — 403 케이스 생략");
  }
}

// ---- [4] 스냅샷 (삭제 검증 전에 — CAM_A 를 스냅샷에 포함시키기 위함) -------------------
console.log("[3] 스냅샷 저장/변형/복원 왕복");
let snapshot;
{
  await rpc("POST", "/scene/reset");
  const { json: { slots } } = await rpc("GET", "/scene/slots");
  await rpc("POST", "/scene/cars", {
    slotId: slots[0].id, carType: 3, color: 4,
    offset: { location: { y: 18 }, rotation: { yaw: 8 } },
    plate: { prefix: "111", kor: "가", number: "1111" },
  });
  await rpc("POST", "/scene/cars", { slotId: slots[3].id, carType: 9, color: 1, offset: { rotation: { yaw: 180 } } });
  await rpc("POST", "/scene/cars", {
    transform: { location: { x: -400, y: -700, z: 10 }, rotation: { pitch: 0, yaw: 33, roll: 0 } },
    offset: { location: { x: 10 } }, carType: 14, color: 7,
  });

  const g = await rpc("GET", "/scene/snapshot");
  snapshot = g.json;
  check(g.status === 200 && snapshot.cars.length === 3, "저장: 차량 3대", `${snapshot.cars?.length}`);
  const freeCar = snapshot.cars.find((c) => c.slotId === null);
  check(!!freeCar && !!freeCar.baseTransform, "자유 배치 차량엔 baseTransform 이 실린다");
  check(snapshot.cameras.some((c) => c.httpPort === CAM_A.http), "스폰 카메라(CAM_A)가 스냅샷에 포함");
  check(snapshot.level === catalog.level, "레벨 기록");

  // --- 씬을 망가뜨린다: 차 전부 삭제, CAM_A 이동, CAM_B 추가 스폰 ---
  await rpc("POST", "/scene/reset");
  await rpc("PATCH", `/scene/cameras/${CAM_A.http}`, { location: { x: 0, y: 0, z: 700 }, yawDeg: 0, pitchDeg: -10 });
  await rpc("POST", "/scene/cameras", {
    location: { x: 500, y: 500, z: 900 }, httpPort: CAM_B.http, mjpegPort: CAM_B.mjpeg, note: "contract-B",
  });

  // --- 복원 ---
  const p = await rpc("POST", "/scene/snapshot", snapshot);
  check(p.status === 200, "복원 200", `got ${p.status} ${JSON.stringify(p.json)}`);
  check(p.json.cars?.restored === 3, "차량 3대 복원", JSON.stringify(p.json.cars));
  check((p.json.failures || []).length === 0, "복원 실패 0건", JSON.stringify(p.json.failures));

  // 차량이 스냅샷과 **정확히** 같은 자리인가 (id 는 재부여되므로 transform 집합으로 비교)
  const { json: { cars: nowCars } } = await rpc("GET", "/scene/cars");
  const key = (c) => [c.slotId ?? "free",
    c.transform.location.x.toFixed(4), c.transform.location.y.toFixed(4), c.transform.location.z.toFixed(4),
    c.transform.rotation.yaw.toFixed(4), c.carType, c.color, c.plate.number].join("|");
  const wantKeys = snapshot.cars.map(key).sort();
  const gotKeys = nowCars.map(key).sort();
  check(JSON.stringify(wantKeys) === JSON.stringify(gotKeys), "차량 배치(슬롯·transform·차종·색·번호판) 완전 일치");

  // 카메라 reconcile: CAM_A 는 스냅샷 포즈로 복귀, CAM_B 는 제거
  const { json: { cameras: nowCams } } = await rpc("GET", "/scene/cameras");
  const a = nowCams.find((c) => c.hucomsPort === CAM_A.http);
  check(!!a && near(a.mount.location.z, 1500), "CAM_A 스냅샷 포즈(z=1500)로 복귀", `z=${a?.mount.location.z}`);
  check(!nowCams.some((c) => c.hucomsPort === CAM_B.http), "스냅샷에 없던 CAM_B 는 제거");
  const posA = await cgi(CAM_A.http, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos");
  const tiltA = posA.ok ? Number((posA.buf.toString().match(/tiltpos\s*=\s*(-?\d+)/) || [])[1]) : NaN;
  check(tiltA === 4500, "CAM_A tilt 도 스냅샷 값(-45 → 4500)", `tilt=${tiltA}`);
}

// ---- [5] 레벨 가드 -----------------------------------------------------------------
console.log("[4] 레벨 가드");
{
  const wrong = await rpc("POST", "/scene/snapshot", { ...snapshot, level: "LV_다른레벨" });
  check(wrong.status === 409, "레벨 불일치는 409", `got ${wrong.status}`);
  const forced = await rpc("POST", "/scene/snapshot", { ...snapshot, level: "LV_다른레벨", force: true });
  check(forced.status === 200, "force 면 강행", `got ${forced.status}`);
}

// ---- [3] 삭제 ----------------------------------------------------------------------
console.log("[5] 카메라 삭제");
{
  const { json: before } = await rpc("GET", "/scene/cameras");
  const r = await rpc("DELETE", `/scene/cameras/${CAM_A.http}`);
  check(r.status === 200 && !!r.json.removed, "DELETE 200", `got ${r.status}`);
  const { json: after } = await rpc("GET", "/scene/cameras");
  check(after.cameras.length === before.cameras.length - 1
    && !after.cameras.some((c) => c.hucomsPort === CAM_A.http), "목록에서 제거");
  const dead = await cgi(CAM_A.http, "/cgi-bin/control/ptzf_status.cgi?action=getptzfpos");
  check(!dead.ok || (dead.buf && dead.buf.length === 0), "CGI 포트 응답 중단", dead.error ?? `${dead.buf?.length}B`);

  const missing = await rpc("DELETE", `/scene/cameras/${CAM_A.http}`);
  check(missing.status === 404, "재삭제는 404", `got ${missing.status}`);

  const { json: all } = await rpc("GET", "/scene/cameras");
  const authored = all.cameras.find((c) => c.spawned === false);
  if (authored) {
    const deny = await rpc("DELETE", `/scene/cameras/${authored.id}`);
    check(deny.status === 403, "레벨 저작 카메라 삭제는 403", `got ${deny.status}`);
  }
}

await rpc("POST", "/scene/reset");
console.log(failures === 0 ? "\n전부 통과" : `\n실패 ${failures}건`);
process.exit(failures === 0 ? 0 : 1);
