import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { spawn, type ChildProcess } from 'node:child_process';
import { join, resolve } from 'node:path';
import { defineConfig, type Plugin } from 'vite';
import { fileURLToPath, URL } from 'node:url';
import react from '@vitejs/plugin-react';

const defaultModelProjectDirectory = fileURLToPath(new URL('../../../samples/jingang-10axis-system-design/model3d/', import.meta.url));
const modelProjectDirectory = resolve(process.env.AIDC_MODEL3D_PROJECT_DIR ?? defaultModelProjectDirectory);
const generatedDirectory = join(modelProjectDirectory, 'generated');
const posePresetsFile = join(modelProjectDirectory, 'pose-presets.json');
const aidcDirectory = fileURLToPath(new URL('../../../', import.meta.url));
const manifestFile = join(generatedDirectory, 'model3d.manifest.json');
const hasDefaultModel = existsSync(manifestFile);
const manifest = hasDefaultModel
  ? JSON.parse(readFileSync(manifestFile, 'utf8')) as { modelId: string; revision: string; source?: { cadUri?: string } }
  : { modelId: 'unloaded', revision: '' };
const robotIrDirectory = resolve(process.env.AIDC_MODEL3D_ROBOT_IR ?? join(modelProjectDirectory, 'robot-ir'));
const assemblyOutputDirectory = resolve(process.env.AIDC_MODELICA_ASSEMBLY_OUTPUT ?? join(modelProjectDirectory, 'release', `${manifest.modelId}-${manifest.revision}.aidc-assembly`));
const exactServicePort = Number(process.env.AIDC_EXACT_SERVICE_PORT ?? 47331);
const exactServiceUrl = `http://127.0.0.1:${exactServicePort}`;
const exactServiceStartupAttempts = 80;
const exactServiceScript = join(aidcDirectory, 'model3d-viewer/services/exact-check/src/server.py');

export default defineConfig({
  plugins: [react(), modelProjectPoseStore(), modelicaAssemblyExport(), exactCheckService()],
  publicDir: hasDefaultModel ? generatedDirectory : false,
  server: {
    proxy: {
      '/api/exact-check': { target: exactServiceUrl, changeOrigin: true, rewrite: (path) => path.replace(/^\/api\/exact-check/, '') },
    },
  },
  build: { outDir: 'dist', emptyOutDir: true },
});

function modelicaAssemblyExport(): Plugin {
  return {
    name: 'aidc-modelica-assembly-export',
    configureServer(server) {
      server.middlewares.use(async (request, response, next) => {
        const pathname = new URL(request.url ?? '/', 'http://localhost').pathname;
        const expectedPath = `/api/model-projects/${encodeURIComponent(manifest.modelId)}/modelica-assembly`;
        if (pathname !== expectedPath) { next(); return; }
        response.setHeader('Content-Type', 'application/json; charset=utf-8');
        if (request.method !== 'POST') {
          response.statusCode = 405; response.end(JSON.stringify({ message: '仅支持导出装配包' })); return;
        }
        if (!existsSync(robotIrDirectory)) {
          response.statusCode = 409;
          response.end(JSON.stringify({ message: `当前三维工程缺少已评审 Robot IR：${robotIrDirectory}` }));
          return;
        }
        const exporter = join(aidcDirectory, 'model3d-viewer/tools/freecad-publisher/src/export_modelica_assembly.py');
        const python = process.env.AIDC_PYTHON ?? 'python';
        const child = spawn(python, [exporter, '--manifest', join(generatedDirectory, 'model3d.manifest.json'), '--robot-ir', robotIrDirectory, '--output', assemblyOutputDirectory], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
        let stdout = ''; let stderr = '';
        child.stdout?.on('data', (value) => { stdout += String(value); });
        child.stderr?.on('data', (value) => { stderr += String(value); });
        child.once('error', (error) => { response.statusCode = 500; response.end(JSON.stringify({ message: error.message })); });
        child.once('exit', (code) => {
          if (response.writableEnded) return;
          try {
            const payload = JSON.parse((code === 0 ? stdout : stderr || stdout).trim());
            response.statusCode = code === 0 ? 200 : 400;
            response.end(JSON.stringify(code === 0 ? payload : { message: payload.error ?? '装配包导出失败' }));
          } catch {
            response.statusCode = 500;
            response.end(JSON.stringify({ message: (stderr || stdout || `导出器退出码 ${code}`).trim() }));
          }
        });
      });
    },
  };
}

function exactCheckService(): Plugin {
  let child: ChildProcess | undefined;
  return {
    name: 'aidc-exact-check-service',
    async configureServer(server) {
      let ready = await isExactServiceReady();
      if (ready) {
        await new Promise((done) => setTimeout(done, 750));
        ready = await isExactServiceReady();
      }
      if (!ready) {
        const python = resolve(process.env.AIDC_FREECAD_PYTHON ?? join(aidcDirectory, '../tools/freecad/1.1.3/bin/python.exe'));
        const cad = manifest.source?.cadUri ? resolve(modelProjectDirectory, manifest.source.cadUri) : undefined;
        if (!existsSync(python) || !cad || !existsSync(cad)) {
          server.config.logger.warn('模型发布物未配置可用的 CAD/FreeCAD；浏览功能正常，精确 BRep 校核不可用');
          return;
        }
        child = spawn(python, [exactServiceScript, '--cad', cad, '--port', String(exactServicePort)], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
        child.stdout?.on('data', (value) => server.config.logger.info(`[exact-check] ${String(value).trim()}`));
        child.stderr?.on('data', (value) => server.config.logger.warn(`[exact-check] ${String(value).trim()}`));
        child.once('exit', (code) => { if (code && code !== 0) server.config.logger.error(`精确校核引擎异常退出：${code}`); });
        for (let attempt = 0; attempt < exactServiceStartupAttempts && !await isExactServiceReady(); attempt += 1) await new Promise((resolve) => setTimeout(resolve, 250));
        if (!await isExactServiceReady()) throw new Error('FreeCAD/OpenCascade 精确校核引擎启动超时');
      }
      server.httpServer?.once('close', () => { if (child && !child.killed) child.kill(); });
    },
  };
}

async function isExactServiceReady(): Promise<boolean> {
  try { return (await fetch(`${exactServiceUrl}/health`, { signal: AbortSignal.timeout(800) })).ok; }
  catch { return false; }
}

function modelProjectPoseStore(): Plugin {
  return {
    name: 'aidc-model-project-pose-store',
    configureServer(server) {
      server.middlewares.use(async (request, response, next) => {
        const pathname = new URL(request.url ?? '/', 'http://localhost').pathname;
        const expectedPath = `/api/model-projects/${encodeURIComponent(manifest.modelId)}/pose-presets`;
        if (pathname !== expectedPath) { next(); return; }
        response.setHeader('Content-Type', 'application/json; charset=utf-8');
        if (request.method === 'GET') {
          if (!existsSync(posePresetsFile)) { response.statusCode = 404; response.end(JSON.stringify({ message: '尚未打开模型' })); return; }
          response.end(readFileSync(posePresetsFile, 'utf8'));
          return;
        }
        if (request.method !== 'PUT') {
          response.statusCode = 405; response.end(JSON.stringify({ message: '仅支持读取和保存姿态预设' })); return;
        }
        try {
          const chunks: Buffer[] = [];
          let length = 0;
          for await (const chunk of request) {
            const buffer = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
            length += buffer.length;
            if (length > 1024 * 1024) throw new Error('姿态预设文件不能超过 1 MB');
            chunks.push(buffer);
          }
          const document = JSON.parse(Buffer.concat(chunks).toString('utf8')) as unknown;
          validatePosePresetDocument(document);
          writeFileSync(posePresetsFile, `${JSON.stringify(document, null, 2)}\n`, 'utf8');
          response.end(JSON.stringify(document));
        } catch (error) {
          response.statusCode = 400;
          response.end(JSON.stringify({ message: error instanceof Error ? error.message : '姿态预设保存失败' }));
        }
      });
    },
    generateBundle() {
      if (hasDefaultModel && existsSync(posePresetsFile)) this.emitFile({ type: 'asset', fileName: 'pose-presets.json', source: readFileSync(posePresetsFile, 'utf8') });
    },
  };
}

function validatePosePresetDocument(value: unknown): asserts value is Record<string, unknown> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('姿态预设文档必须是对象');
  const document = value as Record<string, unknown>;
  if (document.schemaVersion !== 'aidc.model3d.pose-presets/0.1') throw new Error('姿态预设 Schema 版本不受支持');
  if (document.modelId !== manifest.modelId || document.modelRevision !== manifest.revision) throw new Error('姿态预设与当前模型版本不一致');
  if (!Array.isArray(document.poses)) throw new Error('姿态预设列表无效');
  const ids = new Set<string>(); const names = new Set<string>();
  for (const value of document.poses) {
    if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('姿态预设条目无效');
    const pose = value as Record<string, unknown>;
    if (typeof pose.poseId !== 'string' || !pose.poseId || ids.has(pose.poseId)) throw new Error('姿态 ID 缺失或重复');
    if (typeof pose.name !== 'string' || !pose.name.trim() || pose.name.length > 40 || names.has(pose.name)) throw new Error('姿态名称缺失、过长或重复');
    if (!pose.jointValues || typeof pose.jointValues !== 'object' || Array.isArray(pose.jointValues)
      || !Object.values(pose.jointValues).every((entry) => typeof entry === 'number' && Number.isFinite(entry))) throw new Error('姿态关节值无效');
    if (typeof pose.updatedAt !== 'string' || !pose.updatedAt) throw new Error('姿态更新时间无效');
    ids.add(pose.poseId); names.add(pose.name);
  }
}
