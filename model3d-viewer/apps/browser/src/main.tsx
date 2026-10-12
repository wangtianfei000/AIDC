import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import Model3DBrowser from './Model3DBrowser';
import './style.css';

createRoot(document.getElementById('app')!).render(
  <StrictMode>
    <div className="model3d-app-shell">
      <header className="model3d-app-header">
        <div><span>AIDC · CAD 发布模型浏览</span><h1>三维模型浏览器</h1></div>
        <small>设计 · 仿真 · 装配</small>
      </header>
      <main className="model3d-app-content"><Model3DBrowser /></main>
    </div>
  </StrictMode>,
);
