#!/usr/bin/env node
// /scene/cars 배치 변형(offset) 계약 검증 — 실행 중인 sim 을 상대로 수치·동작을 전부 대조한다.
//
// 독립 실행형이다: 의존성 0, node >= 18 만 필요. baro_calory 등 소비 저장소를 일절 건드리지
// 않고 시뮬레이터 계약만 검증한다(2026-08-03 이교수님 확정 — 시뮬 업그레이드 검증은 소비
// 저장소 수정이 아니라 별도 테스트 프로그램으로).
//
// 사용: node tools/scene-test/offset-contract.mjs [--host 127.0.0.1] [--port 8095]
// 전제: sim 이 standalone -game 으로 떠 있고 /scene/* 이 살아 있을 것. 씬을 reset 하므로
//       보존할 배치가 있으면 돌리지 말 것. 성공 시 exit 0, 하나라도 어긋나면 exit 1.
//
// 무엇을 검증하나:
//   1. 수치 — 최종 transform == Offset * Base (UE FTransform 곱). 아래 compose* 가 UE 의
//      FRotator↔FQuat 규약을 그대로 옮긴 참조 구현이고, sim 실응답과 비교한다.
//      짐벌 락(피치 ±90, 임계 SingularityTest > 0.4999995)에서 UE 가 roll 을 yaw 로 접는
//      규약까지 포함(2026-08-03 실기동 역산으로 확정).
//   2. 동작 — slotId+transform 동시 400 / offset 에코 원값 / PATCH 멱등 / 주차면 이동 시
//      변형 유지 / 배치 무관 PATCH 무이동 / offset 없는 옛 호출 무변화.

const args = process.argv.slice(2);
const opt = (name, dflt) => {
  const i = args.indexOf(`--${name}`);
  return i >= 0 && args[i + 1] ? args[i + 1] : dflt;
};
const BASE = `http://${opt("host", "127.0.0.1")}:${opt("port", "8095")}`;

// ---- UE 회전 규약 참조 구현 -------------------------------------------------------
const DEG = Math.PI / 180, RAD = 180 / Math.PI;
const SINGULARITY = 0.4999995; // UE FQuat::Rotator 짐벌 판정 임계(피치 약 89.94°부터)

function normalizeAxis(deg) {
  let a = deg % 360;
  if (a < 0) a += 360;
  return a > 180 ? a - 360 : a;
}

// UE FRotator::Quaternion()
function rotatorToQuat(r = {}) {
  const hp = (r.pitch || 0) * DEG / 2, hy = (r.yaw || 0) * DEG / 2, hr = (r.roll || 0) * DEG / 2;
  const sp = Math.sin(hp), cp = Math.cos(hp), sy = Math.sin(hy), cy = Math.cos(hy), sr = Math.sin(hr), cr = Math.cos(hr);
  return {
    x: cr * sp * sy - sr * cp * cy,
    y: -cr * sp * cy - sr * cp * sy,
    z: cr * cp * sy - sr * sp * cy,
    w: cr * cp * cy + sr * sp * sy,
  };
}

// UE FQuat::operator* — B 먼저, A 나중(오른쪽 먼저)
function quatMul(a, b) {
  return {
    x: a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
    y: a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
    z: a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
    w: a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
  };
}

// UE FQuat::RotateVector
function rotateVec(q, v) {
  const tx = 2 * (q.y * v.z - q.z * v.y), ty = 2 * (q.z * v.x - q.x * v.z), tz = 2 * (q.x * v.y - q.y * v.x);
  return {
    x: v.x + q.w * tx + (q.y * tz - q.z * ty),
    y: v.y + q.w * ty + (q.z * tx - q.x * tz),
    z: v.z + q.w * tz + (q.x * ty - q.y * tx),
  };
}

// UE FQuat::Rotator — 짐벌 락에서는 yaw 일반식(YawY/YawX)이 0/0 이라 잡음이다.
// UE 는 roll 을 yaw 로 접고(roll=0) yaw = ±2*atan2(X,W) 를 쓴다.
function quatToRotator(q) {
  const sing = q.z * q.x - q.w * q.y;
  if (sing < -SINGULARITY) return { pitch: -90, yaw: normalizeAxis(-2 * Math.atan2(q.x, q.w) * RAD), roll: 0 };
  if (sing > SINGULARITY) return { pitch: 90, yaw: normalizeAxis(2 * Math.atan2(q.x, q.w) * RAD), roll: 0 };
  return {
    pitch: Math.asin(Math.min(1, Math.max(-1, 2 * sing))) * RAD,
    yaw: Math.atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z)) * RAD,
    roll: Math.atan2(-2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y)) * RAD,
  };
}

// 최종 배치 = Offset * Base (UE FTransform 곱과 동일)
function composePlacement(base, offset) {
  const bq = rotatorToQuat(base?.rotation);
  const bl = base?.location || {};
  const moved = rotateVec(bq, {
    x: offset?.location?.x || 0, y: offset?.location?.y || 0, z: offset?.location?.z || 0,
  });
  return {
    location: { x: (bl.x || 0) + moved.x, y: (bl.y || 0) + moved.y, z: (bl.z || 0) + moved.z },
    rotation: quatToRotator(quatMul(bq, rotatorToQuat(offset?.rotation))),
  };
}

// ---- RPC ---------------------------------------------------------------------------
async function rpc(method, path, body) {
  const res = await fetch(BASE + path, {
    method,
    headers: body !== undefined ? { "content-type": "application/json" } : undefined,
    // 한글 바디는 Buffer 로 — 문자열 직전달은 Windows curl 계열에서 깨진 전례가 있다.
    body: body !== undefined ? Buffer.from(JSON.stringify(body), "utf8") : undefined,
  });
  const text = await res.text();
  let json; try { json = JSON.parse(text); } catch { json = { text }; }
  return { status: res.status, json };
}

// ---- 판정 헬퍼 ---------------------------------------------------------------------
let failures = 0;
function check(ok, label, detail = "") {
  console.log(`  ${ok ? "OK  " : "FAIL"} ${label}${ok || !detail ? "" : ` — ${detail}`}`);
  if (!ok) failures++;
}
const angleDiff = (a, b) => Math.abs(normalizeAxis(a - b));
const locDiff = (a, b) => Math.max(Math.abs(a.x - b.x), Math.abs(a.y - b.y), Math.abs(a.z - b.z));
const rotDiff = (a, b) => Math.max(angleDiff(a.pitch, b.pitch), angleDiff(a.yaw, b.yaw), angleDiff(a.roll, b.roll));

const off = (loc = {}, rot = {}) => ({
  location: { x: 0, y: 0, z: 0, ...loc },
  rotation: { pitch: 0, yaw: 0, roll: 0, ...rot },
});

// ---- 본 검증 -----------------------------------------------------------------------
const { json: catalog } = await rpc("GET", "/scene/catalog");
if (!catalog.pluginVersion) { console.error(`sim 무응답 또는 씬 API 아님: ${BASE}`); process.exit(1); }
console.log(`sim: ${catalog.level} / plugin v${catalog.pluginVersion} @ ${BASE}\n`);

await rpc("POST", "/scene/reset");
const { json: { slots } } = await rpc("GET", "/scene/slots");
// 방위가 축 정렬이 아닌 주차면이어야 "월드축 기준"과 "주차면축 기준"이 구별된다.
const slot = slots.find((s) => Math.abs(normalizeAxis(s.transform.rotation.yaw)) > 1);
if (!slot) { console.error("기울어진 주차면이 없어 곱 순서를 판별할 수 없음"); process.exit(1); }
const other = slots.find((s) => s.id !== slot.id);
console.log(`기준 주차면: ${slot.id} (yaw ${slot.transform.rotation.yaw.toFixed(2)})\n[1] 수치 — transform == Offset * Base`);

const POSES = [
  ["항등", off()],
  ["좌우 +18cm", off({ y: 18 })],
  ["앞뒤 -35 + 좌우 18 + yaw 10", off({ x: -35, y: 18 }, { yaw: 10 })],
  ["180도 반대", off({}, { yaw: 180 })],
  ["다축 p7/y33/r-4 + 이동", off({ x: 12, y: -8, z: 5 }, { pitch: 7, yaw: 33, roll: -4 })],
  ["짐벌 임계 밖 p89.5", off({}, { pitch: 89.5, yaw: 45, roll: 20 })],
  ["짐벌 임계 밖 p89.9", off({}, { pitch: 89.9, yaw: 45, roll: 20 })],
  ["짐벌 임계 안 p89.99", off({}, { pitch: 89.99, yaw: 45, roll: 20 })],
  ["짐벌 정점 p90", off({}, { pitch: 90, yaw: 45, roll: 20 })],
  ["짐벌 정점 p-90", off({}, { pitch: -90, yaw: -120, roll: 33 })],
];

let worstLoc = 0, worstRot = 0;
for (const [name, offset] of POSES) {
  const r = await rpc("POST", "/scene/cars", { slotId: slot.id, carType: 3, color: 4, offset, force: true });
  if (r.status !== 200) { check(false, name, `HTTP ${r.status}`); continue; }
  const want = composePlacement(slot.transform, offset);
  const dL = locDiff(r.json.car.transform.location, want.location);
  const dR = rotDiff(r.json.car.transform.rotation, want.rotation);
  const echoed = JSON.stringify(r.json.car.offset) === JSON.stringify(offset);
  worstLoc = Math.max(worstLoc, dL); worstRot = Math.max(worstRot, dR);
  check(dL < 1e-6 && dR < 1e-6 && echoed, name,
    `loc ${dL.toExponential(2)}cm rot ${dR.toExponential(2)}deg echo=${echoed}`);
}
console.log(`  최대 오차: 위치 ${worstLoc.toExponential(2)}cm / 회전 ${worstRot.toExponential(2)}deg`);

console.log("[2] 동작 계약");
{
  const r = await rpc("POST", "/scene/cars", {
    slotId: slot.id, carType: 0, color: 0, force: true,
    transform: { location: { x: 0, y: 0, z: 10 }, rotation: { pitch: 0, yaw: 0, roll: 0 } },
  });
  check(r.status === 400, "slotId+transform 동시 지정은 400", `got ${r.status}`);
}
await rpc("POST", "/scene/reset");
{
  const nudge = off({ y: 18 });
  const spawned = (await rpc("POST", "/scene/cars", { slotId: slot.id, carType: 3, color: 4, offset: nudge })).json.car;
  const again = (await rpc("PATCH", `/scene/cars/${spawned.id}`, { offset: nudge })).json.car;
  check(JSON.stringify(again.transform) === JSON.stringify(spawned.transform),
    "같은 offset PATCH 재전송은 멱등(누적 델타 아님)");

  const moved = (await rpc("PATCH", `/scene/cars/${spawned.id}`, { slotId: other.id })).json.car;
  const wantMoved = composePlacement(other.transform, nudge);
  check(moved.slotId === other.id && locDiff(moved.transform.location, wantMoved.location) < 1e-6,
    "주차면 이동 시 변형이 따라간다");

  const recolored = (await rpc("PATCH", `/scene/cars/${spawned.id}`, { color: 1 })).json.car;
  check(JSON.stringify(recolored.transform) === JSON.stringify(moved.transform),
    "배치 무관 PATCH 는 차를 움직이지 않는다");

  const legacy = (await rpc("POST", "/scene/cars", { slotId: slot.id, carType: 1, color: 2, force: true })).json.car;
  check(locDiff(legacy.transform.location, slot.transform.location) < 1e-6
    && rotDiff(legacy.transform.rotation, slot.transform.rotation) < 1e-6,
    "offset 없는 옛 호출은 주차면 트랜스폼 그대로");
}
await rpc("POST", "/scene/reset");

console.log(failures === 0 ? "\n전부 통과" : `\n실패 ${failures}건`);
process.exit(failures === 0 ? 0 : 1);
