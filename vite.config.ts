import { defineConfig } from "vite";
import type { Plugin, ResolvedServerUrls } from "vite";
import { resolve } from "node:path";
import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";

function externalizeArtifacts(): Plugin {
  return {
    name: "motif-wasm-externalize-artifacts",
    resolveId(id) {
      if (id.startsWith("/artifacts/")) {
        return { id, external: true };
      }
    },
  };
}

function serveBuildArtifactsRaw(): Plugin {
  return {
    name: "motif-wasm-serve-build-artifacts-raw",
    configureServer(server) {
      server.middlewares.use((req, res, next) => {
        if (!req.url) return next();
        const pathname = req.url.split("?")[0];
        if (!pathname.startsWith("/artifacts/")) return next();
        const filePath = resolve(__dirname, "build" + pathname);
        if (!existsSync(filePath)) return next();
        try {
          const ext = pathname.split(".").pop()?.toLowerCase();
          const mimeTypes: Record<string, string> = {
            html: "text/html",
            js: "application/javascript",
            wasm: "application/wasm",
            data: "application/octet-stream",
          };
          res.setHeader("Content-Type", mimeTypes[ext ?? ""] ?? "application/octet-stream");
          res.setHeader("Cache-Control", "no-cache");
          res.statusCode = 200;
          res.end(readFileSync(filePath));
        } catch {
          res.statusCode = 404;
          res.end("Not found");
        }
      });
    },
  };
}

function listDemos(): { name: string; path: string }[] {
  const examplesDir = resolve(__dirname, "examples");
  if (!existsSync(examplesDir)) return [];
  return readdirSync(examplesDir)
    .filter((name) => {
      const entry = resolve(examplesDir, name, "index.html");
      return statSync(resolve(examplesDir, name)).isDirectory() && existsSync(entry);
    })
    .map((name) => ({ name, path: `/examples/${name}/` }));
}

function printDemoUrls(): Plugin {
  return {
    name: "motif-wasm-print-demo-urls",
    configureServer(server) {
      const originalPrint = server.printUrls.bind(server);
      server.printUrls = () => {
        originalPrint();
        const demos = listDemos();
        if (demos.length === 0) return;
        const urls: ResolvedServerUrls | null = server.resolvedUrls;
        const bases = urls ? [...urls.local, ...urls.network] : [];
        const base = bases[0]?.replace(/\/$/, "") ?? "";
        console.log("\n  \x1b[1mDemos\x1b[0m:");
        for (const d of demos) {
          console.log(`    \x1b[36m${d.name.padEnd(10)}\x1b[0m ${base}${d.path}`);
        }
        console.log("");

        const artifacts = resolve(__dirname, "build/artifacts");
        if (!existsSync(artifacts)) {
          console.warn(
            "\x1b[33m  build/artifacts/ not found. " +
              "Run 'pnpm build:motif' first.\x1b[0m\n"
          );
        }
      };
    },
  };
}

export default defineConfig({
  root: ".",
  publicDir: "public",

  resolve: {
    alias: {
      '@em-x11': resolve(__dirname, '../em-x11/src'),
    },
  },

  plugins: [externalizeArtifacts(), serveBuildArtifactsRaw(), printDemoUrls()],

  server: {
    headers: {
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
    fs: {
      // em-x11 is a sibling project; twm-session imports from its src/
      allow: [".", "build", "../em-x11"],
    },
  },

  build: {
    outDir: "dist",
  },
});
