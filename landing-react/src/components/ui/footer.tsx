// footer.tsx
import { ArrowRight } from "lucide-react";

type Column = { title: string; links: string[] };

const COLUMNS: Column[] = [
  {
    title: "Product",
    links: [
      "Overview",
      "PPE Detection",
      "Identity & Access",
      "Environment Monitoring",
      "Dashboards",
      "Integrations",
    ],
  },
  {
    title: "Solutions",
    links: ["Construction", "Manufacturing", "Warehousing", "Energy & Utilities", "Logistics"],
  },
  {
    title: "Resources",
    links: ["Documentation", "API Reference", "Case Studies", "Deployment Guide", "System Status"],
  },
  {
    title: "Company",
    links: ["About", "Careers", "Engineering", "Newsroom", "Contact"],
  },
  {
    title: "Legal",
    links: ["Terms of Use", "Privacy", "Security", "Data Processing", "Compliance"],
  },
];

export default function Footer() {
  const handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
    e.preventDefault();
    e.currentTarget.reset();
  };

  return (
    <footer className="relative z-10 bg-[#05060a] border-t border-white/10">
      <div className="max-w-7xl mx-auto px-6 pt-20 pb-12">
        {/* Link columns */}
        <div className="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-x-8 gap-y-10">
          {COLUMNS.map((col) => (
            <div key={col.title}>
              <h3 className="text-xs uppercase tracking-[0.18em] text-muted-foreground mb-5">
                {col.title}
              </h3>
              <ul className="space-y-3">
                {col.links.map((link) => (
                  <li key={link}>
                    <a
                      href="#"
                      className="text-sm text-foreground/80 hover:text-foreground transition-colors"
                    >
                      {link}
                    </a>
                  </li>
                ))}
              </ul>
            </div>
          ))}
        </div>

        {/* Wordmark + mission + signup */}
        <div className="mt-20 flex flex-col gap-12 lg:flex-row lg:items-end lg:justify-between">
          <div className="max-w-md">
            <div className="text-5xl font-extrabold tracking-tight text-foreground select-none">
              PROTEGO
              <span className="align-super text-base text-muted-foreground">®</span>
            </div>
            <p className="mt-5 text-sm text-muted-foreground leading-relaxed">
              Protego turns every camera into a safety supervisor — real-time PPE,
              identity and environment compliance for the sites you run.
            </p>
          </div>

          <form onSubmit={handleSubmit} className="w-full max-w-md">
            <div className="flex items-center gap-3 border-b border-white/15 pb-2 focus-within:border-white/40 transition-colors">
              <input
                type="email"
                required
                placeholder="Enter your email"
                aria-label="Email address"
                className="flex-1 bg-transparent text-sm text-foreground placeholder:text-muted-foreground/70 outline-none"
              />
              <button
                type="submit"
                className="inline-flex items-center gap-1.5 text-sm uppercase tracking-wider text-muted-foreground hover:text-foreground transition-colors"
              >
                Submit <ArrowRight className="h-4 w-4" />
              </button>
            </div>
            <p className="mt-3 text-xs text-muted-foreground/70">
              By signing up, I agree with the data protection policy.
            </p>
          </form>
        </div>

        {/* Bottom bar */}
        <div className="mt-14 pt-6 border-t border-white/10 flex flex-col sm:flex-row items-center justify-between gap-3 text-xs text-muted-foreground/70">
          <span>© {new Date().getFullYear()} Protego. All rights reserved.</span>
          <span className="tracking-[0.18em] uppercase">Vision · Identity · Environment</span>
        </div>
      </div>
    </footer>
  );
}
