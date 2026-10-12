package WalkingRobotRequirements {
    // 简化示例：所有数值仅演示写法，不代表实际项目已经批准。

    package FunctionalRequirements {
        requirement def remoteControlWalking {
            attribute reqId :> String = "REQ-FUN-001";
            doc /*机器人应根据控制端发出的前进、后退、左转、右转和停止命令执行对应运动。*/
        }

        requirement def startupSelfCheck {
            attribute reqId :> String = "REQ-FUN-002";
            doc /*机器人开机后应完成自检；存在影响运动的故障时，不得进入可运动状态。*/
        }
    }

    package PerformanceRequirements {
        requirement def batteryEndurance {
            attribute reqId :> String = "REQ-PER-001";
            doc /*在电池充满、室温和额定负载条件下，机器人连续执行典型任务的续航时间应不少于 2 小时。*/
            attribute measuredEndurance :> ISQ::duration;
            attribute minimumEndurance :> ISQ::duration = 2[h];
            require constraint {measuredEndurance >= minimumEndurance};
        }

        requirement def walkingSpeed {
            attribute reqId :> String = "REQ-PER-002";
            doc /*机器人在室内硬质平地和额定负载条件下的稳定前进速度应不低于 0.5 m/s。*/
            attribute measuredWalkingSpeed :> ISQ::speed;
            attribute minimumWalkingSpeed :> ISQ::speed = 0.5[m/s];
            require constraint {measuredWalkingSpeed >= minimumWalkingSpeed};
        }

        requirement def payloadCapacity {
            attribute reqId :> String = "REQ-PER-003";
            doc /*机器人在正常运动时应能够携带不少于 2 kg 的有效负载。*/
            attribute measuredPayload :> ISQ::mass;
            attribute minimumPayload :> ISQ::mass = 2[kg];
            require constraint {measuredPayload >= minimumPayload};
        }
    }

    package SafetyRequirements {
        requirement def commandStopTime {
            attribute reqId :> String = "REQ-SAF-001";
            doc /*机器人正常行走时收到停止命令后，应在 500 ms 内停止主动运动。*/
            attribute measuredStopTime :> ISQ::duration;
            attribute maximumStopTime :> ISQ::duration = 500[ms];
            require constraint {measuredStopTime <= maximumStopTime};
        }

        requirement def communicationLossResponse {
            attribute reqId :> String = "REQ-SAF-002";
            doc /*机器人运动期间与控制端通信中断时，应停止继续执行运动命令并进入安全状态。*/
        }
    }

    package PhysicalRequirements {
        requirement def totalMass {
            attribute reqId :> String = "REQ-PHY-001";
            doc /*机器人安装电池且不含外部测试设备时，整机质量应不超过 8 kg。*/
            attribute measuredMass :> ISQ::mass;
            attribute maximumMass :> ISQ::mass = 8[kg];
            require constraint {measuredMass <= maximumMass};
        }
    }

    package ServiceRequirements {
        requirement def batteryReplacementTime {
            attribute reqId :> String = "REQ-SVC-001";
            doc /*机器人断电后，受训人员使用规定工具更换电池的时间应不超过 5 分钟。*/
            attribute measuredReplacementTime :> ISQ::duration;
            attribute maximumReplacementTime :> ISQ::duration = 5[min];
            require constraint {measuredReplacementTime <= maximumReplacementTime};
        }
    }
}

