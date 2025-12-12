import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    host: true,
    // Must match NGINX config
    port: 3002,
    // See https://vite.dev/config/server-options.html#server-hmr
    strictPort: true,
    hmr: {
      clientPort: 3002,
    },
    headers: {
      // See https://developer.mozilla.org/en-US/docs/Web/API/Window/crossOriginIsolated#cross-origin_isolating_a_document
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "credentialless",
    },
    proxy: {
      '/api': {
        target: 'http://localhost:3001',
        changeOrigin: true,
      },
    },
  },
});
