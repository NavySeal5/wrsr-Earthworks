import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';

const [source, outputDir, objFile] = process.argv.slice(2);
if (!source || !outputDir) throw new Error('Usage: node bake_metal_textures.mjs <warehouse.dds> <output-dir> [facility.obj] [atlas-preview.png] [tank-texture.dds]');

const rgb565 = value => [((value >> 11) & 31) * 255 / 31, ((value >> 5) & 63) * 255 / 63, (value & 31) * 255 / 31];
function decodeDds(file) {
  const data = fs.readFileSync(file);
  const width = data.readUInt32LE(16), height = data.readUInt32LE(12);
  const fourcc = data.toString('ascii', 84, 88);
  if (data.toString('ascii', 0, 4) !== 'DDS ' || fourcc !== 'DXT1') throw new Error(`Expected DXT1 DDS: ${file} (${fourcc})`);
  const pixels = new Uint8Array(width * height * 3), blocksX = Math.ceil(width / 4), blocksY = Math.ceil(height / 4);
  let offset = 128;
  for (let by = 0; by < blocksY; by++) for (let bx = 0; bx < blocksX; bx++) {
    const c0 = data.readUInt16LE(offset), c1 = data.readUInt16LE(offset + 2), selectors = data.readUInt32LE(offset + 4); offset += 8;
    const a = rgb565(c0), b = rgb565(c1);
    const palette = [a, b, c0 > c1 ? a.map((v, i) => (2 * v + b[i]) / 3) : a.map((v, i) => (v + b[i]) / 2), c0 > c1 ? a.map((v, i) => (v + 2 * b[i]) / 3) : [0, 0, 0]];
    for (let py = 0; py < 4; py++) for (let px = 0; px < 4; px++) {
      const x = bx * 4 + px, y = by * 4 + py;
      if (x >= width || y >= height) continue;
      const color = palette[(selectors >>> (2 * (py * 4 + px))) & 3], p = (y * width + x) * 3;
      pixels[p] = Math.round(color[0]); pixels[p + 1] = Math.round(color[1]); pixels[p + 2] = Math.round(color[2]);
    }
  }
  return { width, height, pixels };
}
function enc565([r, g, b]) { return ((Math.round(r * 31 / 255) << 11) | (Math.round(g * 63 / 255) << 5) | Math.round(b * 31 / 255)); }
function encodeBc1(width, height, pixels) {
  const bx = Math.ceil(width / 4), by = Math.ceil(height / 4), payload = Buffer.alloc(bx * by * 8);
  let off = 0;
  for (let yb = 0; yb < by; yb++) for (let xb = 0; xb < bx; xb++) {
    const block = [];
    for (let y = 0; y < 4; y++) for (let x = 0; x < 4; x++) {
      const sx = Math.min(xb * 4 + x, width - 1), sy = Math.min(yb * 4 + y, height - 1), p = (sy * width + sx) * 3;
      block.push([pixels[p], pixels[p + 1], pixels[p + 2]]);
    }
    const lum = c => 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2];
    block.sort((a, b) => lum(a) - lum(b));
    let c0 = enc565(block[15]), c1 = enc565(block[0]);
    if (c0 <= c1) { if (c0 < 0xffff) [c0, c1] = [c1, c0]; else [c0, c1] = [0xffff, 0xfffe]; }
    const p0 = rgb565(c0), p1 = rgb565(c1), palette = [p0, p1, p0.map((v, i) => (2 * v + p1[i]) / 3), p0.map((v, i) => (v + 2 * p1[i]) / 3)];
    let bits = 0;
    block.forEach((color, i) => {
      let best = 0, bestD = Infinity;
      palette.forEach((candidate, j) => { const d = color.reduce((sum, v, k) => sum + (v - candidate[k]) ** 2, 0); if (d < bestD) { bestD = d; best = j; } });
      bits |= best << (i * 2);
    });
    payload.writeUInt16LE(c0, off); payload.writeUInt16LE(c1, off + 2); payload.writeUInt32LE(bits >>> 0, off + 4); off += 8;
  }
  return payload;
}
function downsample(width, height, pixels) {
  const nextWidth = Math.max(1, Math.floor(width / 2)), nextHeight = Math.max(1, Math.floor(height / 2));
  const reduced = new Uint8Array(nextWidth * nextHeight * 3);
  for (let y = 0; y < nextHeight; y++) for (let x = 0; x < nextWidth; x++) {
    const dst = (y * nextWidth + x) * 3;
    for (let c = 0; c < 3; c++) {
      let sum = 0;
      for (let dy = 0; dy < 2; dy++) for (let dx = 0; dx < 2; dx++) {
        const sx = Math.min(width - 1, x * 2 + dx), sy = Math.min(height - 1, y * 2 + dy);
        sum += pixels[(sy * width + sx) * 3 + c];
      }
      reduced[dst + c] = Math.round(sum / 4);
    }
  }
  return { width: nextWidth, height: nextHeight, pixels: reduced };
}
function writeDds(file, width, height, pixels) {
  const levels = [{ width, height, pixels }];
  while (width > 1 || height > 1) {
    const next = downsample(width, height, pixels);
    ({ width, height, pixels } = next);
    levels.push(next);
  }
  const topBlocksX = Math.ceil(levels[0].width / 4), topBlocksY = Math.ceil(levels[0].height / 4);
  const header = Buffer.alloc(128); header.write('DDS '); header.writeUInt32LE(124, 4); header.writeUInt32LE(0x00a1007, 8);
  header.writeUInt32LE(levels[0].height, 12); header.writeUInt32LE(levels[0].width, 16); header.writeUInt32LE(topBlocksX * topBlocksY * 8, 20);
  header.writeUInt32LE(levels.length, 28); header.writeUInt32LE(32, 76); header.writeUInt32LE(4, 80); header.write('DXT1', 84); header.writeUInt32LE(0x401008, 108);
  fs.writeFileSync(file, Buffer.concat([header, ...levels.map(level => encodeBc1(level.width, level.height, level.pixels))]));
}
function writePng(file, width, height, pixels) {
  const crcTable = new Uint32Array(256);
  for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = (c & 1) ? (0xedb88320 ^ (c >>> 1)) : (c >>> 1); crcTable[n] = c >>> 0; }
  const crc32 = data => { let c = 0xffffffff; for (const b of data) c = crcTable[(c ^ b) & 0xff] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; };
  const chunk = (type, data) => { const name = Buffer.from(type), body = Buffer.concat([name, data]), out = Buffer.alloc(body.length + 8); out.writeUInt32BE(data.length, 0); body.copy(out, 4); out.writeUInt32BE(crc32(body), body.length + 4); return out; };
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(width, 0); ihdr.writeUInt32BE(height, 4); ihdr[8] = 8; ihdr[9] = 2;
  const rows = Buffer.alloc(height * (1 + width * 3)); for (let y = 0; y < height; y++) { const row = y * (1 + width * 3); rows[row] = 0; Buffer.from(pixels.buffer, pixels.byteOffset + y * width * 3, width * 3).copy(rows, row + 1); }
  fs.writeFileSync(file, Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(rows)), chunk('IEND', Buffer.alloc(0))]));
}
function cropAndTone(atlas, rect, output, { contrast, lift, tint }) {
  const [x0, y0, x1, y1] = rect, width = (x1 - x0 + 3) & ~3, height = (y1 - y0 + 3) & ~3;
  const pixels = new Uint8Array(width * height * 3);
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
    const sx = x0 + Math.min(x, x1 - x0 - 1), sy = y0 + Math.min(y, y1 - y0 - 1), src = (sy * atlas.width + sx) * 3, dst = (y * width + x) * 3;
    for (let c = 0; c < 3; c++) pixels[dst + c] = Math.max(0, Math.min(255, Math.round(((atlas.pixels[src + c] - 128) * contrast + 128 + lift) * tint[c])));
  }
  writeDds(path.join(outputDir, output), width, height, pixels);
}
function stripMaterialMesh(file, materialName) {
  if (!file) return;
  const lines = fs.readFileSync(file, 'utf8').split(/\r?\n/);
  const output = [];
  let object = '';
  let removedFaces = 0;
  for (const line of lines) {
    const f = line.trim().split(/\s+/);
    if (f[0] === 'o') object = f[1] || '';
    // OBJ references are global across objects. Keep all source vertex/UV/
    // normal declarations in place and remove only the rib faces so later
    // face indices remain valid for ModelViewer.
    if (object === `mesh_${materialName}` && f[0] === 'f') { removedFaces++; continue; }
    if (object === `mesh_${materialName}` && f[0] === 'usemtl') continue;
    output.push(line);
  }
  fs.writeFileSync(file, output.join('\r\n'));
  console.log(`Removed ${removedFaces} faces from ${materialName}; unused OBJ coordinates are retained to preserve global indices`);
}
fs.mkdirSync(outputDir, { recursive: true });
const atlas = decodeDds(source);
const tankAtlas = decodeDds(process.argv[6] || path.join(path.dirname(source), 'explosive_factory_tank.dds'));
if (process.argv[5]) writePng(process.argv[5], atlas.width, atlas.height, atlas.pixels);
cropAndTone(atlas, [970, 30, 2000, 438], 'earthworks_corrugated_roof.dds', { contrast: 1.0, lift: 0, tint: [1, 1, 1] });
cropAndTone(tankAtlas, [512, 512, 1024, 1024], 'earthworks_tank_steel.dds', { contrast: 0.72, lift: 48, tint: [0.88, 0.94, 0.98] });
stripMaterialMesh(objFile, 'roof_rib_highlight');
console.log(`Baked muted roof and tank DDS textures from ${source}`);
