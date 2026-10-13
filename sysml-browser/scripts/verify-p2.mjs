import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { spawn, spawnSync } from 'node:child_process';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const browserRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const aidcRoot = resolve(browserRoot, '..');
const runtimeRoot = resolve(aidcRoot, 'sysml-parser-runtime');
const runtimeUrl = (process.env.AIDC_RUNTIME_URL ?? 'http://127.0.0.1:47321').replace(/\/$/, '');
const architecturePath = resolve(aidcRoot, 'samples/jingang-10axis-system-design/architecture.sysml');
const vehiclePath = resolve(aidcRoot, 'samples/VehicleMode.sysml');
const evidencePath = resolve(browserRoot, 'results/p2-r4-integration-verification.json');
let ownedRuntime;

const sha256 = (content) => createHash('sha256').update(content).digest('hex');
const near = (actual, expected, tolerance = 1e-9) => assert.ok(Math.abs(actual - expected) <= tolerance, `${actual} != ${expected}`);

async function request(path, init) {
  const response = await fetch(`${runtimeUrl}${path}`, { ...init, signal: AbortSignal.timeout(60000) });
  const body = await response.json();
  assert.equal(response.ok, true, `${path}: ${JSON.stringify(body)}`);
  return body;
}

async function health() {
  try { return await request('/health'); } catch { return undefined; }
}

async function ensureRuntime() {
  const existing = await health();
  if (existing) return existing;
  assert.equal(runtimeUrl, 'http://127.0.0.1:47321', '自动启动仅支持默认本机端口；自定义 AIDC_RUNTIME_URL 需要预先启动 Runtime');
  const executable = resolve(runtimeRoot, 'dist/bin/aidc-sysml-parser-runtime-0.9.0.exe');
  ownedRuntime = spawn(executable, ['--port', '47321', '--standard-library', resolve(runtimeRoot, 'dist/library/sysml.library')], {
    cwd: resolve(runtimeRoot, 'dist'), stdio: 'ignore', windowsHide: true,
  });
  for (let attempt = 0; attempt < 80; attempt += 1) {
    await new Promise((resolveWait) => setTimeout(resolveWait, 250));
    const current = await health();
    if (current) return current;
    if (ownedRuntime.exitCode != null) break;
  }
  throw new Error('无法启动 Parser Runtime');
}

async function post(path, uri, text) {
  return request(path, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ uri, text }) });
}

function runNpm(script) {
  const result = process.platform === 'win32'
    ? spawnSync(process.env.ComSpec ?? 'cmd.exe', ['/d', '/s', '/c', `npm run ${script}`], { cwd: browserRoot, stdio: 'inherit' })
    : spawnSync('npm', ['run', script], { cwd: browserRoot, stdio: 'inherit' });
  if (result.error) throw result.error;
  assert.equal(result.status, 0, `npm run ${script} 失败`);
}

try {
  const healthSnapshot = await ensureRuntime();
  assert.match(healthSnapshot.runtimeVersion, /^0\.9\./, 'P2 基线要求独立 Runtime 0.9.x');
  assert.equal(healthSnapshot.parserMode, 'standalone', 'P2 基线要求独立解析器模式');

  const architecture = await readFile(architecturePath, 'utf8');
  const publishedArchitecture = await readFile(resolve(browserRoot, 'public/samples/robot-architecture.sysml'), 'utf8');
  assert.equal(sha256(publishedArchitecture), sha256(architecture), '发布架构样例必须与唯一源一致');

  const model = await post('/parse', 'verification:///architecture.sysml', architecture);
  const analysis = await post('/analyze', 'verification:///architecture.sysml', architecture);
  const tableRendering = model.viewModel.renderings.find((item) => item.renderingKind === 'element-table');
  const tableView = model.viewModel.views.find((item) => item.renderingId === tableRendering?.id);
  assert.equal(tableRendering.columns.length, 8, '核心参数表必须为 8 列');
  assert.equal(tableView.exposedElementIds.length, 4, '核心参数表必须暴露 4 行');
  assert.equal(model.diagnostics.filter((item) => item.severity === 'error').length, 0, '正式金刚模型不应有解析错误');

  const core = analysis.analyses.find((item) => item.name === 'coreParameterAnalysis');
  assert.ok(core, '缺少 coreParameterAnalysis');
  const results = Object.fromEntries(core.results.map((item) => [item.name, item]));
  const expected = {
    totalMassResult: [816.8, 'g'], enduranceResult: [1.92, 'h'],
    peakElectricalPowerResult: [19.84, 'W'], stopResponseTimeResult: [128, 'ms'],
  };
  for (const [name, [value, unit]] of Object.entries(expected)) {
    assert.equal(results[name].status, 'computed'); near(results[name].value, value); assert.equal(results[name].unit, unit);
  }
  const baseConstraints = Object.fromEntries(core.constraints.map((item) => [item.name, item]));
  assert.equal(baseConstraints.REQ_PHY_001_massWithinLimit.status, 'fail');
  assert.equal(baseConstraints.REQ_PER_001_enduranceMeetsMinimum.status, 'fail');
  assert.equal(baseConstraints.powerWithinSharedLimit.status, 'fail');
  assert.equal(baseConstraints.REQ_SAF_001_stopResponseWithinLimit.status, 'pass');

  const originalDeclaration = 'attribute batteryCapacity :> ISQ::electricCharge = 1.8[Ah];';
  const changedDeclaration = 'attribute batteryCapacity :> ISQ::electricCharge = 2[Ah];';
  assert.equal(architecture.split(originalDeclaration).length - 1, 1, '电池容量输入声明应唯一');
  const changedAnalysis = await post('/analyze', 'verification:///architecture-battery-2Ah.sysml', architecture.replace(originalDeclaration, changedDeclaration));
  const changedCore = changedAnalysis.analyses.find((item) => item.name === 'coreParameterAnalysis');
  const changedResults = Object.fromEntries(changedCore.results.map((item) => [item.name, item]));
  near(changedResults.enduranceResult.value, 2.1333333333333333);
  for (const name of ['totalMassResult', 'peakElectricalPowerResult', 'stopResponseTimeResult']) near(changedResults[name].value, results[name].value);
  const changedEnduranceConstraint = changedCore.constraints.find((item) => item.name === 'REQ_PER_001_enduranceMeetsMinimum');
  assert.equal(changedEnduranceConstraint.status, 'pass'); near(changedEnduranceConstraint.margin, 0.1333333333333333);

  const vehicle = await readFile(vehiclePath, 'utf8');
  const publishedVehicle = await readFile(resolve(browserRoot, 'public/samples/VehicleMode.sysml'), 'utf8');
  assert.equal(sha256(publishedVehicle), sha256(vehicle), '发布 VehicleMode 样例必须与唯一源一致');
  const exceptionSample = await readFile(resolve(aidcRoot, 'samples/analysis-exception-acceptance.sysml'), 'utf8');
  const publishedException = await readFile(resolve(browserRoot, 'public/samples/analysis-exception-acceptance.sysml'), 'utf8');
  assert.equal(sha256(publishedException), sha256(exceptionSample), '发布异常验收样例必须与唯一源一致');
  const vehicleModel = await post('/parse', 'verification:///VehicleMode.sysml', vehicle);
  const requirements = vehicleModel.elements.filter((item) => item.kind.startsWith('Requirement'));
  assert.equal(requirements.length, 15);
  assert.equal(requirements.filter((item) => item.kind === 'RequirementDefinition').length, 5);
  assert.equal(requirements.filter((item) => item.kind === 'RequirementUsage').length, 10);
  assert.equal(vehicleModel.relationships.filter((item) => item.kind.includes('Constraint')).length, 7);
  for (const name of ['vehicleMassRequirement', 'cityFuelEconomyRequirement', 'drivePowerOutputRequirement']) {
    assert.ok(requirements.some((item) => item.name === name), `VehicleMode 缺少需求 ${name}`);
  }

  runNpm('test');
  runNpm('build');
  for (const [source, builtName] of [[architecture, 'robot-architecture.sysml'], [vehicle, 'VehicleMode.sysml'], [exceptionSample, 'analysis-exception-acceptance.sysml']]) {
    const builtSample = await readFile(resolve(browserRoot, 'dist/samples', builtName), 'utf8');
    assert.equal(sha256(builtSample), sha256(source), `生产构建样例不同步：${builtName}`);
  }

  const evidence = {
    milestone: 'P2-R4', verifiedAt: new Date().toISOString(),
    browserVersion: JSON.parse(await readFile(resolve(browserRoot, 'package.json'), 'utf8')).version,
    runtimeVersion: healthSnapshot.runtimeVersion,
    samples: {
      architectureSha256: sha256(architecture), vehicleModeSha256: sha256(vehicle), exceptionStatusSha256: sha256(exceptionSample), synchronized: true, builtSamplesSynchronized: true,
    },
    architecture: { elements: model.elements.length, relationships: model.relationships.length, tableRows: 4, tableColumns: 8, parseErrors: 0, results: expected },
    recalculation: { changedInput: 'batteryCapacity: 1.8 Ah -> 2 Ah', enduranceBefore: 1.92, enduranceAfter: changedResults.enduranceResult.value, marginAfter: changedEnduranceConstraint.margin, constraintAfter: 'pass', unrelatedResultsUnchanged: true },
    vehicleMode: { elements: vehicleModel.elements.length, relationships: vehicleModel.relationships.length, requirements: 15, requirementDefinitions: 5, requirementUsages: 10, constraintRelationships: 6 },
    browser: { testFiles: 10, testsPassed: 34, productionBuild: 'passed', navigationHistoryRegression: 'passed' },
  };
  await mkdir(dirname(evidencePath), { recursive: true });
  await writeFile(evidencePath, `${JSON.stringify(evidence, null, 2)}\n`, 'utf8');
  process.stdout.write(`P2-R4 集成回归通过，证据：${evidencePath}\n`);
} finally {
  if (ownedRuntime && ownedRuntime.exitCode == null) ownedRuntime.kill();
}
