import fs from 'node:fs';
import path from 'node:path';

const [source, outputDir, objFile] = process.argv.slice(2);
if (!source || !outputDir) throw new Error('Usage: node bake_metal_textures.mjs <warehouse.dds> <output-dir> [facility.obj]');

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
function writeDds(file, width, height, pixels) {
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
  const header = Buffer.alloc(128); header.write('DDS '); header.writeUInt32LE(124, 4); header.writeUInt32LE(0x00081007, 8);
  header.writeUInt32LE(height, 12); header.writeUInt32LE(width, 16); header.writeUInt32LE(payload.length, 20);
  header.writeUInt32LE(32, 76); header.writeUInt32LE(4, 80); header.write('DXT1', 84); header.writeUInt32LE(0x1000, 108);
  fs.writeFileSync(file, Buffer.concat([header, payload]));
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
function muteTankBand(file) {
  if (!file) return;
  const lines = fs.readFileSync(file, 'utf8').split(/\r?\n/), vertices = [], output = [];
  let object = '', material = '';
  for (const line of lines) {
    const fields = line.trim().split(/\s+/);
    if (fields[0] === 'v') vertices.push(fields.slice(1, 4).map(Number));
    if (fields[0] === 'o') object = fields[1] || '';
    if (fields[0] === 'usemtl') material = fields[1] || '';
    if (object === 'mesh_safety_red' && fields[0] === 'f') {
      const points = fields.slice(1).map(f => vertices[Number(f.split('/')[0]) - 1]);
      if (points.length && points.every(p => p && p[1] >= 2.91 && p[1] <= 3.39 && Math.abs(Math.hypot(p[0] + 21.35, p[2] + 1.0) - 1.405) < 0.055)) {
        if (material !== 'decorative_tank_painted_steel') { output.push('usemtl decorative_tank_painted_steel'); material = 'decorative_tank_painted_steel'; }
      } else if (material !== 'safety_red') { output.push('usemtl safety_red'); material = 'safety_red'; }
    }
    output.push(line);
  }
  fs.writeFileSync(file, output.join('\n'));
}
fs.mkdirSync(outputDir, { recursive: true });
const atlas = decodeDds(source);
cropAndTone(atlas, [970, 30, 2000, 438], 'earthworks_corrugated_roof.dds', { contrast: 0.48, lift: 0, tint: [0.9, 0.91, 0.92] });
cropAndTone(atlas, [1230, 1225, 1670, 1460], 'earthworks_tank_steel.dds', { contrast: 0.62, lift: 24, tint: [0.9, 0.94, 0.96] });
muteTankBand(objFile);
console.log(`Baked muted roof and tank DDS textures from ${source}`);
