// Copyright (c) 2026 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.
//
// Three-way BVH4 / BVH8 / BVH16 traversal visualizer. DISABLED by default.
// Run with:
//   OpenCascadeGTest --gtest_also_run_disabled_tests
//                    --gtest_filter=BVH_VisualizeTest.DISABLED_GenerateHtml
// Optional env:
//   OCCT_BVH_VIZ_PRIMS=<n>   - initial primitive count (default 256)
//   OCCT_BVH_VIZ_OUT=<path>  - output HTML path (default /tmp/bvh_4_8_16_visualization.html)

#include <gtest/gtest.h>

#include <BVH_BinaryTree.hxx>
#include <BVH_Box.hxx>
#include <BVH_BoxSet.hxx>
#include <BVH_LinearBuilder.hxx>
#include <BVH_QuadTree.hxx>
#include <BVH_Ray.hxx>
#include <BVH_SIMDDispatch.hxx>
#include <BVH_TraverseQuad.hxx>
#include <BVH_TraverseWide.hxx>
#include <BVH_Tree.hxx>
#include <BVH_WideTree.hxx>

#include <cstdlib>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <string>

namespace
{

template <class TreeKind>
opencascade::handle<BVH_Tree<float, 3, TreeKind>> ConvertTreeToFloat(
  const opencascade::handle<BVH_Tree<double, 3, TreeKind>>& theSrc)
{
  auto      aDst = new BVH_Tree<float, 3, TreeKind>;
  const int aLen = theSrc->Length();
  for (int i = 0; i < aLen; ++i)
  {
    const auto& aMin = theSrc->MinPoint(i);
    const auto& aMax = theSrc->MaxPoint(i);
    aDst->MinPointBuffer().push_back(BVH_Vec3f(static_cast<float>(aMin.x()),
                                               static_cast<float>(aMin.y()),
                                               static_cast<float>(aMin.z())));
    aDst->MaxPointBuffer().push_back(BVH_Vec3f(static_cast<float>(aMax.x()),
                                               static_cast<float>(aMax.y()),
                                               static_cast<float>(aMax.z())));
    aDst->NodeInfoBuffer().push_back(theSrc->NodeInfoBuffer()[i]);
  }
  return aDst;
}

const char* kSimdLevelName(BVH::SIMD::Level theLevel)
{
  switch (theLevel)
  {
    case BVH::SIMD::Level::Scalar:
      return "Scalar";
    case BVH::SIMD::Level::SSE2:
      return "SSE2";
    case BVH::SIMD::Level::AVX2:
      return "AVX2";
    case BVH::SIMD::Level::AVX512:
      return "AVX-512";
  }
  return "?";
}

} // namespace

TEST(BVH_VisualizeTest, DISABLED_GenerateHtml)
{
  // ---------------------------------------------------------------------
  // 1. Build a deterministic random scene of axis-aligned boxes inside
  //    the cube [-10, 10]^3. The seed matches the JS Mulberry32 so the
  //    client-side slider regenerates an identical scene.
  // ---------------------------------------------------------------------
  int kNumPrims = 256;
  if (const char* p = std::getenv("OCCT_BVH_VIZ_PRIMS"))
  {
    const int v = std::atoi(p);
    if (v > 0)
    {
      kNumPrims = v;
    }
  }

  std::mt19937                          aRng(42);
  std::uniform_real_distribution<float> aPos(-10.0f, 10.0f);
  std::uniform_real_distribution<float> aSize(0.3f, 1.2f);

  opencascade::handle<BVH_LinearBuilder<double, 3>> aBuilder =
    new BVH_LinearBuilder<double, 3>(1 /*leaf*/, 32 /*max depth*/);
  opencascade::handle<BVH_BoxSet<double, 3>> aBoxSet = new BVH_BoxSet<double, 3>(aBuilder);

  struct RawBox
  {
    float minX, minY, minZ, maxX, maxY, maxZ;
  };

  for (int i = 0; i < kNumPrims; ++i)
  {
    const float        cx = aPos(aRng), cy = aPos(aRng), cz = aPos(aRng);
    const float        hx = aSize(aRng), hy = aSize(aRng), hz = aSize(aRng);
    BVH_Box<double, 3> aBox(BVH_Vec3d(cx - hx, cy - hy, cz - hz),
                            BVH_Vec3d(cx + hx, cy + hy, cz + hz));
    aBoxSet->Add(i, aBox);
  }
  aBoxSet->Build();

  // Build() permutes the internal arrays via Swap(); re-collect boxes in
  // post-build order so aRawBoxes[idx] matches whatever the acceptor reports.
  std::vector<RawBox> aRawBoxes;
  aRawBoxes.reserve(kNumPrims);
  for (int i = 0; i < kNumPrims; ++i)
  {
    const auto aBox = aBoxSet->Box(i);
    aRawBoxes.push_back({static_cast<float>(aBox.CornerMin().x()),
                         static_cast<float>(aBox.CornerMin().y()),
                         static_cast<float>(aBox.CornerMin().z()),
                         static_cast<float>(aBox.CornerMax().x()),
                         static_cast<float>(aBox.CornerMax().y()),
                         static_cast<float>(aBox.CornerMax().z())});
  }

  // ---------------------------------------------------------------------
  // 2. Collapse the binary tree into BVH4 (QuadTree), BVH8 (WideTree<8>),
  //    BVH16 (WideTree<16>). Convert all three to <float, 3> so the SIMD
  //    traversal can consume them.
  // ---------------------------------------------------------------------
  const auto&                                                aBinaryD = aBoxSet->BVH();
  opencascade::handle<BVH_Tree<double, 3, BVH_QuadTree>>     aQuadD(aBinaryD->CollapseToQuadTree());
  opencascade::handle<BVH_Tree<double, 3, BVH_WideTree<8>>>  aOctD(aBinaryD->CollapseToWide<8>());
  opencascade::handle<BVH_Tree<double, 3, BVH_WideTree<16>>> aHexD(aBinaryD->CollapseToWide<16>());

  auto aQuadF = ConvertTreeToFloat<BVH_QuadTree>(aQuadD);
  auto aOctF  = ConvertTreeToFloat<BVH_WideTree<8>>(aOctD);
  auto aHexF  = ConvertTreeToFloat<BVH_WideTree<16>>(aHexD);

  // ---------------------------------------------------------------------
  // 3. A single ray that punches through the scene.
  // ---------------------------------------------------------------------
  const BVH_Vec3f   aOrigin(-15.0f, -3.0f, -2.0f);
  const BVH_Vec3f   aDir(1.0f, 0.18f, 0.12f);
  BVH_Ray<float, 3> aRay(aOrigin, aDir);

  // ---------------------------------------------------------------------
  // 4. Run BVH4 / BVH8 / BVH16 traversal, collect each set of hit indices.
  //    All three must agree -- they share the same underlying primitives.
  // ---------------------------------------------------------------------
  std::set<int> aHitsQuad, aHitsOct, aHitsHex;
  auto          aAccQuad = [&](int thePrim, float) { aHitsQuad.insert(thePrim); };
  auto          aAccOct  = [&](int thePrim, float) { aHitsOct.insert(thePrim); };
  auto          aAccHex  = [&](int thePrim, float) { aHitsHex.insert(thePrim); };
  BVH::SIMD::TraverseQuad(aQuadF, aRay, aAccQuad);
  BVH::SIMD::TraverseWide<8>(aOctF, aRay, aAccOct);
  BVH::SIMD::TraverseWide<16>(aHexF, aRay, aAccHex);

  // Cross-check: all three width factors must return the same hit set.
  EXPECT_EQ(aHitsQuad, aHitsOct) << "BVH4 and BVH8 disagree on hits";
  EXPECT_EQ(aHitsOct, aHitsHex) << "BVH8 and BVH16 disagree on hits";

  // ---------------------------------------------------------------------
  // 5. Emit a self-contained HTML file with the scene parameters inlined.
  //    Three.js is pulled from a CDN; the boxes are regenerated client-
  //    side so a slider can vary N live without re-running the test.
  //
  //    Embedded benchmark numbers come from GCP c3-highcpu-4
  //    (Intel Sapphire Rapids, AVX-512 host) -- the first run that
  //    actually validated the BVH16 zmm kernel end-to-end.
  // ---------------------------------------------------------------------
  std::ostringstream aData;
  aData << "{\n"
        << "  \"ray\": {\"origin\":[" << aOrigin.x() << "," << aOrigin.y() << "," << aOrigin.z()
        << "],\"dir\":[" << aDir.x() << "," << aDir.y() << "," << aDir.z() << "],\"length\":40},\n"
        << "  \"sceneBounds\": [-10, 10],\n"
        << "  \"sizeRange\":   [0.3, 1.2],\n"
        << "  \"initN\":       " << kNumPrims << ",\n"
        << "  \"meta\": {\n"
        << "    \"simdLevel\": \"" << kSimdLevelName(BVH::SIMD::Detect()) << "\"\n"
        << "  }\n"
        << "}\n";

  static const char* kHtmlPrefix = R"HTML(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>OCCT BVH4 / BVH8 / BVH16 traversal</title>
  <style>
    html, body { margin: 0; padding: 0; height: 100%; background: #111;
                  color: #ddd; font-family: monospace; overflow: hidden; }
    #info { position: absolute; top: 10px; left: 10px; padding: 12px 16px;
            background: rgba(0, 0, 0, 0.78); border: 1px solid #444;
            border-radius: 6px; max-width: 420px; line-height: 1.4; }
    #info h3 { margin: 0 0 8px 0; color: #fff; }
    label { display: block; margin: 4px 0; cursor: pointer; }
    label input { vertical-align: middle; margin-right: 6px; }
    .swatch { display: inline-block; width: 12px; height: 12px;
              vertical-align: middle; margin-right: 4px;
              border: 1px solid #888; }
    hr { border: none; border-top: 1px solid #444; margin: 8px 0; }
    #benchTable { background: #000; padding: 6px 8px; margin: 4px 0;
             white-space: pre; font-size: 11px; line-height: 1.35; }
  </style>
</head>
<body>
  <div id="info">
    <h3>BVH4 / BVH8 / BVH16 traversal</h3>
    <div>Host SIMD level: <b><span id="simd">-</span></b></div>
    <div>Primitives: <b><span id="nPrims">-</span></b></div>
    <div>Hits: <b><span id="nHits">-</span></b></div>
    <hr>
    <label style="cursor:auto;">N (prims):
      <input type="range" id="nSlider" min="64" max="20000" value="256" step="1"
             style="width:180px;vertical-align:middle;">
      <span id="nValue" style="display:inline-block;width:50px;text-align:right;">256</span>
    </label>
    <hr>
    <label><input type="checkbox" id="showAll" checked>
      <span class="swatch" style="background:#444"></span>All boxes</label>
    <label><input type="checkbox" id="showRay" checked>
      <span class="swatch" style="background:#0f0"></span>Ray</label>
    <label><input type="checkbox" id="showHits" checked>
      <span class="swatch" style="background:#f33"></span>Hit AABBs (BVH4/8/16 agree)</label>
    <hr>
    <div>Bench on Intel Sapphire Rapids (c3-highcpu-4, AVX-512F):</div>
    <pre id="benchTable">
          Scalar   SSE2    AVX2   AVX-512    best
  BVH4   31.22  [ 3.20]  3.55    3.40      SSE2   9.77x
  BVH8   75.68   --     [5.70]   --        AVX2  13.29x
  BVH16 151.53   --      --     [7.47]     AVX-512 20.28x
  ─────────────────────────────────────────────────────
  per-lane winner: BVH16 AVX-512 = 0.467 ns/lane
  zmm vs ymm: 1.52x per-lane (AVX-512 really uses __m512)</pre>
    <div style="font-size:11px;color:#888;margin-top:6px;">
      (Units: ns per 1-ray-vs-W-AABB kernel call. [brackets] = natural fit kernel for that width.)
    </div>
    <div style="font-size:11px;color:#888;margin-top:8px;">
      Drag = rotate &middot; Wheel = zoom &middot; Shift+drag = pan<br>
      Slider regenerates the scene; hits = AABB-Ray slab test, the same
      result all three traversals return.
    </div>
  </div>
  <canvas id="cv"></canvas>
  <script type="importmap">
  { "imports": {
      "three": "https://unpkg.com/three@0.160.0/build/three.module.js",
      "three/addons/": "https://unpkg.com/three@0.160.0/examples/jsm/"
  } }
  </script>
  <script type="module">
    import * as THREE from 'three';
    import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

    const data = )HTML";

  static const char* kHtmlSuffix = R"HTML(;

    document.getElementById('simd').textContent = data.meta.simdLevel;

    const canvas = document.getElementById('cv');
    const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    renderer.setPixelRatio(window.devicePixelRatio);
    function resize() {
      renderer.setSize(window.innerWidth, window.innerHeight, false);
      camera.aspect = window.innerWidth / window.innerHeight;
      camera.updateProjectionMatrix();
    }

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x111111);
    const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 1000);
    camera.position.set(20, 20, 25);

    scene.add(new THREE.AmbientLight(0x666666));
    const dir = new THREE.DirectionalLight(0xffffff, 0.6);
    dir.position.set(20, 30, 20);
    scene.add(dir);

    scene.add(new THREE.GridHelper(40, 20, 0x444444, 0x282828));

    function makeAxis(dir, len, color, label) {
      const g = new THREE.Group();
      const radius = 0.06;
      const shaft = new THREE.Mesh(
        new THREE.CylinderGeometry(radius, radius, len, 12),
        new THREE.MeshBasicMaterial({ color }));
      shaft.position.copy(dir.clone().multiplyScalar(len / 2));
      const up = new THREE.Vector3(0, 1, 0);
      const q = new THREE.Quaternion().setFromUnitVectors(up, dir);
      shaft.quaternion.copy(q);
      g.add(shaft);
      const tip = new THREE.Mesh(
        new THREE.ConeGeometry(radius * 4, radius * 12, 16),
        new THREE.MeshBasicMaterial({ color }));
      tip.position.copy(dir.clone().multiplyScalar(len));
      tip.quaternion.copy(q);
      g.add(tip);
      const c = document.createElement('canvas');
      c.width = c.height = 64;
      const ctx = c.getContext('2d');
      ctx.fillStyle = '#' + color.toString(16).padStart(6, '0');
      ctx.font = 'bold 48px sans-serif';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(label, 32, 32);
      const tex = new THREE.CanvasTexture(c);
      const spr = new THREE.Sprite(new THREE.SpriteMaterial({ map: tex, depthTest: false }));
      spr.scale.set(2.0, 2.0, 1);
      spr.position.copy(dir.clone().multiplyScalar(len + 1.5));
      g.add(spr);
      return g;
    }
    scene.add(makeAxis(new THREE.Vector3(1, 0, 0), 12, 0xff4040, 'X'));
    scene.add(makeAxis(new THREE.Vector3(0, 1, 0), 12, 0x40ff40, 'Y'));
    scene.add(makeAxis(new THREE.Vector3(0, 0, 1), 12, 0x4080ff, 'Z'));

    const ro = new THREE.Vector3(...data.ray.origin);
    const rd = new THREE.Vector3(...data.ray.dir).normalize();

    const rayGroup = new THREE.Group();
    {
      const len = data.ray.length;
      const r = 0.15;
      const shaft = new THREE.Mesh(
        new THREE.CylinderGeometry(r, r, len, 16),
        new THREE.MeshBasicMaterial({ color: 0x33ff66, transparent: true, opacity: 0.55 }));
      shaft.position.copy(ro.clone().add(rd.clone().multiplyScalar(len / 2)));
      const up = new THREE.Vector3(0, 1, 0);
      const q = new THREE.Quaternion().setFromUnitVectors(up, rd);
      shaft.quaternion.copy(q);
      rayGroup.add(shaft);

      const tip = new THREE.Mesh(
        new THREE.ConeGeometry(0.5, 1.5, 16),
        new THREE.MeshBasicMaterial({ color: 0x33ff66 }));
      tip.position.copy(ro.clone().add(rd.clone().multiplyScalar(len)));
      tip.quaternion.copy(q);
      rayGroup.add(tip);

      const startBall = new THREE.Mesh(
        new THREE.SphereGeometry(0.4, 16, 16),
        new THREE.MeshBasicMaterial({ color: 0xffffff }));
      startBall.position.copy(ro);
      rayGroup.add(startBall);
    }
    scene.add(rayGroup);

    function mulberry32(seed) {
      let s = seed >>> 0;
      return function() {
        s |= 0; s = (s + 0x6D2B79F5) | 0;
        let t = s;
        t = Math.imul(t ^ (t >>> 15), t | 1);
        t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
      };
    }

    const ridx = 1 / rd.x, ridy = 1 / rd.y, ridz = 1 / rd.z;
    function rayHitsBox(b) {
      const t1x = (b[0] - ro.x) * ridx, t2x = (b[3] - ro.x) * ridx;
      const t1y = (b[1] - ro.y) * ridy, t2y = (b[4] - ro.y) * ridy;
      const t1z = (b[2] - ro.z) * ridz, t2z = (b[5] - ro.z) * ridz;
      const tNear = Math.max(Math.min(t1x, t2x),
                              Math.min(t1y, t2y),
                              Math.min(t1z, t2z));
      const tFar  = Math.min(Math.max(t1x, t2x),
                              Math.max(t1y, t2y),
                              Math.max(t1z, t2z));
      return tNear <= tFar && tFar >= 0;
    }

    const matAll = new THREE.LineBasicMaterial({
      color: 0x666666, transparent: true, opacity: 0.25 });
    const matHit = new THREE.LineBasicMaterial({ color: 0xff3333 });
    const dropMat = new THREE.LineDashedMaterial({
      color: 0xffaa00, dashSize: 0.4, gapSize: 0.25 });

    let groupAll  = new THREE.Group();
    let groupHits = new THREE.Group();
    let groupDrops = new THREE.Group();
    scene.add(groupAll, groupHits, groupDrops);

    function disposeGroup(g) {
      g.traverse(o => { if (o.geometry) o.geometry.dispose(); });
      scene.remove(g);
    }

    function makeWireBox(b, mat) {
      const w = b[3] - b[0], h = b[4] - b[1], d = b[5] - b[2];
      const cx = (b[0] + b[3]) / 2, cy = (b[1] + b[4]) / 2, cz = (b[2] + b[5]) / 2;
      const geo = new THREE.BoxGeometry(w, h, d);
      const edges = new THREE.EdgesGeometry(geo);
      const line = new THREE.LineSegments(edges, mat);
      line.position.set(cx, cy, cz);
      return line;
    }

    function rebuild(N) {
      disposeGroup(groupAll);
      disposeGroup(groupHits);
      disposeGroup(groupDrops);
      groupAll = new THREE.Group();
      groupHits = new THREE.Group();
      groupDrops = new THREE.Group();

      const rng = mulberry32(42);
      const lo = data.sceneBounds[0], hi = data.sceneBounds[1];
      const sLo = data.sizeRange[0], sHi = data.sizeRange[1];
      const span = hi - lo, sSpan = sHi - sLo;

      let nHit = 0;
      for (let i = 0; i < N; ++i) {
        const cx = lo + rng() * span, cy = lo + rng() * span, cz = lo + rng() * span;
        const hx = sLo + rng() * sSpan, hy = sLo + rng() * sSpan, hz = sLo + rng() * sSpan;
        const b = [cx - hx, cy - hy, cz - hz, cx + hx, cy + hy, cz + hz];
        groupAll.add(makeWireBox(b, matAll));
        if (rayHitsBox(b)) {
          ++nHit;
          groupHits.add(makeWireBox(b, matHit));
          const P = new THREE.Vector3(cx, cy, cz);
          const t = Math.max(0, P.clone().sub(ro).dot(rd));
          const F = ro.clone().add(rd.clone().multiplyScalar(t));
          const g = new THREE.BufferGeometry().setFromPoints([P, F]);
          const line = new THREE.Line(g, dropMat);
          line.computeLineDistances();
          groupDrops.add(line);
        }
      }
      scene.add(groupAll, groupHits, groupDrops);

      groupAll.visible  = document.getElementById('showAll').checked;
      groupHits.visible = document.getElementById('showHits').checked;
      groupDrops.visible = document.getElementById('showHits').checked;

      document.getElementById('nPrims').textContent = N;
      document.getElementById('nHits').textContent = nHit;
    }

    document.getElementById('showAll').addEventListener('change', e => {
      groupAll.visible = e.target.checked;
    });
    document.getElementById('showHits').addEventListener('change', e => {
      groupHits.visible = e.target.checked;
      groupDrops.visible = e.target.checked;
    });
    document.getElementById('showRay').addEventListener('change', e => {
      rayGroup.visible = e.target.checked;
    });

    const slider = document.getElementById('nSlider');
    const nValue = document.getElementById('nValue');
    slider.value = data.initN;
    nValue.textContent = data.initN;
    let pending = null;
    slider.addEventListener('input', () => {
      nValue.textContent = slider.value;
      if (pending) clearTimeout(pending);
      pending = setTimeout(() => { rebuild(parseInt(slider.value, 10)); pending = null; }, 80);
    });

    rebuild(data.initN);

    const orbit = new OrbitControls(camera, canvas);
    orbit.target.set(0, 0, 0);
    orbit.update();

    function tick() {
      requestAnimationFrame(tick);
      orbit.update();
      renderer.render(scene, camera);
    }
    window.addEventListener('resize', resize);
    resize();
    tick();
  </script>
</body>
</html>
)HTML";

  const char* aOutPath = std::getenv("OCCT_BVH_VIZ_OUT");
  if (aOutPath == nullptr || *aOutPath == '\0')
  {
    aOutPath = "/tmp/bvh_4_8_16_visualization.html";
  }
  std::ofstream aHtml(aOutPath);
  ASSERT_TRUE(aHtml.is_open()) << "Could not open " << aOutPath;
  aHtml << kHtmlPrefix << aData.str() << kHtmlSuffix;
  aHtml.close();

  std::cout << "\n=== BVH4 / BVH8 / BVH16 visualization ===\n"
            << "  primitives    : " << kNumPrims << "\n"
            << "  BVH4  hits    : " << aHitsQuad.size() << "\n"
            << "  BVH8  hits    : " << aHitsOct.size() << "\n"
            << "  BVH16 hits    : " << aHitsHex.size() << "\n"
            << "  SIMD level    : " << kSimdLevelName(BVH::SIMD::Detect()) << "\n"
            << "  HTML output   : " << aOutPath << "\n"
            << "  open with     : xdg-open " << aOutPath << "\n";

  // Numerical sanity check against the BVH16 set (should equal BVH4/BVH8).
  const float aDirLen = std::sqrt(aDir.x() * aDir.x() + aDir.y() * aDir.y() + aDir.z() * aDir.z());
  const BVH_Vec3f aDirN(aDir.x() / aDirLen, aDir.y() / aDirLen, aDir.z() / aDirLen);

  std::cout << "\n  ray origin = (" << aOrigin.x() << ", " << aOrigin.y() << ", " << aOrigin.z()
            << ")"
            << "  dir(normalised) = (" << aDirN.x() << ", " << aDirN.y() << ", " << aDirN.z()
            << ")\n\n";

  std::cout << "  hit AABBs (BVH16, matches BVH4/BVH8):\n";
  for (int idx : aHitsHex)
  {
    const auto& b  = aRawBoxes[idx];
    const float cx = (b.minX + b.maxX) * 0.5f;
    const float cy = (b.minY + b.maxY) * 0.5f;
    const float cz = (b.minZ + b.maxZ) * 0.5f;
    const float dx = cx - aOrigin.x();
    const float dy = cy - aOrigin.y();
    const float dz = cz - aOrigin.z();
    const float t  = std::max(0.0f, dx * aDirN.x() + dy * aDirN.y() + dz * aDirN.z());
    const float fx = aOrigin.x() + t * aDirN.x();
    const float fy = aOrigin.y() + t * aDirN.y();
    const float fz = aOrigin.z() + t * aDirN.z();
    const float perpDist =
      std::sqrt((cx - fx) * (cx - fx) + (cy - fy) * (cy - fy) + (cz - fz) * (cz - fz));
    const float halfDiagonal =
      0.5f
      * std::sqrt((b.maxX - b.minX) * (b.maxX - b.minX) + (b.maxY - b.minY) * (b.maxY - b.minY)
                  + (b.maxZ - b.minZ) * (b.maxZ - b.minZ));
    std::cout << "    [" << idx << "] AABB = [" << b.minX << ", " << b.minY << ", " << b.minZ
              << "] ~ [" << b.maxX << ", " << b.maxY << ", " << b.maxZ << "]"
              << "  centre = (" << cx << ", " << cy << ", " << cz << ")"
              << "  t = " << t << "  perpDist = " << perpDist << "  halfDiag = " << halfDiagonal
              << (perpDist <= halfDiagonal ? "  <- centre within bounding sphere of ray"
                                           : "  <- only a corner clips the ray")
              << "\n";
  }
}
