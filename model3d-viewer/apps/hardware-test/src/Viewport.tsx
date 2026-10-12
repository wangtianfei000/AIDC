import { useEffect, useRef } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { loadRobotModel, type LoadedRobotModel } from '@aidc/model3d-three-runtime';
import { prepareMaterials, disposeObject3D } from '../../browser/src/runtime';

export type ViewHandle = { view:(name:string)=>void };
type Props={simRoot:{position:number[];quaternion:number[]}|null;bodyEuler:number[];pose:Record<string,number>; selected:string; transparent:boolean; axes:boolean;
  onSelect:(name:string)=>void; onReady:(v:ViewHandle)=>void; onStatus:(s:string)=>void};
export default function Viewport(p:Props){
 const host=useRef<HTMLDivElement>(null), loaded=useRef<LoadedRobotModel|null>(null);
 const latest=useRef(p); latest.current=p;
 useEffect(()=>{
  const el=host.current!; let disposed=false, frame=0;
  const scene=new THREE.Scene();scene.background=new THREE.Color('#eaf0f3');
  const camera=new THREE.PerspectiveCamera(35,1,.001,50);
  const renderer=new THREE.WebGLRenderer({antialias:true});renderer.setPixelRatio(Math.min(devicePixelRatio,2));
  renderer.shadowMap.enabled=true; renderer.shadowMap.type=THREE.PCFSoftShadowMap;
  renderer.outputColorSpace=THREE.SRGBColorSpace;el.appendChild(renderer.domElement);
  renderer.domElement.setAttribute('aria-label','机器鸭三维模型');renderer.domElement.setAttribute('role','img');
  const controls=new OrbitControls(camera,renderer.domElement);controls.enableDamping=true;
  scene.add(new THREE.HemisphereLight(0xffffff,0x657487,2.5));
  const sun=new THREE.DirectionalLight(0xffffff,3);sun.position.set(1,2,1);scene.add(sun);
  const attitude=new THREE.Group();scene.add(attitude);
  const grid=new THREE.GridHelper(2,40,0xb0bec8,0xd1dce2);scene.add(grid);
  const axis=new THREE.AxesHelper(.08);scene.add(axis);
  let center=new THREE.Vector3(),radius=.4;
  const fit=(name:string)=>{const d:Record<string,number[]>={front:[1,.15,0],left:[0,.15,-1],top:[0,1,.001],iso:[1,.65,1]};
   const halfVertical=THREE.MathUtils.degToRad(camera.fov/2);
   const halfFov=Math.min(halfVertical,Math.atan(Math.tan(halfVertical)*camera.aspect));
   camera.position.copy(center).add(new THREE.Vector3(...(d[name]||d.iso)).normalize().multiplyScalar(radius/Math.sin(halfFov)*1.08));
   controls.target.copy(center);controls.update();};
  latest.current.onReady({view:fit});
  const resize=new ResizeObserver(()=>{const w=el.clientWidth,h=el.clientHeight;if(w&&h){renderer.setSize(w,h);camera.aspect=w/h;camera.updateProjectionMatrix();}});resize.observe(el);
  const abort=new AbortController();
  loadRobotModel(import.meta.env.BASE_URL+'model3d.manifest.json',{signal:abort.signal,onProgress:(n,total)=>latest.current.onStatus(total?`加载模型 ${Math.round(n/total*100)}%`:'加载模型…')}).then(model=>{
   if(disposed){disposeObject3D(model.displayRoot);return;} loaded.current=model;prepareMaterials(model.modelRoot);attitude.add(model.displayRoot);
   const box=new THREE.Box3().setFromObject(model.displayRoot);box.getCenter(center);radius=box.getSize(new THREE.Vector3()).length()/2;
   attitude.position.copy(center);model.displayRoot.position.sub(center);grid.position.y=box.min.y-.002;fit('iso');latest.current.onStatus('模型就绪 · 12 轴 / 13 刚体');
  }).catch(e=>{if(!disposed)latest.current.onStatus('模型加载失败：'+e.message);});
  let down=[0,0];const pointerDown=(e:PointerEvent)=>{down=[e.clientX,e.clientY];};
  const pointerUp=(e:PointerEvent)=>{if(Math.hypot(e.clientX-down[0],e.clientY-down[1])>5||!loaded.current)return;
   const rect=el.getBoundingClientRect(),ray=new THREE.Raycaster();ray.setFromCamera(new THREE.Vector2((e.clientX-rect.left)/rect.width*2-1,-(e.clientY-rect.top)/rect.height*2+1),camera);
   const hit=ray.intersectObject(loaded.current.modelRoot,true).find(x=>x.object instanceof THREE.Mesh);
   if(hit)latest.current.onSelect(hit.object.name);
  };renderer.domElement.addEventListener('pointerdown',pointerDown);renderer.domElement.addEventListener('pointerup',pointerUp);
  const render=()=>{if(disposed)return;const m=loaded.current;
   if(m){const [roll,pitch,yaw]=latest.current.bodyEuler.map(THREE.MathUtils.degToRad);
    const basis=new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1,0,0),-Math.PI/2);
    attitude.quaternion.copy(basis).multiply(new THREE.Quaternion().setFromEuler(new THREE.Euler(roll,pitch,yaw,'XYZ'))).multiply(basis.clone().invert());
    m.rig.setPose(Object.fromEntries(Object.entries(latest.current.pose).map(([k,v])=>[k,THREE.MathUtils.degToRad(v)])));
    const root=latest.current.simRoot;
    if(root){
     const [w,x,y,z]=root.quaternion;
     attitude.quaternion.copy(basis).multiply(new THREE.Quaternion(x,y,z,w)).multiply(basis.clone().invert());
     attitude.position.set(root.position[0],root.position[2],-root.position[1]);
     // CAD trunk origin is at Z=0.12 m; rotate about that origin, not the mesh centre.
     m.displayRoot.position.set(0,-.12,0);
    }else{attitude.position.copy(center);m.displayRoot.position.copy(center).negate();}
    const selected=m.modelRoot.getObjectByName(latest.current.selected);
    m.modelRoot.traverse(o=>{if(!(o instanceof THREE.Mesh))return;let chosen=false;for(let n:THREE.Object3D|null=o;n;n=n.parent)if(n===selected)chosen=true;
     for(const mat of Array.isArray(o.material)?o.material:[o.material]){const a=mat as THREE.MeshStandardMaterial;
      a.emissive?.setHex(chosen?0x0e8e86:0);a.emissiveIntensity=chosen?.3:0;
      a.opacity=latest.current.transparent?.3:Number(a.userData.aidcOriginalOpacity??1);a.transparent=a.opacity<1;a.depthWrite=!a.transparent;}
    });
    axis.quaternion.copy(attitude.quaternion).multiply(basis);axis.visible=latest.current.axes;if(selected)selected.getWorldPosition(axis.position);else axis.position.copy(center);
   } controls.update();renderer.render(scene,camera);frame=requestAnimationFrame(render);};render();
  return()=>{disposed=true;abort.abort();cancelAnimationFrame(frame);resize.disconnect();controls.dispose();renderer.dispose();
   renderer.domElement.removeEventListener('pointerdown',pointerDown);renderer.domElement.removeEventListener('pointerup',pointerUp);
   disposeObject3D(scene);loaded.current=null;renderer.domElement.remove();};
 },[]);
 return <div className="viewport" ref={host}/>;
}
