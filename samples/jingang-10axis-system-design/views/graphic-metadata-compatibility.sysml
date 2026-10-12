package DiagramGraphicMetadataFixture {
    part def GraphicMetadataFixture {
        @GraphicMetadata {
            graphicWidth = 640;
            graphicHeight = 320;
        }

        part source {
            @GraphicMetadata {
                graphicX = 60;
                graphicY = 100;
                graphicWidth = 140;
                graphicHeight = 80;
            }
            port outputPort {
                @GraphicMetadata {
                    graphicX = 140;
                    graphicY = 40;
                    portPosition = "right";
                    portOffset = 0;
                }
            }
        }

        part sink {
            @GraphicMetadata {
                graphicX = 440;
                graphicY = 100;
                graphicWidth = 140;
                graphicHeight = 80;
            }
            port inputPort {
                @GraphicMetadata {
                    graphicX = 0;
                    graphicY = 40;
                    portPosition = "left";
                    portOffset = 0;
                }
            }
        }

        connect source.outputPort to sink.inputPort {
            @GraphicMetadata {
                labelText = "fixture";
                pathType = "Orthogonal";
                routingStyle = "Manhattan";
            }
        }
    }
}
