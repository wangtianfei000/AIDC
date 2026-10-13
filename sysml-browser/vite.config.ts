import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  base: './',
  // npm workspaces may hoist the React plugin beside another Vite major used
  // by sibling tools. Runtime compatibility is covered by the production build.
  plugins: [react() as any],
  server: { host: '127.0.0.1', port: 5173 },
  preview: { host: '127.0.0.1', port: 4173 },
});

