import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import App from "./Workbench";
import "@maxgraph/core/css/common.css";
import "./styles.css";
createRoot(document.getElementById("root")!).render(<StrictMode><App/></StrictMode>);
