#!/usr/bin/env python3
"""Materialize the Microduck Native12 Robot IR from the reviewed T2 CAD-aligned data."""
from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def unit(v):
    n = math.sqrt(sum(x*x for x in v))
    return [x/n for x in v]


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def quat_from_axes(nx, ny):
    x = unit(nx); z = unit(cross(x, ny)); y = cross(z, x)
    m00,m01,m02=x[0],y[0],z[0]; m10,m11,m12=x[1],y[1],z[1]; m20,m21,m22=x[2],y[2],z[2]
    trace=m00+m11+m22
    if trace > 0:
        s=math.sqrt(trace+1)*2; q=[(m21-m12)/s,(m02-m20)/s,(m10-m01)/s,0.25*s]
    elif m00 > m11 and m00 > m22:
        s=math.sqrt(1+m00-m11-m22)*2; q=[0.25*s,(m01+m10)/s,(m02+m20)/s,(m21-m12)/s]
    elif m11 > m22:
        s=math.sqrt(1+m11-m00-m22)*2; q=[(m01+m10)/s,0.25*s,(m12+m21)/s,(m02-m20)/s]
    else:
        s=math.sqrt(1+m22-m00-m11)*2; q=[(m02+m20)/s,(m12+m21)/s,0.25*s,(m10-m01)/s]
    q=unit(q)
    return [0.0 if abs(v)<1e-15 else v for v in q]


def inertia(ixx, iyy, izz, ixy=0, ixz=0, iyz=0):
    return {"ixx":ixx,"iyy":iyy,"izz":izz,"ixy":ixy,"ixz":ixz,"iyz":iyz}


def body(body_id, parent, mass, com, tensor):
    value={"bodyId":body_id,"translation_m":[0,0,0],"rotation_xyzw":[0,0,0,1],"mass_kg":mass,"centerOfMass_m":com,"inertia_kg_m2":tensor,"collisionAssetIds":[]}
    if parent:value["parentBodyId"]=parent
    return value


def joint(joint_id, parent, child, translation, nx, ny, start, lower, upper):
    return {"jointId":joint_id,"type":"revolute","parentBodyId":parent,"childBodyId":child,
        "translation_m":translation,"rotation_xyzw":quat_from_axes(nx,ny),"axis":[0,0,1],"zeroOffset_rad":0,"initialPosition_rad":start,
        "limit":{"lower":lower,"upper":upper,"velocity":9.42477796076938,"effort":1.176798},
        "damping_Nm_s_rad":0.001,"friction_Nm":0,"armature_kg_m2":0.0005}


pi=math.pi
bodies=[
 body("trunk",None,0.190757252019,[-0.0219402681457,-0.0000310782101326,0.0050751936443],inertia(0.000121877086842,0.000140424626415,0.000111752300357,8.45761930481e-8,-2.31949813718e-5,1.32427125149e-7)),
 body("left_hip_yaw_link","trunk",0.0260936428012,[0,0.000224984719393,0.0170589013038],inertia(4.66657311939e-6,3.92613104921e-6,2.56080242838e-6,-2.66720349501e-13,5.69997940388e-13,-8.93468852439e-8)),
 body("left_hip_roll_link","left_hip_yaw_link",0.00646946094685,[0.0135711386748,-1.3927024468e-7,-0.00674380986871],inertia(8.2523777e-7,1.15131852e-6,6.6064291e-7,1.9456e-12,3.743445e-7,9.05e-13)),
 body("left_upper_leg","left_hip_roll_link",0.0489429099486,[0.00704947642725,0.0222258924485,0.0130033115958],inertia(1.65258e-5,9.55866e-6,1.95918e-5,-4.7926615e-6,9.8973079e-8,2.10935e-7)),
 body("left_lower_leg","left_upper_leg",0.0253255203124,[0,0.0317301531184,-0.00918622779438],inertia(6.12776828e-6,2.54829155e-6,5.12572485e-6,0,0,9.05420652e-7)),
 body("left_foot","left_lower_leg",0.0310845592418,[-0.0070730653269,-0.01448479969,-0.0151804367344],inertia(5.88043e-6,1.201245e-5,8.23442e-6,-3.67618e-8,-3.99796e-9,-1.795e-7)),
 body("neck","trunk",0.0267910105167,[0.000173762584464,-0.0138415637574,0.0144999995348],inertia(8.79861589526e-6,2.5794181576e-6,7.96294176189e-6,1.505095979e-7,1.80134244396e-12,-7.10221126853e-13)),
 body("head","neck",0.167431338437,[0.005521871273,0.0000549439912213,0.0272006910139],inertia(0.000117725381103,0.000133646982551,0.000169526450326,2.76055054952e-7,3.69623420849e-6,-2.95192421629e-8)),
 body("right_hip_yaw_link","trunk",0.0260936428012,[0,0.000224984719393,0.0170589013038],inertia(4.66657311939e-6,3.92613104921e-6,2.56080242838e-6,-2.66720349501e-13,5.69997940388e-13,-8.93468852439e-8)),
 body("right_hip_roll_link","right_hip_yaw_link",0.00646946098075,[0.0135711386864,-1.39278945327e-7,-0.00674381000579],inertia(8.2523777e-7,1.15131852e-6,6.6064291e-7,1.9456e-12,3.743445e-7,9.05e-13)),
 body("right_upper_leg","right_hip_roll_link",0.0489433702001,[-0.00704935102022,0.0222261026586,0.0130033029184],inertia(1.65258e-5,9.55866e-6,1.95918e-5,4.7925265e-6,-9.8964109e-8,2.10935e-7)),
 body("right_lower_leg","right_upper_leg",0.0253255203124,[-0.0317301531274,0,-0.00918622779462],inertia(2.54829155e-6,6.12776828e-6,5.12572485e-6,0,-9.05420649e-7,0)),
 body("right_foot","right_lower_leg",0.0310844913308,[0.00707304571935,-0.0144848031164,-0.0151806684071],inertia(5.88043e-6,1.201245e-5,8.23442e-6,3.67438e-8,4.04245e-9,-1.795e-7)),
]
joints=[
 joint("left_hip_yaw","trunk","left_hip_yaw_link",[0.006,0.0175,-0.005],[0,1,0],[1,0,0],0,-0.4363323129985824,0.5235987755982988),
 joint("left_hip_roll","left_hip_yaw_link","left_hip_roll_link",[0,0.0165,0.0125],[1,0,0],[0,0,-1],-0.08726646259971647,-0.3839724354387525,0.3839724354387525),
 joint("left_hip_pitch","left_hip_roll_link","left_upper_leg",[0.025,0,-0.0185],[0,-1,0],[0,0,-1],-0.457924,-pi/2,pi/2),
 joint("left_knee","left_upper_leg","left_lower_leg",[0.022,0.0357771,-0.004],[0,1,0],[1,0,0],-0.00494,-pi/2,pi/2),
 joint("left_ankle","left_lower_leg","left_foot",[0,0.042,-0.026],[1,0,0],[0,-1,0],0.452984,-pi/2,pi/2),
 joint("neck_pitch","trunk","neck",[0.026,0.0145,0.0324215],[-1,0,0],[0,0,-1],0.3490658503988659,-pi/2,1.0471975511965976),
 joint("head_yaw","neck","head",[0.000446986125312,-0.0487719157139,0.0145],[-0.984807753014,0.173648177667,0],[0,0,-1],0,-2.967059728390373,2.967059728390348),
 joint("right_hip_yaw","trunk","right_hip_yaw_link",[0.006,-0.0175,-0.005],[0,1,0],[1,0,0],0,-0.5235987755982988,0.4363323129985824),
 joint("right_hip_roll","right_hip_yaw_link","right_hip_roll_link",[0,0.0165,0.0125],[-1,0,0],[0,0,1],0.08726646259971647,-0.3839724354387525,0.3839724354387525),
 joint("right_hip_pitch","right_hip_roll_link","right_upper_leg",[0.025,0,-0.0185],[0,-1,0],[0,0,-1],0.457924,-pi/2,pi/2),
 joint("right_knee","right_upper_leg","right_lower_leg",[-0.022,0.0357771,-0.004],[1,0,0],[0,-1,0],0.00494,-pi/2,pi/2),
 joint("right_ankle","right_lower_leg","right_foot",[-0.042,0,-0.026],[0,1,0],[1,0,0],-0.452984,-pi/2,pi/2),
]
contacts=[
 {"contactId":"trunk-box","bodyId":"trunk","shape":"box","translation_m":[-0.006,0,-0.005],"rotation_xyzw":[0,0,0,1],"halfExtents_m":[0.041,0.032,0.039]},
 {"contactId":"head-sphere","bodyId":"head","shape":"sphere","translation_m":[0.006,0,0.027],"rotation_xyzw":[0,0,0,1],"radius_m":0.034},
 {"contactId":"left-shin-capsule","bodyId":"left_lower_leg","shape":"capsule","translation_m":[0,0.02,-0.01],"rotation_xyzw":[0,0,0,1],"radius_m":0.012,"halfLength_m":0.03,"axis":[0,1,0]},
 {"contactId":"right-shin-capsule","bodyId":"right_lower_leg","shape":"capsule","translation_m":[-0.02,0,-0.01],"rotation_xyzw":[0,0,0,1],"radius_m":0.012,"halfLength_m":0.03,"axis":[1,0,0]},
 {"contactId":"left-foot-patch","bodyId":"left_foot","shape":"footPatch","translation_m":[0,0,0],"rotation_xyzw":[0,0,0,1],"points_m":[[-0.025,-0.018,-0.018],[-0.025,0.018,-0.018],[0.025,-0.018,-0.018],[0.025,0.018,-0.018]]},
 {"contactId":"right-foot-patch","bodyId":"right_foot","shape":"footPatch","translation_m":[0,0,0],"rotation_xyzw":[0,0,0,1],"points_m":[[-0.025,-0.018,-0.018],[-0.025,0.018,-0.018],[0.025,-0.018,-0.018],[0.025,0.018,-0.018]]},
]
physics={"schemaVersion":"aidc.robot.physics/0.1","modelId":"aidc.microduck.native12","modelRevision":"p4-r1","coordinateSystem":{"lengthUnit":"m","angleUnit":"rad","forward":"+X","left":"+Y","up":"+Z","geometryQuaternionOrder":"xyzw"},"rootBodyId":"trunk","rootType":"floating","rootInitialTranslation_m":[0,0,0.116719591352],"rootInitialRotation_xyzw":[0,0,0,1],"bodies":bodies,"joints":joints,"namedPoses":[{"name":"stand","jointPositions":{j["jointId"]:j["initialPosition_rad"]+j.get("zeroOffset_rad",0) for j in joints}}],"contactGeometries":contacts,"contactDefaults":{"friction":0.8,"contactMargin_m":0.0002,"excludedBodyPairs":[[j["parentBodyId"],j["childBodyId"]] for j in joints]}}
devices={"schemaVersion":"aidc.robot.devices/0.1","modelId":physics["modelId"],"modelRevision":physics["modelRevision"],"devices":[*[{"deviceId":"actuator-"+j["jointId"],"deviceType":"actuator","model":"AIDCRobotics.Actuators.IdealTorque","jointId":j["jointId"],"commandKind":"torque","gearRatio":320,"rotorInertia_kg_m2":0.0005,"viscousDamping_Nm_s_rad":0.001,"effortLimit_Nm":1.176798,"speedLimit_rad_s":9.42477796076938,"powerDeviceId":"battery"} for j in joints],{"deviceId":"battery","deviceType":"power","model":"AIDCRobotics.Electrical.SimpleBatteryBus","nominalVoltage_V":6.4,"capacity_Ah":1.8,"internalResistance_Ohm":0.12,"currentLimit_A":3.0}]}
commands=[];states=[];diagnostics=[]
for i,j in enumerate(joints,1):
    did="actuator-"+j["jointId"]
    commands.append({"name":j["jointId"]+"TorqueCommand","index":i,"semantic":"jointTorqueCommand","unit":"N.m","direction":1,"scale":1,"samplePeriod_s":0.001,"jointId":j["jointId"],"deviceId":did})
for j in joints:
    did="actuator-"+j["jointId"]
    for semantic,suffix,unit in (("jointPosition","Position","rad"),("jointVelocity","Velocity","rad/s"),("jointTorque","Torque","N.m")):
        states.append({"name":j["jointId"]+suffix,"index":len(states)+1,"semantic":semantic,"unit":unit,"direction":1,"scale":1,"samplePeriod_s":0.001,"jointId":j["jointId"],"deviceId":did})
    diagnostics.append({"name":j["jointId"]+"Valid","index":len(diagnostics)+1,"semantic":"valid","unit":"1","direction":1,"scale":1,"samplePeriod_s":0,"deviceId":did})
interfaces={"schemaVersion":"aidc.robot.interfaces/0.1","modelId":physics["modelId"],"modelRevision":physics["modelRevision"],"interfaceId":"aidc.microduck.native12.control","interfaceVersion":"1.0.0","publicQuaternionOrder":"wxyz","commands":commands,"states":states,"diagnostics":diagnostics}
for name,value in (("physics",physics),("devices",devices),("interfaces",interfaces)):
    (ROOT/f"{name}.json").write_text(json.dumps(value,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"bodies":len(bodies),"joints":len(joints),"mass_kg":sum(b["mass_kg"] for b in bodies)},ensure_ascii=False))
