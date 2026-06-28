# Protego Landing (React + shadcn + TypeScript)

Standalone landing page for Protego, built with the Horizon/Cosmos hero
component (Three.js + GSAP). Includes a **Start Now** button that routes to the
Protego dashboards.

## Stack
- Vite + React 18 + TypeScript
- Tailwind CSS (shadcn project structure, `@/components/ui`)
- Three.js + GSAP for the animated cosmic hero
- shadcn-style `Button` (used for the Start Now CTA)

## Project structure
```
src/
  components/ui/
    horizon-hero-section.tsx   # the hero component
    demo.tsx                   # <Component /> wrapper (DemoOne)
    button.tsx                 # shadcn Button
  lib/utils.ts                 # cn() helper
  App.tsx, main.tsx, index.css
components.json                # shadcn config
```
The `@` alias points to `src/` (see `vite.config.ts` and `tsconfig.json`).
shadcn convention puts reusable primitives in `@/components/ui` so the CLI and
imports (`@/components/ui/button`) resolve consistently — keep new UI there.

## Run
```bash
npm install
npm run dev        # http://localhost:5173
npm run build      # production build into dist/
```

## Start Now target
The CTA navigates to `VITE_DASHBOARD_URL` (see `.env`), default
`http://localhost:8080/dashboard` — i.e. your running `run_protego.py`
dashboards. Change `.env` to point elsewhere (e.g. a deployed URL).

## Notes
- The original component was authored in loose JS; refs were typed as `any`
  and a few callback params annotated so it compiles under TypeScript. Logic is
  unchanged.
- Three.js postprocessing (`EffectComposer`, `UnrealBloomPass`) is imported from
  `three/examples/jsm/...` and bundled by Vite — no CDN needed.
