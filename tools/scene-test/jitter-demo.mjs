#!/usr/bin/env node
// 주차면을 채우되 차마다 조금씩 다르게 세우는 데모 씬 — offset 기능을 눈으로 확인하는 용도.
// 독립 실행형(의존성 0, node >= 18). 소비 저장소를 건드리지 않는다.
//
// 사용: node tools/scene-test/jitter-demo.mjs [jitter|straight] [--host 127.0.0.1] [--port 8095]
//   jitter   변형 배치 — 좌우/앞뒤로 비껴, ±9도 틀고, 다섯에 하나는 180도 반대로 주차
//   straight 대조군 — 같은 자리·같은 차종·같은 색, offset 만 전부 항등
// 차량 선택과 변형은 시드 고정 난수의 **분리된 두 스트림**이라, 두 모드의 차이는 offset 뿐이다
// (한 스트림이면 jitter 쪽이 난수를 더 뽑아 차종·빈자리까지 달라져 A/B 비교가 깨진다).
// 끝에 주차면이 가장 많이 잡히는 카메라의 jpeg.cgi URL 을 찍어 준다 — 그걸로 스냅샷을 보라.
// 주의: 기존 스폰 차량을 reset 으로 지운다.

const args = process.argv.slice(2);
const MODE = args[0] && !args[0].startsWith("--") ? args[0] : "jitter";
const opt = (name, dflt) => {
  const i = args.indexOf(`--${name}`);
  return i >= 0 && args[i + 1] ? args[i + 1] : dflt;
};
const BASE = `http://${opt("host", "127.0.0.1")}:${opt("port", "8095")}`;

async function rpc(method, path, body) {
  const res = await fetch(BASE + path, {
    method,
    headers: body !== undefined ? { "content-type": "application/json" } : undefined,
    body: body !== undefined ? Buffer.from(JSON.stringify(body), "utf8") : undefined,
  });
  const text = await res.text();
  try { return { status: res.status, json: JSON.parse(text) }; } catch { return { status: res.status, json: { text } }; }
}

const makeRand = (s) => () => { s = (s * 1103515245 + 12345) & 0x7fffffff; return s / 0x7fffffff; };
const pick = makeRand(20260803);   // 어느 자리에 어떤 차 — 두 모드에서 동일
const jit = makeRand(77777);       // 얼마나 비뚤게 — jitter 모드만 소비
const between = (a, b) => a + (b - a) * jit();

const { json: catalog } = await rpc("GET", "/scene/catalog");
if (!catalog.carCount) { console.error(`sim 무응답: ${BASE}`); process.exit(1); }

await rpc("POST", "/scene/reset");
const { json: { slots } } = await rpc("GET", "/scene/slots");
const { json: { cameras } } = await rpc("GET", "/scene/cameras");

let spawned = 0, flipped = 0;
for (const slot of slots) {
  const skip = pick() < 0.18;                                  // 빈 자리도 남긴다
  const body = { slotId: slot.id, carType: Math.floor(pick() * catalog.carCount), color: Math.floor(pick() * 8) };
  if (skip) continue;
  if (MODE === "jitter") {
    const flip = jit() < 0.2;                                  // 다섯에 하나는 반대로 주차
    if (flip) flipped++;
    body.offset = {
      location: { x: between(-25, 25), y: between(-22, 22), z: 0 },
      rotation: { pitch: 0, yaw: (flip ? 180 : 0) + between(-9, 9), roll: 0 },
    };
  }
  const r = await rpc("POST", "/scene/cars", body);
  if (r.status === 200) spawned++;
  else console.log(`  ${slot.id} 실패 ${r.status}: ${r.json.error ?? ""}`);
}
console.log(`${MODE}: ${spawned}/${slots.length}면 배치, 반대 주차 ${flipped}대`);

// 주차면이 프레임에 가장 많이 잡히는 카메라를 골라 스냅샷 URL 로 안내
const points = slots.map((s) => s.transform.location);
let best = null;
for (const cam of cameras) {
  const { json } = await rpc("POST", "/scene/project", { cameraId: cam.id, points });
  const visible = (json.points || []).filter((p) => p.visible).length;
  if (!best || visible > best.visible) best = { cam, visible };
}
if (best) {
  console.log(`스냅샷: ${best.cam.id} (주차면 ${best.visible}/${slots.length} 프레임 안)`);
  console.log(`  curl -o scene.jpg http://127.0.0.1:${best.cam.hucomsPort}/cgi-bin/image/jpeg.cgi`);
}
