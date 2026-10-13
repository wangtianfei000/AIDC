# P4 详细装配基线

本目录的主模型已经从方盒包络研究改为现有十轴详细机械装配。模型保留真实设计曲面、孔位、壳体、连接件、脚、头部零件和 HD-1910 尺寸图重建几何，不再用长方体代替机械零件。

## 当前主交付

- `jingang-10axis-layout.FCStd`：约 8.8 MB 的详细 FreeCAD 装配副本，包含 212 个文档对象和 83 个叶级有限几何对象；
- `jingang-10axis-layout-preview.png`：详细装配预览；
- `p4-layout-manifest.json`：83 个几何对象、19 个 SysML ID 映射和 30 个打印文件记录；
- `p4-detailed-parts.csv`：30 个打印 STL、39 个实物打印件的尺寸、体积、三角面数和 SHA-256；
- `p4-validation.json`：详细几何校验与当前配置阻断项；
- `build_p4_layout.py`、`verify_p4_layout.py`：详细装配构建和校验脚本；
- `audit_existing_detailed_assembly.py`、`detailed-assembly-audit.json`：采用现有模型前的审计工具和结果。

原先的简化方盒模型只保留为 `build_p4_envelope_study.py`，不得再作为 P4 主装配或零件设计依据。

## 已核对内容

- SysML 的 19 个 `componentId` 与详细 CAD 对象一一对应；
- 十个 HD-1910 对象的名义外廓均为 `20 × 29 × 34 mm` 的不同姿态排列，并保留孔型重建网格；
- 30 个打印 STL 的 SHA-256 和尺寸与冻结清单一致；
- 30 个 STL 对应 39 个实物打印件，来源校验记录均为封闭、单实体网格；
- 躯干、左右腿、双脚、头颈、壳体、连接件、轴环和电子件参考模型均保留实际形状。

## 尚未满足“每个零件都正确”的部分

详细机械几何已经接入，但 P4 退出条件暂不满足：

1. `SYS-COMPUTE` 当前 CAD 是 Raspberry Pi Zero 2 W，项目资料中又存在 Radxa Zero 3W 方案；
2. `SYS-MOTION-CONTROL` 当前 CAD 是 RPI Robot HAT，第一台机器实施说明又采用 OpenRB-150；
3. `SYS-SENSING` 仍是 `25.8 × 15.5 mm` IMU 包络，SKU、孔位、连接器和元件高度没有冻结；
4. HD-1910 已按现有尺寸图重建外壳和孔型，但花键、螺纹、盲孔深度及实物配合仍需正式图纸或样件验证。

在上述配置冻结以前，模型状态为 `NOT_RELEASED_DETAILED_REVIEW_REQUIRED`。

## 复现

```powershell
& 'tools/freecad/1.1.3/bin/freecadcmd.exe' -c "import runpy; runpy.run_path(r'AIDC/samples/jingang-10axis-system-design/layout/build_p4_layout.py', run_name='__main__')"
& 'tools/freecad/1.1.3/bin/freecadcmd.exe' -c "import runpy; runpy.run_path(r'AIDC/samples/jingang-10axis-system-design/layout/verify_p4_layout.py', run_name='__main__')"
python AIDC/samples/jingang-10axis-system-design/layout/render_p4_preview.py
```
