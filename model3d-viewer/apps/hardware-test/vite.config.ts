import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import { fileURLToPath, URL } from 'node:url';
export default defineConfig({
  base:'/workbench/', plugins:[react()],
  publicDir:fileURLToPath(new URL('../../../../Jingang/apps/hardware-test/model3d/generated/',import.meta.url)),
  server:{proxy:{'/api':{target:'http://127.0.0.1:8765',changeOrigin:true}}},
});
