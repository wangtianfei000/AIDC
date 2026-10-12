package AtlasRobotRequirements {
    requirement def EmergencyStop {
        doc /*The robot shall enter a safe stopped state within 500 ms after an emergency stop command.*/
        attribute reqId :> String = "REQ-SAF-001";
        attribute status :> String = "approved";
        attribute discipline :> String = "safety";
        attribute variant :> String = "all";
        require constraint {stopTime <= 500};
    }

    requirement def PayloadCapacity {
        doc /*The robot shall carry a nominal payload of at least 12 kg.*/
        attribute reqId :> String = "REQ-MEC-004";
        attribute status :> String = "review";
        attribute discipline :> String = "mechanical";
        attribute variant :> String = "industrial";
        require constraint {payload >= 12};
    }

    requirement def PositioningAccuracy {
        doc /*The end effector positioning error shall not exceed 0.2 mm under nominal load.*/
        attribute reqId :> String = "REQ-CTL-012";
        attribute status :> String = "draft";
        attribute discipline :> String = "controls";
        attribute variant :> String = "precision";
        require constraint {positionError <= 0.2};
    }

    requirement emergencyStop : EmergencyStop;
    requirement payloadCapacity : PayloadCapacity;
    requirement positioningAccuracy : PositioningAccuracy;
}

