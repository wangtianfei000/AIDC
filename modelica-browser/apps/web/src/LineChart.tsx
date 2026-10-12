import { useEffect, useRef } from "react";
import { LineChart as EChartsLineChart } from "echarts/charts";
import { DataZoomComponent, GridComponent, LegendComponent, ToolboxComponent, TooltipComponent } from "echarts/components";
import { init, use } from "echarts/core";
import { CanvasRenderer } from "echarts/renderers";
import type { Series } from "./types";

use([EChartsLineChart,GridComponent,LegendComponent,TooltipComponent,ToolboxComponent,DataZoomComponent,CanvasRenderer]);

const palette=["#087f6c","#c56a18","#3977b7","#9a4d97"];
const fmt=(n:number)=>Math.abs(n)>=1000||Math.abs(n)<.001&&n!==0?n.toExponential(3):n.toFixed(4).replace(/\.0+$/,'').replace(/(\.\d*?)0+$/,'$1');

export function LineChart({data,unit}:{data:Series|null;unit?:string}){
  const host=useRef<HTMLDivElement>(null);
  useEffect(()=>{
    if(!host.current||!data||data.rows.length<2)return;
    const chart=init(host.current,undefined,{renderer:"canvas"});
    const names=data.columns.slice(1);
    chart.setOption({
      animation:false,
      color:palette,
      grid:{left:62,right:28,top:42,bottom:42,containLabel:true},
      legend:{top:4,left:4,itemWidth:18,itemHeight:3,textStyle:{fontFamily:"Consolas",fontSize:11,color:"#314844"}},
      tooltip:{
        trigger:"axis",renderMode:"richText",confine:true,
        axisPointer:{type:"cross",snap:true,crossStyle:{color:"#4f6f69",width:1,type:"dashed"},label:{backgroundColor:"#355c55",precision:6}},
        formatter:(input:unknown)=>{
          const items=(Array.isArray(input)?input:[input]) as Array<{seriesName:string;color:string;value:[number,number]}>;
          if(!items.length)return "";
          return [`时间  ${fmt(Number(items[0].value[0]))} s`,...items.map(item=>`${item.seriesName}  ${fmt(Number(item.value[1]))}${unit?` ${unit}`:""}`)].join("\n");
        }
      },
      toolbox:{right:6,top:0,feature:{dataZoom:{yAxisIndex:"none"},restore:{}},iconStyle:{borderColor:"#5e7b76"}},
      xAxis:{type:"value",name:"时间 / s",nameLocation:"middle",nameGap:34,min:"dataMin",max:"dataMax",axisLine:{lineStyle:{color:"#9baea9"}},axisLabel:{color:"#617873",fontFamily:"Consolas",formatter:(value:number)=>fmt(value)},splitLine:{show:false},axisPointer:{show:true}},
      yAxis:{type:"value",name:unit||"",nameLocation:"end",scale:true,axisLine:{show:true,lineStyle:{color:"#9baea9"}},axisLabel:{color:"#617873",fontFamily:"Consolas",formatter:(value:number)=>fmt(value)},splitLine:{lineStyle:{color:"#e6eeec"}},axisPointer:{show:true}},
      dataZoom:[{type:"inside",xAxisIndex:0,filterMode:"none"}],
      series:names.map((name,index)=>({name,type:"line",showSymbol:false,symbol:"none",sampling:"lttb",lineStyle:{width:2.2},emphasis:{focus:"series"},data:data.rows.map(row=>[row[0],row[index+1]])}))
    });
    const observer=new ResizeObserver(()=>chart.resize());
    observer.observe(host.current);
    return ()=>{observer.disconnect();chart.dispose()};
  },[data,unit]);
  if(!data||data.rows.length<2)return <div className="empty compact"><strong>暂无曲线数据</strong><span>选择一个已完成的运行和输出变量。</span></div>;
  return <div className="echarts-plot" ref={host} role="img" aria-label="仿真时序曲线"/>;
}
