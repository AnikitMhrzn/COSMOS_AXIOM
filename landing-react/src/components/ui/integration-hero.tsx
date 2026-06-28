// integration-hero.tsx
import { Button } from "@/components/ui/button";
import {
  Camera,
  ScanFace,
  HardHat,
  ShieldCheck,
  Siren,
  Activity,
  Gauge,
  Cpu,
  Server,
  Database,
  Wifi,
  BellRing,
  Mail,
  Video,
  Boxes,
  type LucideIcon,
} from "lucide-react";

const DASHBOARD_URL =
  (import.meta as any).env?.VITE_DASHBOARD_URL || "http://localhost:8080/dashboard";

const ICONS_ROW1: LucideIcon[] = [
  Camera,
  ScanFace,
  HardHat,
  ShieldCheck,
  Siren,
  Activity,
  Gauge,
];

const ICONS_ROW2: LucideIcon[] = [
  Cpu,
  Server,
  Database,
  Wifi,
  BellRing,
  Mail,
  Video,
];

// Repeat enough times so the marquee can loop seamlessly (-50% shift).
const repeated = (icons: LucideIcon[], repeat = 4) =>
  Array.from({ length: repeat }).flatMap(() => icons);

const IconChip = ({ Icon }: { Icon: LucideIcon }) => (
  <div className="h-16 w-16 flex-shrink-0 rounded-full border border-border bg-secondary/40 shadow-[0_4px_24px_rgba(0,0,0,0.45)] flex items-center justify-center">
    <Icon className="h-7 w-7 text-muted-foreground" strokeWidth={1.5} />
  </div>
);

export default function IntegrationHero() {
  return (
    <section className="relative z-10 min-h-screen flex flex-col justify-center overflow-hidden py-32">
      {/* blend out of the cosmos hero: transparent at the top (live scene shows
          through) easing down into solid black for the content below */}
      <div className="pointer-events-none absolute inset-0 bg-gradient-to-b from-transparent from-0% via-[#05060a] via-[30%] to-[#05060a]" />
      {/* subtle dot grid, masked so it only appears once the backdrop is solid */}
      <div className="absolute inset-0 bg-[radial-gradient(circle_at_center,rgba(255,255,255,0.05)_1px,transparent_1px)] [background-size:24px_24px] [mask-image:linear-gradient(to_bottom,transparent,black_32%)]" />

      <div className="relative max-w-7xl mx-auto px-6 text-center">
        <span className="inline-flex items-center gap-2 px-3 py-1 mb-5 text-xs uppercase tracking-[0.2em] rounded-full border border-border bg-secondary/40 text-muted-foreground">
          <Boxes className="h-3.5 w-3.5" /> Integrations
        </span>
        <h1 className="text-4xl lg:text-6xl font-bold tracking-tight text-foreground">
          Fits into your safety stack
        </h1>
        <p className="mt-5 text-lg text-muted-foreground max-w-xl mx-auto">
          Protego plugs into the cameras, sensors and alerting tools you already
          run — no rip-and-replace, no new hardware.
        </p>
        <Button
          variant="cosmic"
          size="lg"
          className="mt-8"
          onClick={() => (window.location.href = DASHBOARD_URL)}
        >
          Get started →
        </Button>

        {/* Marquee */}
        <div className="mt-16 overflow-hidden relative pb-2">
          <div className="flex gap-10 w-max animate-scroll-left">
            {repeated(ICONS_ROW1, 4).map((Icon, i) => (
              <IconChip key={i} Icon={Icon} />
            ))}
          </div>
          <div className="flex gap-10 w-max mt-6 animate-scroll-right">
            {repeated(ICONS_ROW2, 4).map((Icon, i) => (
              <IconChip key={i} Icon={Icon} />
            ))}
          </div>

          {/* edge fades */}
          <div className="absolute left-0 top-0 h-full w-24 bg-gradient-to-r from-[#05060a] to-transparent pointer-events-none" />
          <div className="absolute right-0 top-0 h-full w-24 bg-gradient-to-l from-[#05060a] to-transparent pointer-events-none" />
        </div>
      </div>
    </section>
  );
}
