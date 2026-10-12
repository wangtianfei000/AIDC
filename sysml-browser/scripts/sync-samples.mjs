import { createHash } from 'node:crypto';
import { copyFile, mkdir, readFile } from 'node:fs/promises';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const browserRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const aidcRoot = resolve(browserRoot, '..');
const checkOnly = process.argv.includes('--check');

export const sampleMappings = [
  ['samples/jingang-10axis-system-design/architecture.sysml', 'public/samples/robot-architecture.sysml'],
  ['samples/VehicleMode.sysml', 'public/samples/VehicleMode.sysml'],
  ['samples/analysis-exception-acceptance.sysml', 'public/samples/analysis-exception-acceptance.sysml'],
];

function hash(content) {
  return createHash('sha256').update(content).digest('hex');
}

let drift = false;
for (const [sourceRelative, targetRelative] of sampleMappings) {
  const source = resolve(aidcRoot, sourceRelative);
  const target = resolve(browserRoot, targetRelative);
  const sourceContent = await readFile(source);
  const sourceHash = hash(sourceContent);
  let targetHash = '';
  try { targetHash = hash(await readFile(target)); } catch { /* missing target is drift */ }
  if (sourceHash !== targetHash) {
    drift = true;
    if (!checkOnly) {
      await mkdir(dirname(target), { recursive: true });
      await copyFile(source, target);
      targetHash = sourceHash;
      process.stdout.write(`已同步 ${sourceRelative} -> ${targetRelative}\n`);
    } else {
      process.stderr.write(`样例不同步：${sourceRelative} -> ${targetRelative}\n`);
    }
  }
  if (sourceHash === targetHash) process.stdout.write(`样例一致 ${targetRelative} ${sourceHash}\n`);
}

if (checkOnly && drift) {
  process.stderr.write('请运行 npm run samples:sync 后提交唯一源与发布副本。\n');
  process.exit(1);
}
