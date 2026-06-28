// testimonials.tsx
import { motion } from "motion/react";
import { TestimonialsColumn, type Testimonial } from "@/components/ui/testimonials-columns-1";

const testimonials: Testimonial[] = [
  {
    text: "Protego flags a missing hard hat before the worker reaches the line. Our near-miss reports dropped by half in a single quarter.",
    image: "https://randomuser.me/api/portraits/women/1.jpg",
    name: "Briana Patton",
    role: "Site Safety Manager",
  },
  {
    text: "We went from manual PPE spot-checks to continuous monitoring across every camera. Audits are no longer a scramble.",
    image: "https://randomuser.me/api/portraits/men/2.jpg",
    name: "Bilal Ahmed",
    role: "EHS Director",
  },
  {
    text: "Setup took an afternoon on our existing cameras. No new hardware, and the alerts land straight in our Teams channel.",
    image: "https://randomuser.me/api/portraits/women/3.jpg",
    name: "Saman Malik",
    role: "Operations Lead",
  },
  {
    text: "Identity and access checks mean only certified staff enter the hot zones. Compliance is finally something we can measure.",
    image: "https://randomuser.me/api/portraits/men/4.jpg",
    name: "Omar Raza",
    role: "Plant Manager",
  },
  {
    text: "Environment monitoring caught a gas threshold breach overnight and paged the on-call crew automatically.",
    image: "https://randomuser.me/api/portraits/women/5.jpg",
    name: "Zainab Hussain",
    role: "Facilities Engineer",
  },
  {
    text: "Insurers love the audit trail. Every incident is timestamped with footage, so disputes basically disappeared.",
    image: "https://randomuser.me/api/portraits/women/6.jpg",
    name: "Aliza Khan",
    role: "Risk & Compliance Officer",
  },
  {
    text: "Our crews actually trust it. The overlays are clear, not naggy, and false alarms are rare.",
    image: "https://randomuser.me/api/portraits/men/7.jpg",
    name: "Farhan Siddiqui",
    role: "Site Foreman",
  },
  {
    text: "Rolled it out across four sites in two weeks. The dashboards give leadership one source of truth.",
    image: "https://randomuser.me/api/portraits/women/8.jpg",
    name: "Sana Sheikh",
    role: "Regional HSE Manager",
  },
  {
    text: "Real-time PPE detection turned safety from a checklist into a live signal we can act on.",
    image: "https://randomuser.me/api/portraits/men/9.jpg",
    name: "Hassan Ali",
    role: "Quality & Safety Head",
  },
];

const firstColumn = testimonials.slice(0, 3);
const secondColumn = testimonials.slice(3, 6);
const thirdColumn = testimonials.slice(6, 9);

const Testimonials = () => {
  return (
    <section className="relative z-10 bg-background py-24">
      <div className="container z-10 mx-auto px-6">
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          whileInView={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.8, delay: 0.1, ease: [0.16, 1, 0.3, 1] }}
          viewport={{ once: true }}
          className="flex flex-col items-center justify-center max-w-[540px] mx-auto"
        >
          <div className="flex justify-center">
            <div className="border border-border py-1 px-4 rounded-lg text-xs uppercase tracking-[0.2em] text-muted-foreground">
              Feedback
            </div>
          </div>

          <h2 className="text-3xl sm:text-4xl lg:text-5xl font-bold tracking-tight mt-5 text-foreground text-center">
            Trusted on the floor
          </h2>
          <p className="text-center mt-5 text-muted-foreground">
            What safety, operations and compliance teams say after putting Protego on their sites.
          </p>
        </motion.div>

        <div className="flex justify-center gap-6 mt-12 [mask-image:linear-gradient(to_bottom,transparent,black_25%,black_75%,transparent)] max-h-[740px] overflow-hidden">
          <TestimonialsColumn testimonials={firstColumn} duration={15} />
          <TestimonialsColumn testimonials={secondColumn} className="hidden md:block" duration={19} />
          <TestimonialsColumn testimonials={thirdColumn} className="hidden lg:block" duration={17} />
        </div>
      </div>
    </section>
  );
};

export default Testimonials;
