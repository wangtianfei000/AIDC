import { useEffect, useRef, useState } from "react";
import { FitPlugin, Geometry, Graph, InternalEvent, Point, Shape, ShapeRegistry, type AbstractCanvas2D, type Cell, type CellStyle, type PerimeterFunction } from "@maxgraph/core";
import type { Component, IconGraphic, ModelClass, TypeIcon } from "./types";

type DiagramPoint=[number,number];
type Props={cls:ModelClass|null;icons:Record<string,TypeIcon>;selected:Component|null;onSelect:(component:Component)=>void};
type GraphRuntime={graph:Graph;componentCells:Map<string,Cell>};

const color=(value:[number,number,number]|undefined,fallback="none")=>value&&!value.some(channel=>channel<0)?`rgb(${value.join(",")})`:fallback;
const compactNumber=(value:string)=>{const trimmed=value.trim();if(!/^[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?$/.test(trimmed))return trimmed;const number=Number(trimmed);return Number.isFinite(number)?new Intl.NumberFormat("zh-CN",{maximumSignificantDigits:2,useGrouping:false}).format(number):trimmed};
const visibleFor=(expression:string|undefined,values:Record<string,string>)=>{if(!expression)return true;const strip=(value:string)=>{let result=value.trim();while(result.startsWith("(")&&result.endsWith(")"))result=result.slice(1,-1).trim();return result};const read=(raw:string):boolean=>{const value=strip(raw),or=value.split(/\s+or\s+/i);if(or.length>1)return or.some(read);const and=value.split(/\s+and\s+/i);if(and.length>1)return and.every(read);if(/^not\s+/i.test(value))return !read(value.replace(/^not\s+/i,""));const equal=value.match(/^([A-Za-z_]\w*)\s*(==|<>)\s*(.+)$/);if(equal){const actual=values[equal[1]]||"",expected=equal[3].trim().split(".").pop()||"",same=(actual.split(".").pop()||actual)===expected;return equal[2]==="=="?same:!same}if(/^true$/i.test(value))return true;if(/^false$/i.test(value))return false;return /^(true|1)$/i.test(values[value]||"")};return read(expression)};
function connectorIsVisible(icon:TypeIcon,component:Component,connector:TypeIcon["connectors"][number]){const values={...(icon.defaults||{}),...(component.modifiers||{})};if(!connector.placement.visible||!visibleFor(connector.condition,values))return false;if(connector.name==="internalSupport")return false;if(connector.name==="heatPort"&&/^(false|0)$/i.test(values.useHeatPort||""))return false;if(connector.name==="support"&&/^(false|0)$/i.test(values.useSupport||""))return false;return true}

const componentShapeData=new WeakMap<Cell,{icon:TypeIcon|undefined;component:Component}>();
class ModelicaComponentShape extends Shape{
  constructor(){super();this.pointerEvents=true}
  override paintVertexShape(canvas:AbstractCanvas2D,x:number,y:number,width:number,height:number){
    const cell=this.state?.cell,data=cell?componentShapeData.get(cell):undefined;if(!data)return;
    if(this.node instanceof SVGElement){this.node.setAttribute("data-component-name",data.component.name);this.node.setAttribute("data-component-type",data.component.type)}
    canvas.save();canvas.setFillColor("#ffffff");canvas.setFillAlpha(.001);canvas.setStrokeColor(null);canvas.rect(x,y,width,height);canvas.fill();canvas.restore();
    const {icon,component}=data;if(!icon){canvas.setFillColor("#ffffff");canvas.setStrokeColor("#9eb6b2");canvas.rect(x,y,width,height);canvas.fillAndStroke();canvas.setFontColor("#314b47");canvas.setFontSize(9);canvas.text(x,y,width,height,component.name,"center","middle",false,"","visible",false,0,"ltr");return}
    const [[xmin,ymin],[xmax,ymax]]=icon.extent,iconWidth=xmax-xmin,iconHeight=ymax-ymin;if(!iconWidth||!iconHeight)return;
    const scale=Math.min(width/Math.abs(iconWidth),height/Math.abs(iconHeight)),offsetX=x+(width-Math.abs(iconWidth)*scale)/2-xmin*scale,offsetY=y+(height-Math.abs(iconHeight)*scale)/2+ymax*scale,values={...(icon.defaults||{}),...(component.modifiers||{})},map=(point:DiagramPoint):DiagramPoint=>[offsetX+point[0]*scale,offsetY-point[1]*scale];
    for(const graphic of icon.graphics.filter(item=>visibleFor(item.visible,values))){
      const origin=graphic.origin||[0,0],mappedOrigin=map(origin),rotation=graphic.rotation||0,stroke=color(graphic.lineColor,"#405a56"),filled=Boolean(graphic.fillPattern&&graphic.fillPattern!=="None");canvas.save();if(rotation)canvas.rotate(-rotation,false,false,mappedOrigin[0],mappedOrigin[1]);canvas.setStrokeColor(stroke);canvas.setFillColor(filled?color(graphic.fillColor,"#ffffff"):null);canvas.setStrokeWidth(Math.max(.8,(graphic.thickness||.25)*2));canvas.setDashed(graphic.linePattern==="Dash"||graphic.linePattern==="Dot");if(graphic.linePattern==="Dot")canvas.setDashPattern("2 4");
      const local=(point:DiagramPoint)=>map([origin[0]+point[0],origin[1]+point[1]]);
      if(graphic.type==="line"&&graphic.points?.length){canvas.begin();graphic.points.forEach((point,index)=>{const [px,py]=local(point);index?canvas.lineTo(px,py):canvas.moveTo(px,py)});canvas.stroke()}
      else if(graphic.type==="polygon"&&graphic.points?.length){canvas.begin();graphic.points.forEach((point,index)=>{const [px,py]=local(point);index?canvas.lineTo(px,py):canvas.moveTo(px,py)});canvas.close();filled?canvas.fillAndStroke():canvas.stroke()}
      else if((graphic.type==="rectangle"||graphic.type==="ellipse")&&graphic.extent?.length===2){const [a,b]=graphic.extent,p1=local([Math.min(a[0],b[0]),Math.max(a[1],b[1])]),shapeWidth=Math.abs(b[0]-a[0])*scale,shapeHeight=Math.abs(b[1]-a[1])*scale;if(graphic.type==="rectangle")graphic.radius?canvas.roundrect(p1[0],p1[1],shapeWidth,shapeHeight,graphic.radius*scale,graphic.radius*scale):canvas.rect(p1[0],p1[1],shapeWidth,shapeHeight);else canvas.ellipse(p1[0],p1[1],shapeWidth,shapeHeight);filled?canvas.fillAndStroke():canvas.stroke()}
      else if(graphic.type==="text"&&graphic.extent?.length===2){const [a,b]=graphic.extent,p1=local([Math.min(a[0],b[0]),Math.max(a[1],b[1])]),boxWidth=Math.abs(b[0]-a[0])*scale,boxHeight=Math.abs(b[1]-a[1])*scale,text=(graphic.text||"").replace(/%([A-Za-z_]\w*)/g,(_,key)=>key==="name"?component.name:key==="controllerType"?(values[key]?.split(".").pop()||component.type.split(".").pop()||""):compactNumber(values[key]||"")),alignment=(graphic.alignment||"Center").toLowerCase(),horizontal=alignment.includes("left")?"left":alignment.includes("right")?"right":"center",anchorX=horizontal==="left"?p1[0]:horizontal==="right"?p1[0]+boxWidth:p1[0]+boxWidth/2,anchorY=p1[1]+boxHeight/2;canvas.setFontColor(color(graphic.textColor,"#111"));canvas.setFontFamily("Arial, Microsoft YaHei, sans-serif");canvas.setFontSize((graphic.fontSize||Math.max(11,Math.abs(b[1]-a[1])*.55))*scale);canvas.text(anchorX,anchorY,boxWidth,boxHeight,text,horizontal,"middle",false,"","visible",false,0,"ltr")}
      canvas.restore();
    }
  }
}
ShapeRegistry.add("modelicaComponent",ModelicaComponentShape);

function connectorPoint(component:Component,port:string,icon:TypeIcon|undefined,center:{x:number;y:number},width:number,height:number):DiagramPoint|null{
  const connector=icon?.connectors.find(item=>item.name===port);if(!connector||!icon)return null;
  const [[xmin,ymin],[xmax,ymax]]=icon.extent,[cx,cy]=connector.placement.origin,iconWidth=Math.abs(xmax-xmin),iconHeight=Math.abs(ymax-ymin);if(!iconWidth||!iconHeight)return null;
  const scale=Math.min(width/iconWidth,height/iconHeight),drawWidth=iconWidth*scale,drawHeight=iconHeight*scale,localX=(width-drawWidth)/2+(cx-xmin)*scale-width/2,localY=(height-drawHeight)/2+(ymax-cy)*scale-height/2,angle=-(component.placement?.rotation||0)*Math.PI/180,cos=Math.cos(angle),sin=Math.sin(angle);
  return [center.x+localX*cos-localY*sin,center.y+localX*sin+localY*cos];
}

const centerPerimeter:PerimeterFunction=bounds=>new Point(bounds.getCenterX(),bounds.getCenterY());
function portStyle(type:string,rotation:number):CellStyle{
  const leaf=type.split(".").pop()||"",input=leaf.includes("Input"),output=leaf.includes("Output"),positive=leaf.includes("PositivePin"),negative=leaf.includes("NegativePin"),flangeA=/Flange_a$/i.test(leaf),flangeB=/Flange_b$/i.test(leaf),pin=positive||negative||leaf.endsWith("Pin"),flange=flangeA||flangeB||leaf.includes("Flange");
  const shape=input||output?"triangle":pin?"rectangle":"ellipse",filled=input||positive||flangeA;
  const signal=input||output,mechanical=flange||leaf.includes("Support"),strokeColor=signal?"#00007f":pin?"#0000ff":mechanical?"#4a4a4a":"#405a56";
  return {shape,direction:"east",rotation,perimeter:centerPerimeter,fillColor:filled?strokeColor:"#ffffff",strokeColor,strokeWidth:.75,resizable:false,movable:false,editable:false};
}

function connectorSize(connector:TypeIcon["connectors"][number],iconScale:number){
  const [[x1,y1],[x2,y2]]=connector.placement.extent||[[-10,-10],[10,10]],declared=Math.max(Math.abs(x2-x1),Math.abs(y2-y1))*iconScale,leaf=connector.type.split(".").pop()||"",signal=leaf.includes("Input")||leaf.includes("Output");
  return Math.max(signal?4:2.5,Math.min(signal?10:5,declared));
}

function signalPortRotation(type:string,position:DiagramPoint,center:{x:number;y:number},fallback:number){
  const leaf=type.split(".").pop()||"",input=leaf.includes("Input"),output=leaf.includes("Output");if(!input&&!output)return fallback;
  let dx=position[0]-center.x,dy=position[1]-center.y;if(input){dx=-dx;dy=-dy}
  if(Math.abs(dx)>=Math.abs(dy))return dx>=0?0:180;
  return dy>=0?90:270;
}

function routedConnectionPoints(connection:ModelClass["connections"][number],from:string,to:string,portPositions:Map<string,DiagramPoint>,mapPoint:(point:DiagramPoint)=>{x:number;y:number}){
  const source=portPositions.get(from),target=portPositions.get(to),raw=(connection.points||[]).map(item=>mapPoint(item));if(raw.length<2||!source||!target)return raw.map(item=>new Point(item.x,item.y));
  const points=raw.map(item=>({x:item.x,y:item.y})),last=points.length-1,sourcePoint={x:source[0],y:source[1]},targetPoint={x:target[0],y:target[1]};points[0]=sourcePoint;points[last]=targetPoint;
  if(points.length===2&&sourcePoint.x!==targetPoint.x&&sourcePoint.y!==targetPoint.y){const horizontal=Math.abs(raw[1].x-raw[0].x)>=Math.abs(raw[1].y-raw[0].y);points.splice(1,0,horizontal?{x:targetPoint.x,y:sourcePoint.y}:{x:sourcePoint.x,y:targetPoint.y})}
  else if(points.length>2){const firstHorizontal=Math.abs(raw[1].x-raw[0].x)>=Math.abs(raw[1].y-raw[0].y),lastHorizontal=Math.abs(raw[last].x-raw[last-1].x)>=Math.abs(raw[last].y-raw[last-1].y);if(firstHorizontal)points[1].y=sourcePoint.y;else points[1].x=sourcePoint.x;if(lastHorizontal)points[points.length-2].y=targetPoint.y;else points[points.length-2].x=targetPoint.x}
  return points.map(item=>new Point(item.x,item.y));
}

export function MaxGraphDiagram({cls,icons,selected,onSelect}:Props){
  const containerRef=useRef<HTMLDivElement>(null),runtimeRef=useRef<GraphRuntime|null>(null),onSelectRef=useRef(onSelect),selectedRef=useRef(selected);
  const [zoom,setZoom]=useState(100);
  onSelectRef.current=onSelect;selectedRef.current=selected;
  const placed=cls?.components.filter(component=>component.placement?.visible)||[];

  useEffect(()=>{
    const container=containerRef.current;if(!container||!cls||!placed.length)return;
    const graph=new Graph(container);graph.setPanning(true);graph.setConnectable(false);graph.setCellsMovable(false);graph.setCellsResizable(false);graph.setCellsEditable(false);graph.setCellsDeletable(false);graph.setCellsBendable(false);graph.centerZoom=true;
    const panning=graph.getPlugin("PanningHandler") as {useLeftButtonForPanning:boolean;ignoreCell:boolean}|undefined;if(panning){panning.useLeftButtonForPanning=true;panning.ignoreCell=false}
    const parent=graph.getDefaultParent(),componentCells=new Map<string,Cell>(),portCells=new Map<string,Cell>(),portPositions=new Map<string,DiagramPoint>();runtimeRef.current={graph,componentCells};
    const extent=cls.diagram?.extent||[[-100,-100],[100,100]],[[xmin,ymin],[xmax,ymax]]=extent,scale=2.4,pad=55,point=([x,y]:DiagramPoint)=>({x:(x-xmin)*scale+pad,y:(ymax-y)*scale+pad});
    graph.batchUpdate(()=>{
      for(const component of placed){
        const placement=component.placement!,center=point(placement.origin),width=Math.max(28,Math.abs(placement.extent[1][0]-placement.extent[0][0])*scale),height=Math.max(28,Math.abs(placement.extent[1][1]-placement.extent[0][1])*scale),icon=icons[component.type];
        const cell=graph.insertVertex({parent,id:`component:${component.name}`,value:"",position:[center.x-width/2,center.y-height/2],size:[width,height],style:{shape:"modelicaComponent",fillColor:"none",strokeColor:"none",rotation:-(placement.rotation||0),resizable:false,movable:false,editable:false}});componentShapeData.set(cell,{icon,component});componentCells.set(component.name,cell);
        for(const connector of icon?.connectors.filter(item=>connectorIsVisible(icon,component,item))||[]){const key=`${component.name}.${connector.name}`,position=connectorPoint(component,connector.name,icon,center,width,height);if(!position)continue;const iconWidth=Math.abs(icon.extent[1][0]-icon.extent[0][0]),iconHeight=Math.abs(icon.extent[1][1]-icon.extent[0][1]),iconScale=Math.min(width/iconWidth,height/iconHeight),size=connectorSize(connector,iconScale),fallback=(connector.placement.rotation||0)-(placement.rotation||0),rotation=signalPortRotation(connector.type,position,center,fallback),port=graph.insertVertex({parent,id:`port:${key}`,value:"",position:[position[0]-size/2,position[1]-size/2],size:[size,size],style:portStyle(connector.type,rotation)});portCells.set(key,port);portPositions.set(key,position)}
      }
      cls.connections.forEach((connection,index)=>{const from=connection.from||connection.left||"",to=connection.to||connection.right||"",fromName=from.split(".")[0],toName=to.split(".")[0],source=portCells.get(from)||componentCells.get(fromName),target=portCells.get(to)||componentCells.get(toName);if(!source||!target)return;const explicit=Boolean(connection.points&&connection.points.length>=2),strokeColor=connection.lineColor?`rgb(${connection.lineColor.join(",")})`:"#405a56",edge=graph.insertEdge({parent,id:`connection:${index}:${from}:${to}`,value:"",source,target,style:{edgeStyle:explicit?undefined:"orthogonalEdgeStyle",noEdgeStyle:explicit,rounded:false,orthogonalLoop:true,jettySize:"auto",strokeColor,strokeWidth:.8,endArrow:"none",startArrow:"none",editable:false,movable:false,resizable:false}});if(explicit){const geometry=edge.getGeometry()?.clone()||new Geometry(),routed=routedConnectionPoints(connection,from,to,portPositions,point);geometry.points=routed.slice(1,-1);edge.setGeometry(geometry)}});
    });
    const selectionListener=()=>{const cell=graph.getSelectionCell(),id=cell?.getId()||"";if(id.startsWith("component:")){const component=placed.find(item=>item.name===id.slice("component:".length));if(component)onSelectRef.current(component)}};graph.getSelectionModel().addListener(InternalEvent.CHANGE,selectionListener);
    const zoomListener=()=>setZoom(Math.round(graph.getView().getScale()*100));graph.getView().addListener(InternalEvent.SCALE,zoomListener);graph.getView().addListener(InternalEvent.SCALE_AND_TRANSLATE,zoomListener);
    const fit=()=>{graph.getPlugin<FitPlugin>("fit")?.fit({border:48});graph.center(true,true);zoomListener()};requestAnimationFrame(fit);
    const wheel=(event:WheelEvent)=>{event.preventDefault();event.deltaY<0?graph.zoomIn():graph.zoomOut();zoomListener()};
    const keydown=(event:KeyboardEvent)=>{if(event.key==="+"||event.key==="="){graph.zoomIn();zoomListener()}else if(event.key==="-"){graph.zoomOut();zoomListener()}else if(event.key==="0"){fit()}};
    container.addEventListener("wheel",wheel,{passive:false});container.addEventListener("keydown",keydown);const observer=new ResizeObserver(()=>{if(graph.getGraphBounds().width)fit()});observer.observe(container);
    return()=>{observer.disconnect();container.removeEventListener("wheel",wheel);container.removeEventListener("keydown",keydown);graph.getSelectionModel().removeListener(selectionListener);graph.getView().removeListener(zoomListener);runtimeRef.current=null;graph.destroy();container.replaceChildren()};
  },[cls?.id,icons]);

  useEffect(()=>{const runtime=runtimeRef.current;if(!runtime)return;const cell=selected?runtime.componentCells.get(selected.name):null;if(cell&&runtime.graph.getSelectionCell()!==cell)runtime.graph.setSelectionCell(cell);else if(!cell&&!runtime.graph.isSelectionEmpty())runtime.graph.clearSelection()},[selected?.name,cls?.id]);
  if(!cls||!placed.length)return <div className="workspace-empty"><strong>当前模型没有图形位置信息</strong><span>请在 Modelica 组件中补充 Placement annotation。</span></div>;
  const run=(action:(graph:Graph)=>void)=>{const graph=runtimeRef.current?.graph;if(!graph)return;action(graph);setZoom(Math.round(graph.getView().getScale()*100))};
  const fit=()=>run(graph=>{graph.getPlugin<FitPlugin>("fit")?.fit({border:48});graph.center(true,true)});
  return <div className="diagram-stage maxgraph-stage" role="application" aria-label="组件连接画布，可滚轮缩放并拖拽平移">
    <div ref={containerRef} className="maxgraph-container" tabIndex={0}/>
    <div className="diagram-zoom"><button onClick={()=>run(graph=>graph.zoomOut())} title="缩小（-）">−</button><button className="zoom-value" onClick={()=>run(graph=>{graph.zoomActual();graph.center(true,true)})} title="恢复 100%">{zoom}%</button><button onClick={()=>run(graph=>graph.zoomIn())} title="放大（+）">＋</button><button onClick={fit} title="适应窗口（0）">适应</button></div>
    <span className="canvas-hint">滚轮缩放 · 空白处拖拽平移 · 双击组件查看参数</span>
  </div>;
}
