import { defineConfig } from 'vite';
import { fileURLToPath, URL } from 'node:url';

// Base path: при деплое на GitHub Pages переопределяется через VITE_BASE.
// Для gerainnn.github.io/Dodsterp/ → VITE_BASE=/Dodsterp/
// Для wanderworld.github.io/wanderworld/ → VITE_BASE=/wanderworld/
const base = process.env.VITE_BASE ?? '/';

export default defineConfig({
  base,
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
  server: {
    host: true,
    port: 5173,
  },
  build: {
    target: 'es2022',
    sourcemap: false,
    rollupOptions: {
      output: {
        manualChunks: {
          three: ['three'],
        },
      },
    },
  },
});
