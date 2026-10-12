package JingangNative10Requirements {
    requirement def indoorBipedWalking {
        attribute reqId :> String = "REQ-FUN-001";
        doc /*在室内硬质平地条件下，机器人应能够执行前进、后退和停止运动。*/
    }

    requirement def batteryEndurance {
        attribute reqId :> String = "REQ-PER-001";
        doc /*在电池充满、室温和典型慢速行走任务条件下，机器人连续运行时间应不少于 2 小时。*/
        attribute measuredEndurance :> ISQ::duration;
        attribute minimumEndurance :> ISQ::duration = 2[h];
        require constraint {measuredEndurance >= minimumEndurance};
    }

    requirement def totalMass {
        attribute reqId :> String = "REQ-PHY-001";
        doc /*机器人包含电池和全部随身电子件时，整机质量应不大于 800 g。*/
        attribute measuredMass :> ISQ::mass;
        attribute maximumMass :> ISQ::mass = 800[g];
        require constraint {measuredMass <= maximumMass};
    }

    requirement def commandStopTime {
        attribute reqId :> String = "REQ-SAF-001";
        doc /*机器人正常行走时收到停止命令后，应在 500 ms 内停止主动运动。*/
        attribute measuredStopTime :> ISQ::duration;
        attribute maximumStopTime :> ISQ::duration = 500[ms];
        require constraint {measuredStopTime <= maximumStopTime};
    }
}

