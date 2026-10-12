import { useEffect, useMemo, useRef, useState } from 'react';
import type { KeyboardEvent as ReactKeyboardEvent, PointerEvent as ReactPointerEvent } from 'react';
import { Activity, AlertTriangle, Box, Camera, CheckCircle2, ChevronDown, ChevronRight, Crosshair, Download, Eye, EyeOff, Focus, Folder, Maximize2, Move3d, Play, RefreshCw, RotateCcw, Save, ShieldAlert, Tags, Upload, Wrench } from 'lucide-react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import {
  INTERFERENCE_REPORT_SCHEMA_VERSION, POSE_PRESETS_SCHEMA_VERSION,
  type InterferencePairResult, type Model3DKinematics, type Model3DManifest,
  type Model3DPosePreset, type Model3DPosePresetDocument,
} from '@aidc/model3d-contracts';
import { applyKinematics, ExplosionRig, inferProductStructure, loadRobotModel, RobotRig, type ExplosionDepth } from '@aidc/model3d-three-runtime';
import { interpolatePose, isAtJointLimit, jointLabel, standardPosePresets, type JointPose } from './motion';
import { collectCollisionProxies, detectInterferences, formatDistance, refreshCollisionProxies, type CollisionProxy } from './collision';
import { exactCheck } from './exactCheck';
import {
  applyDisplayState, componentIdFromObject, componentObjects, defaultModel3DViewState, disposeObject3D,
  parseModel3DViewState, prepareMaterials, type Model3DViewState,
} from './runtime';

type LoadStatus = { phase: 'loading' | 'ready' | 'error'; message: string; progress?: number };
type AssemblyExportStatus = { phase: 'idle' | 'exporting' | 'success' | 'error'; message?: string };
type ProductEntry = { nodeId: string; label: string; nodeType: 'collection' | 'part'; parentNodeId?: string; nodeName: string; bodyId: string };
type ProductRow = ProductEntry & { depth: number; hasChildren: boolean };

interface ViewerEnvironment {
  scene: THREE.Scene;
  renderer: THREE.WebGLRenderer;
  perspective: THREE.PerspectiveCamera;
  activeCamera: THREE.PerspectiveCamera | THREE.OrthographicCamera;
  controls: OrbitControls;
  displayRoot?: THREE.Object3D;
  modelRoot?: THREE.Object3D;
  components: Map<string, THREE.Object3D>;
  productNodes: Map<string, THREE.Object3D>;
  productTargets: Map<string, THREE.Object3D[]>;
  bodies: Map<string, THREE.Object3D>;
  rig?: RobotRig;
  explosionRig?: ExplosionRig;
  bomLabels?: THREE.Group;
  ghostRoot?: THREE.Object3D;
  collisionProxies: CollisionProxy[];
  collisionHelpers: THREE.Box3Helper[];
  motionFrame?: number;
  selectionMeshes: THREE.Mesh[];
  floor: THREE.Mesh;
  grid: THREE.GridHelper;
  frame: number;
}

export default function Model3DBrowserView() {
  const hostRef = useRef<HTMLDivElement>(null);
  const fileRef = useRef<HTMLInputElement>(null);
  const envRef = useRef<ViewerEnvironment | undefined>(undefined);
  const initial = useMemo(() => parseModel3DViewState(readSessionState()), []);
  const manifestUrl = '/model3d.manifest.json';
  const [localFiles, setLocalFiles] = useState<File[]>();
  const [status, setStatus] = useState<LoadStatus>({ phase: 'loading', message: '正在准备三维场景…' });
  const [assemblyExport, setAssemblyExport] = useState<AssemblyExportStatus>({ phase: 'idle' });
  const [manifest, setManifest] = useState<Model3DManifest>();
  const [productEntries, setProductEntries] = useState<ProductEntry[]>([]);
  const [selectedNodeId, setSelectedNodeId] = useState<string>();
  const [expandedNodes, setExpandedNodes] = useState(() => new Set<string>());
  const [selectedComponentId, setSelectedComponentId] = useState(initial.selectedComponentId ?? new URLSearchParams(window.location.search).get('componentId') ?? undefined);
  const [hidden, setHidden] = useState(() => new Set(initial.hiddenComponentIds));
  const [isolated, setIsolated] = useState(initial.isolatedComponentId);
  const [transparent, setTransparent] = useState(initial.transparent);
  const [jointValues, setJointValues] = useState<JointPose>(initial.jointValues);
  const [selectedJointId, setSelectedJointId] = useState(initial.selectedJointId);
  const [motionPanel, setMotionPanel] = useState(false);
  const [collisionPanel, setCollisionPanel] = useState(false);
  const [assemblyPanel, setAssemblyPanel] = useState(false);
  const [collisionEnabled, setCollisionEnabled] = useState(false);
  const [collisionResults, setCollisionResults] = useState<InterferencePairResult[]>([]);
  const [collisionFilter, setCollisionFilter] = useState<'active' | 'all'>('active');
  const [selectedPairId, setSelectedPairId] = useState<string>();
  const [exactChecking, setExactChecking] = useState(false);
  const [exactMessage, setExactMessage] = useState('');
  const [explosionFactor, setExplosionFactor] = useState(0);
  const [explosionScopeNodeId, setExplosionScopeNodeId] = useState('');
  const [explosionDepth, setExplosionDepth] = useState<ExplosionDepth>('current');
  const [explosionSpacing, setExplosionSpacing] = useState(1);
  const [bomLabelsEnabled, setBomLabelsEnabled] = useState(initial.bomLabelsEnabled);
  const [leaderLinesEnabled, setLeaderLinesEnabled] = useState(true);
  const [assemblyTick, setAssemblyTick] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [presetId, setPresetId] = useState('neutral');
  const [savedPoseName, setSavedPoseName] = useState('');
  const [posePresetDocument, setPosePresetDocument] = useState<Model3DPosePresetDocument>();
  const [posePresetWritable, setPosePresetWritable] = useState(false);
  const [motionMessage, setMotionMessage] = useState('');
  const [search, setSearch] = useState('');
  const [leftSidebarWidth, setLeftSidebarWidth] = useState(210);
  const [rightPropertiesWidth, setRightPropertiesWidth] = useState(225);
  const [rightToolWidth, setRightToolWidth] = useState(330);
  const liveStateRef = useRef({ selectedComponentId, hidden, isolated, transparent, jointValues, selectedJointId, bomLabelsEnabled });
  liveStateRef.current = { selectedComponentId, hidden, isolated, transparent, jointValues, selectedJointId, bomLabelsEnabled };
  const pickingStateRef = useRef({ assemblyPanel, explosionFactor, productEntries });
  pickingStateRef.current = { assemblyPanel, explosionFactor, productEntries };
  const toolPanelOpen = motionPanel || collisionPanel || assemblyPanel;
  const rightSidebarWidth = toolPanelOpen ? rightToolWidth : rightPropertiesWidth;

  const clampSidebarWidth = (width: number, oppositeWidth: number, side: 'left' | 'right') => {
    const minimum = side === 'left' ? 160 : 190;
    const available = Math.max(minimum, window.innerWidth - oppositeWidth - 380);
    return Math.round(Math.min(520, available, Math.max(minimum, width)));
  };

  const updateSidebarWidth = (side: 'left' | 'right', width: number) => {
    if (side === 'left') {
      setLeftSidebarWidth(clampSidebarWidth(width, rightSidebarWidth, side));
      return;
    }
    const next = clampSidebarWidth(width, leftSidebarWidth, side);
    if (toolPanelOpen) setRightToolWidth(next);
    else setRightPropertiesWidth(next);
  };

  const beginSidebarResize = (side: 'left' | 'right', event: ReactPointerEvent<HTMLDivElement>) => {
    if (event.button !== 0) return;
    event.preventDefault();
    const startX = event.clientX;
    const startWidth = side === 'left' ? leftSidebarWidth : rightSidebarWidth;
    document.body.classList.add('model3d-resizing-sidebar');
    const handleMove = (moveEvent: PointerEvent) => {
      const delta = moveEvent.clientX - startX;
      updateSidebarWidth(side, startWidth + (side === 'left' ? delta : -delta));
    };
    const handleUp = () => {
      document.body.classList.remove('model3d-resizing-sidebar');
      window.removeEventListener('pointermove', handleMove);
      window.removeEventListener('pointerup', handleUp);
      window.removeEventListener('pointercancel', handleUp);
    };
    window.addEventListener('pointermove', handleMove);
    window.addEventListener('pointerup', handleUp);
    window.addEventListener('pointercancel', handleUp);
  };

  const resizeSidebarWithKeyboard = (side: 'left' | 'right', event: ReactKeyboardEvent<HTMLDivElement>) => {
    if (event.key !== 'ArrowLeft' && event.key !== 'ArrowRight') return;
    event.preventDefault();
    const direction = event.key === 'ArrowRight' ? 1 : -1;
    const current = side === 'left' ? leftSidebarWidth : rightSidebarWidth;
    updateSidebarWidth(side, current + direction * (side === 'left' ? 12 : -12));
  };

  const captureViewState = (): Model3DViewState => {
    const env = envRef.current;
    const current = liveStateRef.current;
    return {
      projection: 'perspective',
      selectedComponentId: current.selectedComponentId,
      hiddenComponentIds: [...current.hidden],
      isolatedComponentId: current.isolated,
      transparent: current.transparent,
      wireframe: false,
      jointValues: current.jointValues,
      selectedJointId: current.selectedJointId,
      ghostEnabled: false,
      explosionFactor: 0,
      bomLabelsEnabled: current.bomLabelsEnabled,
    };
  };

  useEffect(() => {
    saveSessionState(captureViewState());
    const env = envRef.current;
    if (env?.modelRoot) applyProductDisplayState(env, { hiddenComponentIds: [...hidden], isolatedComponentId: isolated, transparent, wireframe: false });
  }, [hidden, isolated, transparent, selectedComponentId, jointValues, selectedJointId, bomLabelsEnabled]);

  useEffect(() => {
    const env = envRef.current;
    if (!env) return;
    const selected = productEntries.find((item) => item.nodeId === selectedNodeId);
    selectObjects(env, selected && !selected.parentNodeId ? [] : env.productTargets.get(selectedNodeId ?? '') ?? []);
  }, [selectedNodeId, productEntries]);

  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;
    const controller = new AbortController();
    let disposed = false;
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xedf2f5);
    const renderer = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true });
    renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.08;
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    host.replaceChildren(renderer.domElement);
    const perspective = new THREE.PerspectiveCamera(38, 1, 0.001, 100);
    const activeCamera = perspective;
    activeCamera.position.set(0.45, 0.34, 0.5);
    const controls = new OrbitControls(activeCamera, renderer.domElement);
    controls.enableDamping = true; controls.screenSpacePanning = true;
    scene.add(new THREE.HemisphereLight(0xf7fbff, 0x52606b, 1.05));
    const key = new THREE.DirectionalLight(0xffffff, 2.35);
    key.position.set(2.2, 3.4, 2.8); key.castShadow = true;
    key.shadow.mapSize.set(2048, 2048); key.shadow.bias = -0.00015; key.shadow.normalBias = 0.015;
    key.shadow.camera.near = .1; key.shadow.camera.far = 8;
    key.shadow.camera.left = -1.2; key.shadow.camera.right = 1.2; key.shadow.camera.top = 1.2; key.shadow.camera.bottom = -1.2;
    scene.add(key);
    const fill = new THREE.DirectionalLight(0xbfd8ee, .7); fill.position.set(-2.4, 1.2, 1.6); scene.add(fill);
    const rim = new THREE.DirectionalLight(0xfff1dd, .5); rim.position.set(.2, 2.2, -3); scene.add(rim);
    const grid = new THREE.GridHelper(1.5, 30, 0x9baab3, 0xd5dde2); scene.add(grid);
    const floor = new THREE.Mesh(new THREE.PlaneGeometry(4, 4), new THREE.ShadowMaterial({ color: 0x34424f, opacity: .16 }));
    floor.rotation.x = -Math.PI / 2; floor.receiveShadow = true; floor.renderOrder = -1; scene.add(floor);
    const env: ViewerEnvironment = { scene, renderer, perspective, activeCamera, controls, components: new Map(), productNodes: new Map(), productTargets: new Map(), bodies: new Map(), collisionProxies: [], collisionHelpers: [], selectionMeshes: [], floor, grid, frame: 0 };
    envRef.current = env;
    const resize = () => resizeViewer(env, host);
    const observer = new ResizeObserver(resize); observer.observe(host); resize();
    const render = () => { env.frame = requestAnimationFrame(render); controls.update(); renderer.render(scene, env.activeCamera); };
    render();
    let pointerDown: THREE.Vector2 | undefined;
    const rememberPointerDown = (event: PointerEvent) => { pointerDown = new THREE.Vector2(event.clientX, event.clientY); };
    const pick = (event: MouseEvent) => {
      if (!env.modelRoot) return;
      if (event.button !== 0 || !pointerDown || pointerDown.distanceTo(new THREE.Vector2(event.clientX, event.clientY)) > 5) return;
      const bounds = renderer.domElement.getBoundingClientRect();
      const pointer = new THREE.Vector2((event.clientX - bounds.left) / bounds.width * 2 - 1, -(event.clientY - bounds.top) / bounds.height * 2 + 1);
      const hits = new THREE.Raycaster(); hits.setFromCamera(pointer, env.activeCamera);
      const hit = hits.intersectObject(env.modelRoot, true).find((item) => item.object instanceof THREE.Mesh
        && item.object.visible && item.object.userData.aidcIgnorePicking !== true);
      const hitNodeId = componentIdFromObject(hit?.object ?? null, env.productNodes);
      if (!hitNodeId) return;
      const pickingState = pickingStateRef.current;
      const targetNodeId = pickingState.assemblyPanel && pickingState.explosionFactor > 0
        ? resolveExplosionTarget(hitNodeId, pickingState.productEntries, env.explosionRig?.nodeIds ?? []) ?? hitNodeId
        : hitNodeId;
      chooseProductNode(targetNodeId);
    };
    renderer.domElement.addEventListener('pointerdown', rememberPointerDown);
    renderer.domElement.addEventListener('click', pick);

    const load = async () => {
      setStatus({ phase: 'loading', message: '正在加载三维模型…', progress: 0 });
      try {
        const result = localFiles?.length
          ? await loadLocalFiles(localFiles, (loaded, total) => setStatus({ phase: 'loading', message: '正在加载本地模型…', progress: total ? loaded / total : undefined }))
          : await loadRegistered(manifestUrl, controller.signal, (loaded, total) => setStatus({ phase: 'loading', message: '正在加载模型网格…', progress: total ? loaded / total : undefined }));
        if (!result) throw new Error('没有可加载的三维模型包');
        if (disposed) { disposeObject3D(result.displayRoot); return; }
        env.displayRoot = result.displayRoot; env.modelRoot = result.modelRoot; scene.add(result.displayRoot);
        env.rig = result.rig;
        prepareMaterials(result.modelRoot);
        alignGroundPlane(env, result.displayRoot);
        env.components = result.manifest ? componentObjects(result.modelRoot, result.manifest) : genericComponents(result.modelRoot);
        const genericProduct = result.manifest ? undefined : inferProductStructure(result.modelRoot);
        env.productNodes = result.manifest
          ? new Map(result.manifest.productStructure.flatMap((entry) => {
              const object = result.modelRoot.getObjectByName(entry.nodeName);
              return object ? [[entry.nodeId, object] as const] : [];
            }))
          : genericProduct!.nodes;
        env.bodies = result.manifest
          ? new Map(result.manifest.bodies.flatMap((body) => {
              const object = result.modelRoot.getObjectByName(body.nodeName);
              return object ? [[body.bodyId, object] as const] : [];
            }))
          : genericComponents(result.modelRoot);
        env.collisionProxies = result.manifest ? collectCollisionProxies(result.manifest, env.components) : [];
        const nextEntries: ProductEntry[] = result.manifest
          ? result.manifest.productStructure
          : genericProduct!.entries;
        env.explosionRig = new ExplosionRig(nextEntries, result.modelRoot, env.productNodes);
        const rootNodeId = nextEntries.find((entry) => !entry.parentNodeId)?.nodeId;
        if (rootNodeId) env.explosionRig?.layout(rootNodeId, 'current', env.activeCamera, 1);
        env.productTargets = buildProductTargets(nextEntries, env.productNodes);
        setManifest(result.manifest); setProductEntries(nextEntries);
        setCollisionEnabled(false); setCollisionResults([]); setSelectedPairId(undefined);
        setPosePresetDocument(result.posePresets); setPosePresetWritable(result.posePresetWritable === true);
        setPresetId('neutral'); setSavedPoseName('');
        setExpandedNodes(new Set(nextEntries.filter((entry) => entry.nodeId === rootNodeId || entry.parentNodeId === rootNodeId).map((entry) => entry.nodeId)));
        setExplosionScopeNodeId(rootNodeId ?? ''); setExplosionDepth('current'); setExplosionSpacing(1); setExplosionFactor(0);
        setSelectedNodeId((current) => nextEntries.some((entry) => entry.nodeId === current) ? current : rootNodeId);
        if (result.rig) {
          const restored = Object.keys(initial.jointValues).length ? result.rig.setPose(initial.jointValues) : result.rig.getPose();
          const pose = { ...result.rig.getPose(), ...restored };
          setJointValues(pose);
          setSelectedJointId((current) => current && result.rig!.jointIds.includes(current) ? current : result.rig!.jointIds[0]);
        } else {
          setJointValues({}); setSelectedJointId(undefined);
        }
        applyProductDisplayState(env, { hiddenComponentIds: [...hidden], isolatedComponentId: isolated, transparent, wireframe: false });
        if (initial.bomLabelsEnabled && result.manifest) createBomLabels(env, nextEntries, true, rootNodeId);
        fitViewer(env, result.displayRoot);
        setStatus({ phase: 'ready', message: `${nextEntries.length} 个产品结构节点` });
      } catch (error) {
        if (!disposed && !(error instanceof DOMException && error.name === 'AbortError')) setStatus({ phase: 'error', message: error instanceof Error ? error.message : '模型加载失败' });
      }
    };
    void load();
    return () => {
      disposed = true; controller.abort(); observer.disconnect(); controls.dispose();
      renderer.domElement.removeEventListener('pointerdown', rememberPointerDown); renderer.domElement.removeEventListener('click', pick); cancelAnimationFrame(env.frame); if (env.motionFrame) cancelAnimationFrame(env.motionFrame);
      clearGhost(env); clearCollisionHighlights(env); clearBomLabels(env);
      if (env.displayRoot) disposeObject3D(env.displayRoot); renderer.dispose(); renderer.forceContextLoss(); host.replaceChildren();
      env.floor.geometry.dispose();
      const floorMaterials = Array.isArray(env.floor.material) ? env.floor.material : [env.floor.material];
      floorMaterials.forEach((material) => material.dispose());
      env.grid.geometry.dispose();
      const gridMaterials = Array.isArray(env.grid.material) ? env.grid.material : [env.grid.material];
      gridMaterials.forEach((material) => material.dispose());
      if (envRef.current === env) envRef.current = undefined;
    };
  }, [manifestUrl, localFiles]);

  const chooseComponent = (componentId: string) => {
    setSelectedComponentId(componentId);
    if (manifest?.joints.some((joint) => joint.jointId === componentId)) setSelectedJointId(componentId);
  };
  const chooseProductNode = (nodeId: string) => setSelectedNodeId(nodeId);
  const selectedProductEntry = productEntries.find((item) => item.nodeId === selectedNodeId);
  const productRows = useMemo(() => flattenProductTree(productEntries, expandedNodes, search), [productEntries, expandedNodes, search]);
  const presets = useMemo(() => manifest ? standardPosePresets(manifest) : [], [manifest]);
  const poseOptions = useMemo(() => [
    ...presets,
    ...(posePresetDocument?.poses ?? []).map((pose) => ({ id: `saved:${pose.poseId}`, label: pose.name, pose: pose.jointValues })),
  ], [presets, posePresetDocument]);

  const applyJointValue = (jointId: string, requestedValue: number) => {
    const env = envRef.current; const joint = manifest?.joints.find((item) => item.jointId === jointId);
    if (!env?.rig || !joint || !Number.isFinite(requestedValue)) return;
    if (env.motionFrame) { cancelAnimationFrame(env.motionFrame); env.motionFrame = undefined; setPlaying(false); }
    env.rig.setJoint(jointId, requestedValue);
    setJointValues(env.rig.getPose()); setSelectedJointId(jointId); chooseComponent(jointId);
    setMotionMessage('');
  };

  const animateToPose = (target: JointPose, label: string) => {
    const env = envRef.current;
    if (!env?.rig || !manifest) return;
    if (env.motionFrame) cancelAnimationFrame(env.motionFrame);
    clearGhost(env);
    const from = env.rig.getPose();
    const bounded = Object.fromEntries(manifest.joints.map((joint) => [joint.jointId, target[joint.jointId] ?? from[joint.jointId] ?? joint.neutralValue]));
    const duration = motionDuration(manifest, from, bounded);
    const started = performance.now(); setPlaying(true); setMotionMessage(`${label} · ${Math.round(duration)} ms`);
    const step = (now: number) => {
      const progress = Math.min(1, (now - started) / duration);
      env.rig!.setPose(interpolatePose(from, bounded, progress));
      setJointValues(env.rig!.getPose());
      if (progress < 1) env.motionFrame = requestAnimationFrame(step);
      else { env.motionFrame = undefined; setPlaying(false); setMotionMessage(`${label}已完成`); }
    };
    env.motionFrame = requestAnimationFrame(step);
  };

  const resetPose = () => {
    const preset = presets.find((item) => item.id === 'neutral'); if (preset) animateToPose(preset.pose, preset.label);
  };
  const savePose = async () => {
    const pose = envRef.current?.rig?.getPose(); const label = savedPoseName.trim();
    if (!pose || !label) { setMotionMessage('请先输入姿态名称'); return; }
    if (!manifest || !posePresetDocument || !posePresetWritable) { setMotionMessage('当前模型工程为只读，不能保存姿态'); return; }
    const existing = posePresetDocument.poses.find((item) => item.name === label);
    const saved: Model3DPosePreset = { poseId: existing?.poseId ?? createPoseId(), name: label, jointValues: pose, updatedAt: new Date().toISOString() };
    const poses = existing ? posePresetDocument.poses.map((item) => item.poseId === existing.poseId ? saved : item) : [...posePresetDocument.poses, saved];
    const next: Model3DPosePresetDocument = { ...posePresetDocument, poses };
    try {
      const persisted = await saveProjectPosePresets(manifest.modelId, next);
      setPosePresetDocument(persisted); setPresetId(`saved:${saved.poseId}`); setSavedPoseName('');
      setMotionMessage(existing ? `已更新工程姿态“${label}”` : `已保存工程姿态“${label}”`);
    } catch (error) {
      setMotionMessage(error instanceof Error ? error.message : '姿态保存失败');
    }
  };

  useEffect(() => { if (motionPanel && envRef.current) clearGhost(envRef.current); }, [motionPanel]);
  useEffect(() => {
    const env = envRef.current;
    if (!env || !manifest || !collisionEnabled) { if (env) clearCollisionHighlights(env); return; }
    env.scene.updateMatrixWorld(true);
    refreshCollisionProxies(env.collisionProxies, env.components);
    const next = detectInterferences(env.collisionProxies, manifest, jointValues);
    setCollisionResults((current) => mergeExactResults(next, current));
  }, [jointValues, manifest, collisionEnabled, assemblyTick]);
  useEffect(() => {
    const env = envRef.current; if (!env) return;
    if (!collisionPanel || !selectedPairId) { clearCollisionHighlights(env); return; }
    const visible = collisionResults.filter((item) =>
      (item.status === 'WARNING' || item.status === 'CONFIRMED') && item.pairId === selectedPairId,
    );
    renderCollisionHighlights(env, visible);
  }, [collisionResults, selectedPairId, collisionPanel]);

  const filteredCollisionResults = collisionResults.filter((item) => collisionFilter === 'all' || item.status === 'WARNING' || item.status === 'CONFIRMED' || item.status === 'UNVERIFIED');
  const activeCollisionResults = collisionResults.filter((item) => item.status === 'WARNING' || item.status === 'CONFIRMED');
  const activeCollisionCount = activeCollisionResults.length;
  const selectCollisionPair = (item: InterferencePairResult) => {
    setSelectedPairId(item.pairId ?? `${item.componentA}::${item.componentB}`); setSelectedComponentId(item.componentA);
    const env = envRef.current; if (env) fitViewerToComponents(env, [item.componentA, item.componentB]);
  };
  const selectCollisionComponent = (componentId: string) => {
    setSelectedPairId(undefined);
    setSelectedComponentId(componentId);
    const component = manifest?.components.find((item) => item.componentId === componentId);
    const productEntry = component && productEntries.find((item) => item.nodeType === 'part'
      && (item.nodeName === component.nodeName || item.bodyId === component.bodyId));
    if (productEntry) {
      setSelectedNodeId(productEntry.nodeId);
      setExpandedNodes((current) => {
        const next = new Set(current);
        const byId = new Map(productEntries.map((item) => [item.nodeId, item]));
        let parentId = productEntry.parentNodeId;
        while (parentId) {
          next.add(parentId);
          parentId = byId.get(parentId)?.parentNodeId;
        }
        return next;
      });
    }
    const env = envRef.current; const object = env?.components.get(componentId);
    if (env && object) {
      selectObjects(env, [object]);
    }
  };
  const locateInterferencePose = (item: InterferencePairResult) => {
    selectCollisionPair(item);
    if (item.pose) animateToPose(item.pose, '干涉姿态');
  };
  const checkExact = async (item: InterferencePairResult) => {
    const env = envRef.current; if (!env || !manifest) return;
    const componentA = manifest.components.find((entry) => entry.componentId === item.componentA);
    const componentB = manifest.components.find((entry) => entry.componentId === item.componentB);
    const objectA = env.components.get(item.componentA); const objectB = env.components.get(item.componentB);
    if (!componentA || !componentB || !objectA || !objectB) return;
    objectA.updateWorldMatrix(true, false); objectB.updateWorldMatrix(true, false);
    setExactChecking(true); setExactMessage('正在调用内置精确校核引擎…');
    try {
      const result = await exactCheck({
        schemaVersion: INTERFERENCE_REPORT_SCHEMA_VERSION, modelId: manifest.modelId, modelRevision: manifest.revision,
        cadSha256: manifest.source?.cadSha256 ?? manifest.assets.find((asset) => asset.role === 'visual')?.sha256 ?? '0'.repeat(64), tolerance: manifest.collision?.tolerance ?? 0,
        componentA: { componentId: item.componentA, sourceObject: componentA.sourceObject, matrixWorld: objectA.matrixWorld.toArray() },
        componentB: { componentId: item.componentB, sourceObject: componentB.sourceObject, matrixWorld: objectB.matrixWorld.toArray() },
        pose: { ...jointValues },
      });
      setCollisionResults((current) => current.map((entry) => entry.pairId === item.pairId ? result : entry));
      setExactMessage(result.message ?? '精确校核完成');
    } catch (error) {
      setCollisionResults((current) => current.map((entry) => entry.pairId === item.pairId ? { ...entry, status: 'UNVERIFIED', message: '内置精确校核引擎不可用', checkedAt: new Date().toISOString() } : entry));
      setExactMessage(error instanceof Error ? `${error.message}；请重启三维模型浏览器` : '内置精确校核引擎不可用');
    } finally { setExactChecking(false); }
  };
  const selectedExplosionTargetId = resolveExplosionTarget(selectedNodeId, productEntries, envRef.current?.explosionRig?.nodeIds ?? []);
  const selectedExplosionTarget = productEntries.find((entry) => entry.nodeId === selectedExplosionTargetId);
  const setExplodeValue = (factor: number) => {
    const env = envRef.current; if (!env?.explosionRig) return;
    env.explosionRig.setExplode(factor);
    setExplosionFactor(factor); setAssemblyTick((value) => value + 1);
  };
  const updateExplodeDirection = (axis: 0 | 1 | 2, value: number) => {
    const rig = envRef.current?.explosionRig; if (!rig || !selectedExplosionTargetId || !Number.isFinite(value)) return;
    const direction = rig.getDirection(selectedExplosionTargetId); if (!direction) return;
    direction[axis] = value; rig.setDirection(selectedExplosionTargetId, direction); rig.setExplode(explosionFactor);
    setAssemblyTick((tick) => tick + 1);
  };
  const restoreSelectedExplosionTarget = () => {
    const rig = envRef.current?.explosionRig;
    if (!rig || !selectedExplosionTargetId || explosionFactor <= 0) return;
    if (rig.restoreNode(selectedExplosionTargetId)) setAssemblyTick((tick) => tick + 1);
  };
  const fitExplosionView = () => {
    const env = envRef.current;
    if (env?.displayRoot) fitViewer(env, env.displayRoot);
  };
  const layoutExplosion = (scopeNodeId = explosionScopeNodeId, depth = explosionDepth, spacing = explosionSpacing) => {
    const env = envRef.current; if (!env?.explosionRig || !scopeNodeId) return;
    env.explosionRig.layout(scopeNodeId, depth, env.activeCamera, spacing);
    env.explosionRig.setExplode(explosionFactor);
    setExplosionScopeNodeId(scopeNodeId); setExplosionDepth(depth); setExplosionSpacing(spacing); setAssemblyTick((tick) => tick + 1);
  };
  const enterExplosionScope = (entry: ProductEntry) => {
    if (entry.nodeType !== 'collection') return;
    setAssemblyPanel(true); setMotionPanel(false); setCollisionPanel(false);
    setSelectedNodeId(entry.nodeId); layoutExplosion(entry.nodeId, explosionDepth, explosionSpacing);
  };
  const resetAssemblyWorkspace = () => {
    const env = envRef.current;
    if (env) clearGhost(env);
    env?.explosionRig?.reset();
    setExplosionFactor(0); setAssemblyTick((tick) => tick + 1);
  };
  const closeAssemblyWorkspace = () => {
    if (assemblyPanel) resetAssemblyWorkspace();
    setAssemblyPanel(false);
  };
  const selectedExplodeDirection = selectedExplosionTargetId ? envRef.current?.explosionRig?.getDirection(selectedExplosionTargetId) : undefined;
  const selectedExplosionTargetRestored = selectedExplosionTargetId ? envRef.current?.explosionRig?.isNodeRestored(selectedExplosionTargetId) === true : false;
  const explosionScopes = productEntries.filter((entry) => entry.nodeType === 'collection');
  const explosionPlan = envRef.current?.explosionRig?.plan;

  async function exportModelicaAssembly() {
    if (!manifest || assemblyExport.phase === 'exporting') return;
    setAssemblyExport({ phase: 'exporting', message: '正在校验三维拓扑与 Robot IR…' });
    try {
      const response = await fetch(`/api/model-projects/${encodeURIComponent(manifest.modelId)}/modelica-assembly`, { method: 'POST' });
      const payload = await response.json() as { output?: string; bodyCount?: number; jointCount?: number; message?: string };
      if (!response.ok) throw new Error(payload.message || '装配包导出失败');
      setAssemblyExport({ phase: 'success', message: `已导出 ${payload.bodyCount ?? 0} 刚体 / ${payload.jointCount ?? 0} 关节：${payload.output}` });
    } catch (error) {
      setAssemblyExport({ phase: 'error', message: error instanceof Error ? error.message : '装配包导出失败' });
    }
  }

  useEffect(() => {
    const env = envRef.current; if (!env) return;
    if (bomLabelsEnabled) createBomLabels(env, productEntries, leaderLinesEnabled, selectedNodeId); else clearBomLabels(env);
  }, [bomLabelsEnabled, leaderLinesEnabled, selectedNodeId, manifest, productEntries, explosionScopeNodeId, explosionDepth]);

  return <div className="model3d-view">
    <div className="model3d-toolbar">
      <div className="model3d-status"><Box size={15} /><span><strong>{manifest?.name ?? (localFiles ? '本地三维模型' : '三维模型')}</strong><small>{status.message}</small></span>{status.phase === 'loading' && <RefreshCw size={14} className="spin" />}</div>
      <div className="model3d-tools">
        <button onClick={() => setStandardView(envRef.current, 'iso')} title="等轴测"><Focus size={14} />等轴测</button>
        <button onClick={() => setStandardView(envRef.current, 'front')}>前</button><button onClick={() => setStandardView(envRef.current, 'left')}>左</button><button onClick={() => setStandardView(envRef.current, 'top')}>顶</button>
        <button onClick={() => envRef.current?.displayRoot && fitViewer(envRef.current, envRef.current.displayRoot)} title="适应窗口"><Maximize2 size={14} /></button>
        <button className={transparent ? 'active' : ''} onClick={() => setTransparent((value) => !value)}>透明</button>
        <button className={motionPanel ? 'active' : ''} disabled={!manifest?.joints.length} onClick={() => { const next = !motionPanel; closeAssemblyWorkspace(); setMotionPanel(next); setCollisionPanel(false); }}><Activity size={14} />运动学</button>
        <button className={collisionPanel ? 'active' : ''} disabled={!manifest?.collision} onClick={() => { const next = !collisionPanel; closeAssemblyWorkspace(); setCollisionPanel(next); setMotionPanel(false); }}><ShieldAlert size={14} />干涉 {activeCollisionCount}</button>
        <button className={assemblyPanel ? 'active' : ''} disabled={!productEntries.length} onClick={() => { if (assemblyPanel) closeAssemblyWorkspace(); else setAssemblyPanel(true); setMotionPanel(false); setCollisionPanel(false); }}><Wrench size={14} />装配</button>
        <button onClick={() => downloadScreenshot(envRef.current)} title="保存截图"><Camera size={14} /></button>
        <button disabled={!manifest || assemblyExport.phase === 'exporting' || Boolean(localFiles)} onClick={() => void exportModelicaAssembly()} title={localFiles ? '本地临时模型不能导出工程装配包' : '校验并导出 Modelica 最小装配包'}>{assemblyExport.phase === 'exporting' ? <RefreshCw size={14} className="spin" /> : <Download size={14} />}导出 Modelica</button>
        <button onClick={() => fileRef.current?.click()}><Upload size={14} />打开模型</button>
        <input ref={fileRef} hidden multiple type="file" accept=".json,.glb,.gltf,model/gltf-binary,model/gltf+json" onChange={(event) => { const files = [...(event.target.files ?? [])]; if (files.length) setLocalFiles(files); event.target.value = ''; }} />
      </div>
    </div>
    {assemblyExport.phase !== 'idle' && <div className={`model3d-export-message ${assemblyExport.phase}`}>{assemblyExport.phase === 'success' ? <CheckCircle2 size={14} /> : assemblyExport.phase === 'error' ? <AlertTriangle size={14} /> : <RefreshCw size={14} className="spin" />}<span>{assemblyExport.message}</span>{assemblyExport.phase !== 'exporting' && <button onClick={() => setAssemblyExport({ phase: 'idle' })}>关闭</button>}</div>}
    <div className={`model3d-main ${toolPanelOpen ? 'motion-mode' : ''}`} style={{ gridTemplateColumns: `${leftSidebarWidth}px 7px minmax(0,1fr) 7px ${rightSidebarWidth}px` }}>
      <aside className="model3d-tree"><div className="model3d-panel-head"><strong>CAD 产品结构</strong><span>{productEntries.length}</span></div><input value={search} onChange={(event) => setSearch(event.target.value)} placeholder="搜索集合或部件…" /><div className="model3d-component-list">{productRows.map((entry) => <div key={entry.nodeId} className={entry.nodeId === selectedNodeId ? 'selected' : ''} style={{ paddingLeft: entry.depth * 12 }}><button className="model3d-tree-toggle" disabled={!entry.hasChildren} title={expandedNodes.has(entry.nodeId) ? '折叠' : '展开'} onClick={() => setExpandedNodes((current) => { const next = new Set(current); next.has(entry.nodeId) ? next.delete(entry.nodeId) : next.add(entry.nodeId); return next; })}>{entry.hasChildren ? expandedNodes.has(entry.nodeId) ? <ChevronDown size={12} /> : <ChevronRight size={12} /> : <span />}</button><button className="model3d-component" title={entry.nodeType === 'collection' ? '双击以此集合作为爆炸范围' : undefined} onClick={() => chooseProductNode(entry.nodeId)} onDoubleClick={() => enterExplosionScope(entry)}>{entry.nodeType === 'collection' ? <Folder size={13} /> : <Box size={13} />}<span><strong>{entry.label}</strong><small>{entry.nodeName}</small></span></button><button className="model3d-eye" title={hidden.has(entry.nodeId) ? '显示' : '隐藏'} onClick={() => setHidden((current) => { const next = new Set(current); next.has(entry.nodeId) ? next.delete(entry.nodeId) : next.add(entry.nodeId); return next; })}>{hidden.has(entry.nodeId) ? <EyeOff size={13} /> : <Eye size={13} />}</button></div>)}</div></aside>
      <div className="model3d-sidebar-resizer left" role="separator" aria-label="调整 CAD 产品结构栏宽度" aria-orientation="vertical" aria-valuemin={160} aria-valuemax={520} aria-valuenow={leftSidebarWidth} tabIndex={0} onPointerDown={(event) => beginSidebarResize('left', event)} onKeyDown={(event) => resizeSidebarWithKeyboard('left', event)} />
      <div className="model3d-canvas-wrap"><div ref={hostRef} className="model3d-canvas" />{status.phase === 'loading' && <div className="model3d-loading"><RefreshCw className="spin" /><strong>{status.message}</strong>{status.progress != null && <><progress max="1" value={status.progress} /><span>{Math.round(status.progress * 100)}%</span></>}</div>}{status.phase === 'error' && <div className="model3d-error"><AlertTriangle /><strong>无法打开三维模型</strong><span>{status.message}</span><button onClick={() => fileRef.current?.click()}>选择 GLB、glTF 或模型包文件</button></div>}{motionPanel && manifest?.collision && collisionEnabled && <section className={`model3d-live-collision ${activeCollisionCount ? 'warning' : 'clear'}`}><header><span>{activeCollisionCount ? <ShieldAlert size={14} /> : <CheckCircle2 size={14} />}<strong>{activeCollisionCount ? `${activeCollisionCount} 项实时干涉预警` : '当前姿态未发现干涉'}</strong></span><button onClick={() => setCollisionEnabled(false)}>暂停</button></header>{activeCollisionResults.length > 0 && <div>{activeCollisionResults.slice(0, 3).map((item) => <button key={item.pairId} onClick={() => locateInterferencePose(item)}><span>{item.componentA}</span><i>×</i><span>{item.componentB}</span></button>)}{activeCollisionResults.length > 3 && <small>另有 {activeCollisionResults.length - 3} 项</small>}</div>}<footer>运动时持续进行网格级干涉预警<button onClick={() => { setCollisionPanel(true); setMotionPanel(false); setAssemblyPanel(false); }}>查看详情</button></footer></section>}</div>
      <div className="model3d-sidebar-resizer right" role="separator" aria-label="调整属性与工具栏宽度" aria-orientation="vertical" aria-valuemin={190} aria-valuemax={520} aria-valuenow={rightSidebarWidth} tabIndex={0} onPointerDown={(event) => beginSidebarResize('right', event)} onKeyDown={(event) => resizeSidebarWithKeyboard('right', event)} />
      <aside className={`model3d-properties ${motionPanel || collisionPanel || assemblyPanel ? 'motion-open' : ''}`}>
        <div className="model3d-panel-tabs"><button className={!motionPanel && !collisionPanel && !assemblyPanel ? 'active' : ''} onClick={() => { setMotionPanel(false); setCollisionPanel(false); closeAssemblyWorkspace(); }}>部件</button><button className={motionPanel ? 'active' : ''} disabled={!manifest?.joints.length} onClick={() => { closeAssemblyWorkspace(); setMotionPanel(true); setCollisionPanel(false); }}>运动 {manifest?.joints.length ?? 0}</button><button className={collisionPanel ? 'active' : ''} disabled={!manifest?.collision} onClick={() => { closeAssemblyWorkspace(); setCollisionPanel(true); setMotionPanel(false); }}>干涉 {activeCollisionCount}</button><button className={assemblyPanel ? 'active' : ''} disabled={!productEntries.length} onClick={() => { setAssemblyPanel(true); setMotionPanel(false); setCollisionPanel(false); }}>装配</button></div>
        {assemblyPanel && productEntries.length ? <div className="model3d-assembly-panel">
          <section className="model3d-explode-controls"><header><span><Move3d size={14} />原点径向爆炸</span><strong>{Math.round(explosionFactor * 100)}%</strong></header><label className="wide">范围<select aria-label="爆炸范围" value={explosionScopeNodeId} onChange={(event) => layoutExplosion(event.target.value, explosionDepth, explosionSpacing)}>{explosionScopes.map((entry) => <option key={entry.nodeId} value={entry.nodeId}>{entry.label}</option>)}</select></label><label className="wide">层级<select aria-label="爆炸层级" value={explosionDepth} onChange={(event) => layoutExplosion(explosionScopeNodeId, event.target.value as ExplosionDepth, explosionSpacing)}><option value="current">当前层</option><option value="two-levels">递归两级</option><option value="leaves">全部叶部件</option></select></label><input aria-label="爆炸程度" type="range" min="0" max="1" step="0.01" value={explosionFactor} onChange={(event) => setExplodeValue(Number(event.target.value))} onPointerUp={fitExplosionView} onKeyUp={fitExplosionView} /><label className="wide">间距<input aria-label="爆炸间距" type="range" min="0.5" max="2.5" step="0.05" value={explosionSpacing} onChange={(event) => { const spacing = Number(event.target.value); envRef.current?.explosionRig?.setSpacing(spacing); envRef.current?.explosionRig?.setExplode(explosionFactor); setExplosionSpacing(spacing); setAssemblyTick((tick) => tick + 1); }} onPointerUp={fitExplosionView} onKeyUp={fitExplosionView} /><span>{explosionSpacing.toFixed(2)}×</span></label><button onClick={() => layoutExplosion()}><Focus size={12} />重新计算布局</button><button onClick={() => setExplodeValue(0)}><RotateCcw size={12} />全部归位</button><button className="wide" disabled={!selectedExplosionTargetId || explosionFactor <= 0 || selectedExplosionTargetRestored} title={selectedExplosionTarget ? `归位：${selectedExplosionTarget.label}` : '请先在模型树或三维视图中选择爆炸组件'} onClick={restoreSelectedExplosionTarget}><RotateCcw size={12} />{selectedExplosionTargetRestored ? '所选组件已归位' : selectedExplosionTarget ? `归位所选：${selectedExplosionTarget.label}` : '选择组件后归位'}</button></section>
          <section className="model3d-assembly-options readable"><button className={bomLabelsEnabled ? 'active' : ''} onClick={() => setBomLabelsEnabled((value) => !value)}><Tags size={12} />BOM 标注</button><button className={leaderLinesEnabled ? 'active' : ''} disabled={!bomLabelsEnabled} onClick={() => setLeaderLinesEnabled((value) => !value)}><Move3d size={12} />引出线</button></section>
          {selectedExplodeDirection && <section className="model3d-direction-editor"><strong>人工修正方向 · {selectedExplosionTargetId}</strong><div>{(['X','Y','Z'] as const).map((axis, index) => <label key={axis}>{axis}<input type="number" step="0.1" value={Number(selectedExplodeDirection[index].toFixed(2))} onChange={(event) => updateExplodeDirection(index as 0 | 1 | 2, Number(event.target.value))} /></label>)}</div><small>默认方向来自爆炸原点与组件几何中心的连线；修改后向量自动归一化。</small></section>}
          <div className="model3d-mode-note">爆炸原点取当前范围的几何中心；所有组件沿原点至自身几何中心的射线按统一倍率放大，以保持原有上下、左右和前后顺序。视角只参与屏幕重叠检查。当前层保持子总成刚性，双击模型树集合可进入其内部。{explosionPlan && <strong>原点 [{explosionPlan.origin.map((value) => value.toFixed(3)).join(', ')}] · {explosionPlan.entries.length} 个目标 · 防重叠迭代 {explosionPlan.relaxationIterations} 次{explosionPlan.remainingOverlaps ? ` · 仍有 ${explosionPlan.remainingOverlaps} 处重叠` : ' · 无剩余重叠'}</strong>}</div>
        </div> : collisionPanel && manifest ? <div className="model3d-collision-panel">
          <div className="model3d-collision-summary"><span className={activeCollisionCount ? 'warning' : 'clear'}>{activeCollisionCount ? <ShieldAlert size={16} /> : <CheckCircle2 size={16} />}<strong>{activeCollisionCount ? `${activeCollisionCount} 项待处理` : '当前姿态未发现干涉'}</strong></span><small>实时检测先用包围盒粗筛，再校验三角网格相交与实际间隙；工程结论由内置 FreeCAD 几何引擎给出。</small></div>
          <div className="model3d-collision-controls"><button className={collisionEnabled ? 'active' : ''} onClick={() => setCollisionEnabled((value) => !value)}>{collisionEnabled ? '实时检测中' : '已暂停'}</button><select value={collisionFilter} onChange={(event) => setCollisionFilter(event.target.value as 'active' | 'all')}><option value="active">待处理</option><option value="all">全部结果</option></select></div>
          {exactMessage && <div className="model3d-exact-message">{exactChecking && <RefreshCw size={11} className="spin" />}{exactMessage}</div>}
          <div className="model3d-collision-list">{filteredCollisionResults.map((item) => <article key={item.pairId} className={`${item.status.toLowerCase()} ${item.pairId === selectedPairId ? 'selected' : ''}`} onClick={() => selectCollisionPair(item)}>
            <header><span>{collisionStatusLabel(item.status)}</span><code>{item.precision === 'BREP_EXACT' ? 'B-Rep 精确' : item.precision === 'MESH_EXACT' ? '网格精确' : '实时预警'}</code></header>
            <div className="model3d-collision-objects">
              <button title={`在视图中选择 ${item.componentA}`} onClick={(event) => { event.stopPropagation(); selectCollisionComponent(item.componentA); }}>{item.componentA}</button>
              <i>×</i>
              <button title={`在视图中选择 ${item.componentB}`} onClick={(event) => { event.stopPropagation(); selectCollisionComponent(item.componentB); }}>{item.componentB}</button>
            </div>
            <dl><div><dt>最小间隙</dt><dd>{formatDistance(item.minimumDistance)}</dd></div><div><dt>相交体积</dt><dd>{formatVolume(item.commonVolume)}</dd></div></dl>
            <p>{item.message}</p>
            <footer><button onClick={(event) => { event.stopPropagation(); selectCollisionPair(item); }}><Crosshair size={11} />定位</button><button disabled={!item.pose || playing} onClick={(event) => { event.stopPropagation(); locateInterferencePose(item); }}>定位到干涉姿态</button><button disabled={exactChecking || item.status === 'ALLOWED_CONTACT'} onClick={(event) => { event.stopPropagation(); void checkExact(item); }}>精确校核</button></footer>
          </article>)}{!filteredCollisionResults.length && <div className="model3d-no-selection">当前筛选条件下没有干涉记录</div>}</div>
        </div> : motionPanel && manifest ? <div className="model3d-motion-panel">
          <div className="model3d-pose-controls">
            <label>姿态预设<select value={presetId} onChange={(event) => setPresetId(event.target.value)}><optgroup label="内置预设">{presets.map((preset) => <option key={preset.id} value={preset.id}>{preset.label}</option>)}</optgroup>{(posePresetDocument?.poses.length ?? 0) > 0 && <optgroup label="工程姿态">{posePresetDocument!.poses.map((pose) => <option key={pose.poseId} value={`saved:${pose.poseId}`}>{pose.name}</option>)}</optgroup>}</select></label>
            <button disabled={playing} onClick={() => { const preset = poseOptions.find((item) => item.id === presetId); if (preset) animateToPose(preset.pose, preset.label); }}><Play size={13} />播放预设</button>
            <button disabled={playing} onClick={resetPose}><RotateCcw size={13} />中性复位</button>
            <div className="model3d-save-pose"><input aria-label="姿态名称" value={savedPoseName} maxLength={40} disabled={!posePresetWritable} placeholder={posePresetWritable ? '输入工程姿态名称' : '当前模型工程为只读'} onChange={(event) => setSavedPoseName(event.target.value)} onKeyDown={(event) => { if (event.key === 'Enter' && savedPoseName.trim()) void savePose(); }} /><button disabled={!posePresetWritable || !savedPoseName.trim()} onClick={() => void savePose()}><Save size={13} />保存姿态</button></div>
          </div>
          {motionMessage && <div className="model3d-motion-message">{playing && <RefreshCw size={11} className="spin" />}{motionMessage}</div>}
          <div className="model3d-joint-list">{manifest.joints.map((joint) => {
            const value = jointValues[joint.jointId] ?? joint.neutralValue;
            const degrees = THREE.MathUtils.radToDeg(value);
            const lower = THREE.MathUtils.radToDeg(joint.limit?.lower ?? -Math.PI);
            const upper = THREE.MathUtils.radToDeg(joint.limit?.upper ?? Math.PI);
            const atLimit = isAtJointLimit(joint, value);
            return <div key={joint.jointId} className={`model3d-joint ${joint.jointId === selectedJointId ? 'selected' : ''} ${atLimit ? 'at-limit' : ''}`} onClick={() => { setSelectedJointId(joint.jointId); chooseComponent(joint.jointId); }}>
              <div><strong>{jointLabel(joint.jointId)}</strong><code>{joint.jointId}</code>{atLimit && <span>{atLimit === 'lower' ? '下限' : '上限'}</span>}</div>
              <input aria-label={`${jointLabel(joint.jointId)}滑块`} type="range" min={lower} max={upper} step="0.1" disabled={joint.type === 'fixed' || playing} value={degrees} onClick={(event) => event.stopPropagation()} onChange={(event) => applyJointValue(joint.jointId, THREE.MathUtils.degToRad(Number(event.target.value)))} />
              <label><input aria-label={`${jointLabel(joint.jointId)}角度`} type="number" min={lower} max={upper} step="0.1" disabled={joint.type === 'fixed' || playing} value={Number(degrees.toFixed(1))} onClick={(event) => event.stopPropagation()} onChange={(event) => applyJointValue(joint.jointId, THREE.MathUtils.degToRad(Number(event.target.value)))} /><span>°</span></label>
              <small>{lower.toFixed(1)}° — {upper.toFixed(1)}° · {joint.type}</small>
            </div>;
          })}</div>
        </div> : selectedProductEntry ? <><div className="model3d-selected-title">{selectedProductEntry.nodeType === 'collection' ? <Folder /> : <Box />}<strong>{selectedProductEntry.label}</strong><code>{selectedProductEntry.nodeName}</code></div><dl><div><dt>节点类型</dt><dd>{selectedProductEntry.nodeType === 'collection' ? '集合' : '部件'}</dd></div><div><dt>父集合</dt><dd>{selectedProductEntry.parentNodeId ?? '根集合'}</dd></div><div><dt>直接子节点</dt><dd>{productEntries.filter((item) => item.parentNodeId === selectedProductEntry.nodeId).length}</dd></div><div><dt>所属刚体</dt><dd>{selectedProductEntry.bodyId}</dd></div></dl><div className="model3d-property-actions"><button onClick={() => setIsolated((value) => value === selectedProductEntry.nodeId ? undefined : selectedProductEntry.nodeId)}>{isolated === selectedProductEntry.nodeId ? '退出隔离' : '隔离节点'}</button><button onClick={() => { setHidden(new Set()); setIsolated(undefined); }}>全部显示</button><button onClick={() => envRef.current && fitViewerObjects(envRef.current, envRef.current.productTargets.get(selectedProductEntry.nodeId) ?? [])}>定位节点</button></div></> : <div className="model3d-no-selection">在 CAD 产品结构或三维场景中选择集合或部件</div>}
      </aside>
    </div>
  </div>;
}

async function loadRegistered(url: string, signal: AbortSignal, onProgress: (loaded: number, total?: number) => void) {
  const loaded = await loadRobotModel(url, { signal, onProgress });
  const poseStore = await loadProjectPosePresets(loaded.manifest, signal);
  return { displayRoot: loaded.displayRoot, modelRoot: loaded.modelRoot, manifest: loaded.manifest, rig: loaded.rig, posePresets: poseStore.document, posePresetWritable: poseStore.writable };
}

async function loadLocalFiles(files: File[], onProgress: (loaded: number, total?: number) => void) {
  const manifestFile = files.find((file) => file.name.toLowerCase().includes('manifest') && file.name.toLowerCase().endsWith('.json'));
  const posePresetsFile = files.find((file) => file.name.toLowerCase() === 'pose-presets.json');
  let manifest: Model3DManifest | undefined;
  let posePresets: Model3DPosePresetDocument | undefined;
  let visual = files.find((file) => /\.(glb|gltf)$/i.test(file.name));
  if (manifestFile) {
    manifest = JSON.parse(await manifestFile.text()) as Model3DManifest;
    const kinematicsAsset = manifest.assets.find((item) => item.role === 'kinematics');
    if (kinematicsAsset) {
      const name = kinematicsAsset.uri.split('/').pop();
      const kinematicsFile = files.find((file) => file.name === name);
      if (!kinematicsFile) throw new Error(`请同时选择模型包引用的 ${name}`);
      applyKinematics(manifest, JSON.parse(await kinematicsFile.text()) as Model3DKinematics);
    }
    if (posePresetsFile) posePresets = parsePosePresetDocument(JSON.parse(await posePresetsFile.text()) as unknown, manifest);
    const uri = manifest.assets.find((item) => item.role === 'visual')?.uri;
    if (!uri) throw new Error('模型包清单没有 visual 资产');
    const name = uri.split('/').pop(); visual = files.find((file) => file.name === name) ?? visual;
    if (!visual) throw new Error(`请同时选择模型包引用的 ${name}`);
  }
  if (!visual) throw new Error('请选择 .glb、.gltf，或同时选择 manifest JSON 与视觉资产');
  const objectUrls = files.map((file) => ({ file, url: URL.createObjectURL(file) }));
  const urlByName = new Map(objectUrls.flatMap(({ file, url }) => {
    const path = file.webkitRelativePath || file.name;
    return [[file.name, url], [path, url], [path.replace(/\\/g, '/'), url]] as [string, string][];
  }));
  const manager = new THREE.LoadingManager();
  manager.setURLModifier((requestedUrl) => {
    const decoded = decodeURIComponent(requestedUrl).replace(/\\/g, '/');
    const basename = decoded.split('/').pop() ?? decoded;
    return urlByName.get(decoded) ?? urlByName.get(basename) ?? requestedUrl;
  });
  const url = objectUrls.find(({ file }) => file === visual)!.url;
  try {
    const gltf = await new Promise<Awaited<ReturnType<GLTFLoader['loadAsync']>>>((resolve, reject) => new GLTFLoader(manager).load(url, resolve, (event) => onProgress(event.loaded, event.total || undefined), reject));
    const displayRoot = new THREE.Group(); displayRoot.name = 'aidc-local-model-display';
    if (manifest?.coordinateSystem.up === '+Z') displayRoot.rotation.x = -Math.PI / 2;
    displayRoot.add(gltf.scene);
    return { displayRoot, modelRoot: gltf.scene, manifest, rig: manifest ? new RobotRig(manifest, gltf.scene) : undefined, posePresets, posePresetWritable: false };
  } finally { for (const entry of objectUrls) URL.revokeObjectURL(entry.url); }
}

async function loadProjectPosePresets(manifest: Model3DManifest, signal: AbortSignal): Promise<{ document: Model3DPosePresetDocument; writable: boolean }> {
  const projectUrl = `/api/model-projects/${encodeURIComponent(manifest.modelId)}/pose-presets`;
  let response = await fetch(projectUrl, { signal });
  if (response.ok) return { document: parsePosePresetDocument(await response.json() as unknown, manifest), writable: true };
  response = await fetch('/pose-presets.json', { signal });
  if (!response.ok) throw new Error('模型工程缺少 pose-presets.json');
  return { document: parsePosePresetDocument(await response.json() as unknown, manifest), writable: false };
}

async function saveProjectPosePresets(modelId: string, document: Model3DPosePresetDocument): Promise<Model3DPosePresetDocument> {
  const response = await fetch(`/api/model-projects/${encodeURIComponent(modelId)}/pose-presets`, {
    method: 'PUT', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(document),
  });
  const value = await response.json().catch(() => undefined) as Model3DPosePresetDocument | { message?: string } | undefined;
  if (!response.ok) throw new Error(value && 'message' in value && value.message ? value.message : '模型工程姿态保存失败');
  return value as Model3DPosePresetDocument;
}

function parsePosePresetDocument(value: unknown, manifest: Pick<Model3DManifest, 'modelId' | 'revision'>): Model3DPosePresetDocument {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('姿态预设文档无效');
  const document = value as Partial<Model3DPosePresetDocument>;
  if (document.schemaVersion !== POSE_PRESETS_SCHEMA_VERSION) throw new Error('姿态预设 Schema 版本不受支持');
  if (document.modelId !== manifest.modelId || document.modelRevision !== manifest.revision) throw new Error('姿态预设与当前模型版本不一致');
  if (!Array.isArray(document.poses)) throw new Error('姿态预设列表无效');
  const ids = new Set<string>(); const names = new Set<string>();
  for (const pose of document.poses) {
    if (!pose || typeof pose.poseId !== 'string' || !pose.poseId || ids.has(pose.poseId)) throw new Error('姿态 ID 缺失或重复');
    if (typeof pose.name !== 'string' || !pose.name.trim() || pose.name.length > 40 || names.has(pose.name)) throw new Error('姿态名称缺失、过长或重复');
    if (!isJointPose(pose.jointValues)) throw new Error(`姿态“${pose.name}”的关节值无效`);
    if (typeof pose.updatedAt !== 'string' || !pose.updatedAt) throw new Error(`姿态“${pose.name}”缺少更新时间`);
    ids.add(pose.poseId); names.add(pose.name);
  }
  return document as Model3DPosePresetDocument;
}

function genericComponents(root: THREE.Object3D) {
  const result = new Map<string, THREE.Object3D>();
  root.traverse((object) => { if (object.name && (object instanceof THREE.Mesh || object.children.length)) result.set(object.name, object); });
  if (!result.size) result.set('model', root);
  return result;
}

function resizeViewer(env: ViewerEnvironment, host: HTMLElement) {
  const width = Math.max(1, host.clientWidth); const height = Math.max(1, host.clientHeight); const aspect = width / height;
  env.renderer.setSize(width, height, false); env.perspective.aspect = aspect; env.perspective.updateProjectionMatrix();
}

function alignGroundPlane(env: ViewerEnvironment, object: THREE.Object3D) {
  object.updateMatrixWorld(true);
  const bounds = new THREE.Box3().setFromObject(object, true);
  if (!Number.isFinite(bounds.min.y)) return;
  const groundY = bounds.min.y - .001;
  env.floor.position.y = groundY;
  env.grid.position.y = groundY + .0005;
}

function fitViewer(env: ViewerEnvironment, object: THREE.Object3D) {
  const sphere = new THREE.Box3().setFromObject(object).getBoundingSphere(new THREE.Sphere());
  fitViewerSphere(env, sphere);
}

function fitViewerObjects(env: ViewerEnvironment, objects: THREE.Object3D[]) {
  const bounds = new THREE.Box3();
  for (const object of objects) bounds.union(new THREE.Box3().setFromObject(object, true));
  if (bounds.isEmpty()) return;
  fitViewerSphere(env, bounds.getBoundingSphere(new THREE.Sphere()));
}

function fitViewerSphere(env: ViewerEnvironment, sphere: THREE.Sphere) {
  if (!Number.isFinite(sphere.radius) || sphere.radius <= 0) return;
  const direction = env.activeCamera.position.clone().sub(env.controls.target).normalize(); if (!direction.lengthSq()) direction.set(1, .7, 1).normalize();
  const distance = sphere.radius * 3.2; env.activeCamera.position.copy(sphere.center).add(direction.multiplyScalar(distance));
  env.activeCamera.near = Math.max(sphere.radius / 100, .001); env.activeCamera.far = Math.max(sphere.radius * 30, 10);
  if (env.activeCamera instanceof THREE.OrthographicCamera) env.activeCamera.zoom = Math.max(0.2, 0.32 / sphere.radius);
  env.activeCamera.updateProjectionMatrix(); env.controls.target.copy(sphere.center); env.controls.update();
}

function setStandardView(env: ViewerEnvironment | undefined, view: 'iso' | 'front' | 'left' | 'top') {
  if (!env?.displayRoot) return;
  const sphere = new THREE.Box3().setFromObject(env.displayRoot).getBoundingSphere(new THREE.Sphere());
  const directions = { iso: new THREE.Vector3(1, .75, 1), front: new THREE.Vector3(1, 0, 0), left: new THREE.Vector3(0, 0, -1), top: new THREE.Vector3(0, 1, 0.001) };
  env.activeCamera.position.copy(sphere.center).add(directions[view].normalize().multiplyScalar(sphere.radius * 3.2)); env.controls.target.copy(sphere.center); env.controls.update();
}

function selectObjects(env: ViewerEnvironment, objects: THREE.Object3D[]) {
  clearSelectionHighlight(env);
  const selected = new Set<THREE.Mesh>();
  for (const object of objects) object.traverse((child) => {
    if (child instanceof THREE.Mesh && child.visible) selected.add(child);
  });
  for (const child of selected) {
    env.selectionMeshes.push(child);
    for (const material of selectedMaterials(child)) {
      // Match FreeCAD's solid green selection cue while retaining the source
      // material, lighting, and normal depth occlusion.
      material.color.lerp(new THREE.Color(0x00ff00), .38);
      material.emissive.set(0x087a2f);
      material.emissiveIntensity = .14;
      material.needsUpdate = true;
    }
  }
}

function flattenProductTree(entries: ProductEntry[], expanded: Set<string>, search: string): ProductRow[] {
  const children = new Map<string | undefined, ProductEntry[]>();
  for (const entry of entries) {
    const siblings = children.get(entry.parentNodeId) ?? [];
    siblings.push(entry);
    children.set(entry.parentNodeId, siblings);
  }
  const term = search.trim().toLowerCase();
  const matches = (entry: ProductEntry): boolean => !term
    || `${entry.label} ${entry.nodeName} ${entry.bodyId}`.toLowerCase().includes(term)
    || (children.get(entry.nodeId) ?? []).some(matches);
  const rows: ProductRow[] = [];
  const visit = (entry: ProductEntry, depth: number) => {
    if (!matches(entry)) return;
    const descendants = children.get(entry.nodeId) ?? [];
    rows.push({ ...entry, depth, hasChildren: descendants.length > 0 });
    if (term || expanded.has(entry.nodeId)) descendants.forEach((child) => visit(child, depth + 1));
  };
  (children.get(undefined) ?? []).forEach((root) => visit(root, 0));
  return rows;
}

function resolveExplosionTarget(selectedNodeId: string | undefined, entries: ProductEntry[], targetNodeIds: string[]): string | undefined {
  if (!selectedNodeId || !targetNodeIds.length) return undefined;
  const targets = new Set(targetNodeIds);
  const byId = new Map(entries.map((entry) => [entry.nodeId, entry]));
  let nodeId: string | undefined = selectedNodeId;
  while (nodeId) {
    if (targets.has(nodeId)) return nodeId;
    nodeId = byId.get(nodeId)?.parentNodeId;
  }
  return undefined;
}

function buildProductTargets(entries: ProductEntry[], nodes: Map<string, THREE.Object3D>): Map<string, THREE.Object3D[]> {
  const children = new Map<string, ProductEntry[]>();
  for (const entry of entries) {
    if (!entry.parentNodeId) continue;
    const siblings = children.get(entry.parentNodeId) ?? [];
    siblings.push(entry);
    children.set(entry.parentNodeId, siblings);
  }
  const result = new Map<string, THREE.Object3D[]>();
  const resolve = (entry: ProductEntry, stack = new Set<string>()): THREE.Object3D[] => {
    const cached = result.get(entry.nodeId); if (cached) return cached;
    if (stack.has(entry.nodeId)) return [];
    const nextStack = new Set(stack).add(entry.nodeId);
    const targets = entry.nodeType === 'part'
      ? (nodes.get(entry.nodeId) ? [nodes.get(entry.nodeId)!] : [])
      : (children.get(entry.nodeId) ?? []).flatMap((child) => resolve(child, nextStack));
    const unique = [...new Set(targets)];
    result.set(entry.nodeId, unique);
    return unique;
  };
  entries.forEach((entry) => resolve(entry));
  return result;
}

function applyProductDisplayState(
  env: ViewerEnvironment,
  state: Pick<Model3DViewState, 'hiddenComponentIds' | 'isolatedComponentId' | 'transparent' | 'wireframe'>,
) {
  if (!env.modelRoot) return;
  applyDisplayState(env.modelRoot, new Map(), { ...state, hiddenComponentIds: [], isolatedComponentId: undefined });
  for (const nodeId of state.hiddenComponentIds) {
    for (const target of env.productTargets.get(nodeId) ?? []) target.traverse((object) => {
      if (object instanceof THREE.Mesh) object.visible = false;
    });
  }
  if (state.isolatedComponentId) {
    const allowed = new Set<THREE.Mesh>();
    for (const target of env.productTargets.get(state.isolatedComponentId) ?? []) target.traverse((object) => {
      if (object instanceof THREE.Mesh) allowed.add(object);
    });
    env.modelRoot.traverse((object) => {
      if (object instanceof THREE.Mesh && !allowed.has(object)) object.visible = false;
    });
  }
}

function clearSelectionHighlight(env: ViewerEnvironment) {
  for (const mesh of env.selectionMeshes) {
    for (const material of selectedMaterials(mesh)) {
      material.color.setHex(Number(material.userData.aidcOriginalColor ?? 0xffffff));
      material.emissive.setHex(Number(material.userData.aidcOriginalEmissive ?? 0));
      material.emissiveIntensity = Number(material.userData.aidcOriginalEmissiveIntensity ?? 1);
      material.depthTest = material.userData.aidcOriginalDepthTest !== false;
      material.depthWrite = material.userData.aidcOriginalDepthWrite !== false;
      material.needsUpdate = true;
    }
  }
  env.selectionMeshes = [];
}

function selectedMaterials(mesh: THREE.Mesh): THREE.MeshStandardMaterial[] {
  const values = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
  return values.filter((material): material is THREE.MeshStandardMaterial =>
    'emissive' in material && material.emissive instanceof THREE.Color,
  );
}

function motionDuration(manifest: Model3DManifest, from: JointPose, to: JointPose): number {
  let seconds = 0;
  for (const joint of manifest.joints) {
    const distance = Math.abs((to[joint.jointId] ?? joint.neutralValue) - (from[joint.jointId] ?? joint.neutralValue));
    const velocity = joint.limit?.velocity && joint.limit.velocity > 0 ? joint.limit.velocity : Math.PI;
    seconds = Math.max(seconds, distance / velocity);
  }
  return THREE.MathUtils.clamp(seconds * 1250, 450, 2400);
}

function createPoseId(): string {
  return typeof crypto.randomUUID === 'function' ? crypto.randomUUID() : `pose-${Date.now()}`;
}

function isJointPose(value: unknown): value is JointPose {
  return !!value && typeof value === 'object' && !Array.isArray(value)
    && Object.values(value).every((entry) => typeof entry === 'number' && Number.isFinite(entry));
}

function clearGhost(env: ViewerEnvironment) {
  if (!env.ghostRoot) return;
  env.scene.remove(env.ghostRoot);
  env.ghostRoot.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;
    for (const material of Array.isArray(object.material) ? object.material : [object.material]) material.dispose();
  });
  env.ghostRoot.clear(); env.ghostRoot = undefined;
}

function createBomLabels(env: ViewerEnvironment, productEntries: ProductEntry[], showLeaderLines: boolean, selectedNodeId?: string) {
  clearBomLabels(env);
  const holder = new THREE.Group(); holder.name = 'aidc-bom-label-holder';
  const items: THREE.Object3D[] = [];
  const targetIds = new Set(env.explosionRig?.nodeIds ?? []);
  productEntries.filter((entry) => targetIds.has(entry.nodeId)).forEach((entry, index) => {
    const object = env.productNodes.get(entry.nodeId); if (!object) return;
    const canvas = document.createElement('canvas'); canvas.width = 512; canvas.height = 128;
    const context = canvas.getContext('2d'); if (!context) return;
    const selected = entry.nodeId === selectedNodeId;
    context.fillStyle = selected ? 'rgba(20,109,96,.96)' : 'rgba(24,45,58,.90)'; context.roundRect(2, 2, 508, 124, 24); context.fill();
    context.fillStyle = selected ? '#ffffff' : '#5ed0bf'; context.font = '700 42px system-ui'; context.fillText(String(index + 1).padStart(2, '0'), 22, 79);
    context.fillStyle = '#ffffff'; context.font = '600 28px system-ui'; context.fillText(entry.label, 100, 75, 390);
    const texture = new THREE.CanvasTexture(canvas); texture.colorSpace = THREE.SRGBColorSpace;
    const material = new THREE.SpriteMaterial({ map: texture, transparent: true, depthTest: false });
    const sprite = new THREE.Sprite(material); sprite.name = `bom:${entry.nodeId}`; sprite.userData.productNodeId = entry.nodeId; sprite.userData.aidcIgnorePicking = true; sprite.scale.set(.038, .0095, 1); sprite.renderOrder = 30;
    const center = new THREE.Box3().setFromObject(object, true).getCenter(new THREE.Vector3()); object.worldToLocal(center);
    const side = index % 2 ? -1 : 1; const labelPosition = center.clone().add(new THREE.Vector3(.018 * side, .014 + index % 3 * .004, 0)); sprite.position.copy(labelPosition);
    if (showLeaderLines) {
      const geometry = new THREE.BufferGeometry().setFromPoints([center, labelPosition]);
      const lineMaterial = new THREE.LineBasicMaterial({ color: selected ? 0x168f7e : 0x597788, transparent: true, opacity: .85, depthTest: false });
      const line = new THREE.Line(geometry, lineMaterial); line.name = `bom-line:${entry.nodeId}`; line.userData.aidcIgnorePicking = true; line.renderOrder = 29; object.add(line); items.push(line);
    }
    object.add(sprite); items.push(sprite);
  });
  holder.userData.items = items; env.bomLabels = holder;
}

function clearBomLabels(env: ViewerEnvironment) {
  const items = env.bomLabels?.userData.items as THREE.Object3D[] | undefined;
  for (const item of items ?? []) {
    item.removeFromParent();
    if (item instanceof THREE.Sprite) { const material = item.material as THREE.SpriteMaterial; material.map?.dispose(); material.dispose(); }
    if (item instanceof THREE.Line) { item.geometry.dispose(); (item.material as THREE.Material).dispose(); }
  }
  env.bomLabels = undefined;
}

function mergeExactResults(preview: InterferencePairResult[], current: InterferencePairResult[]): InterferencePairResult[] {
  const exact = new Map(current.filter((item) => item.precision === 'BREP_EXACT').map((item) => [item.pairId, item]));
  return preview.map((item) => {
    const prior = exact.get(item.pairId);
    return prior && samePose(prior.pose, item.pose) ? prior : item;
  });
}

function samePose(left?: Record<string, number>, right?: Record<string, number>): boolean {
  if (!left || !right) return false;
  const keys = new Set([...Object.keys(left), ...Object.keys(right)]);
  return [...keys].every((key) => Math.abs((left[key] ?? 0) - (right[key] ?? 0)) < 1e-7);
}

function clearCollisionHighlights(env: ViewerEnvironment) {
  for (const helper of env.collisionHelpers) {
    env.scene.remove(helper); helper.geometry.dispose();
    const materials = Array.isArray(helper.material) ? helper.material : [helper.material]; materials.forEach((material) => material.dispose());
  }
  env.collisionHelpers = [];
}

function renderCollisionHighlights(env: ViewerEnvironment, results: InterferencePairResult[]) {
  clearCollisionHighlights(env);
  const ids = new Set(results.flatMap((item) => [item.componentA, item.componentB]));
  for (const componentId of ids) {
    const object = env.components.get(componentId); if (!object) continue;
    const helper = new THREE.Box3Helper(new THREE.Box3().setFromObject(object, true), new THREE.Color(0xd14f45));
    helper.renderOrder = 20;
    const materials = Array.isArray(helper.material) ? helper.material : [helper.material]; materials.forEach((material) => { material.depthTest = false; });
    env.scene.add(helper); env.collisionHelpers.push(helper);
  }
}

function fitViewerToComponents(env: ViewerEnvironment, componentIds: string[]) {
  const box = new THREE.Box3(); let found = false;
  for (const componentId of componentIds) {
    const object = env.components.get(componentId); if (!object) continue;
    box.union(new THREE.Box3().setFromObject(object, true)); found = true;
  }
  if (!found || box.isEmpty()) return;
  const sphere = box.getBoundingSphere(new THREE.Sphere());
  const direction = env.activeCamera.position.clone().sub(env.controls.target).normalize();
  env.activeCamera.position.copy(sphere.center).add(direction.multiplyScalar(Math.max(.05, sphere.radius * 3.5)));
  env.controls.target.copy(sphere.center); env.activeCamera.updateProjectionMatrix(); env.controls.update();
}

function collisionStatusLabel(status: InterferencePairResult['status']) {
  return ({ WARNING: '待校核', CONFIRMED: '已确认干涉', CLEARED: '已排除', ALLOWED_CONTACT: '允许接触', UNVERIFIED: '无法校核' })[status];
}

function formatVolume(value?: number) {
  if (value == null || !Number.isFinite(value)) return '—';
  return `${(value * 1e9).toFixed(value < 1e-9 ? 3 : 1)} mm³`;
}

function downloadScreenshot(env: ViewerEnvironment | undefined) {
  if (!env) return; env.renderer.render(env.scene, env.activeCamera);
  const link = document.createElement('a'); link.download = `aidc-model3d-${Date.now()}.png`; link.href = env.renderer.domElement.toDataURL('image/png'); link.click();
}

const sessionStateKey = 'aidc-model3d-browser-session';

function readSessionState(): Record<string, unknown> | undefined {
  try { const raw = localStorage.getItem(sessionStateKey); return raw ? JSON.parse(raw) as Record<string, unknown> : undefined; }
  catch { return undefined; }
}

function saveSessionState(state: Model3DViewState) {
  localStorage.setItem(sessionStateKey, JSON.stringify(state));
}
