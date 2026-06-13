import { defineConfig } from "vite";

export default defineConfig({
  base: process.env.VITE_BASE ?? "/",
  build: {
    outDir: "dist",
    emptyOutDir: true,
    rollupOptions: {
      output: {
        assetFileNames: (info) => {
          const name = info.names?.[0] ?? info.name ?? "asset";
          if (name.endsWith(".ts")) {
            return `assets/${name.slice(0, -3)}-[hash].js`;
          }
          return "assets/[name]-[hash][extname]";
        },
      },
    },
  },
  worker: {
    format: "es",
  },
});
