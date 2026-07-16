# Roadmap

Guiding principle: **score outfits, not items.** The current engine ranks items
per category and lets the Gemini stylist prompt carry all combination logic;
the phases below move harmony, formality and learning into our own algorithm.

## Phase 1 — Outfit quality (the algorithmic core)

- [x] **1.1 Formality & style metadata** — `formality` column (0 sporty … 5 formal)
  on every catalog type; map the already-parsed `occasion` (work/casual/sport/
  date/special) to a target formality and penalize distance from it.
- [x] **1.2 Combination-level scoring** — enumerate combinations of the top
  candidates per category and score them as a whole: sum of item scores +
  color-harmony term (neutrals pair with everything, clash pairs penalized) +
  formality-consistency term + bold-pattern-count penalty. One-piece garments
  replace top+bottom in enumeration. The rule-based fallback then produces
  coherent outfits on its own; Gemini refines instead of rescuing.
- [x] **1.3 Structured feedback** — one-tap chips under the stars: too hot,
  too cold, colors clash, too formal, too sporty, uncomfortable. Tags are
  stored per rating (`feedback_tags`) and immediately steer the retry after
  a poor rating (temperature shift, formality target).
- [x] **1.4 Outfit memory** — learn pair affinities from highly rated
  recommendations (items that shine together get a pair bonus in 1.2):
  +0.05 per >= 4-star co-occurrence, capped at 0.15 per pair, applied in
  combination scoring.
- [x] **1.5 Temperature auto-calibration** — "too hot/cold" feedback nudges a
  per-type personal offset on the comfort range (`type_temp_offsets`):
  ±0.5 °C per tagged rating, cumulative, bounded to ±5 °C, applied when
  scoring temperature fit.

## Phase 2 — Real-life state

- [x] **2.1 "I wore this" button + wear history** (`wear_history` table) —
  one tap under a recommendation logs a wear for every wardrobe piece in it.
- [x] **2.2 Variety** — decaying recency penalty (−0.3/days), random choice
  among outfits within 0.05 of the best (wardrobe only), +0.05 exploration
  bonus for never-worn pieces.
- [x] **2.3 Laundry state** — clean/dirty flag with a per-category wear
  budget (tops 2, bottoms 4, outerwear 8; shoes & co never), auto-dirty when
  the budget runs out, dirty items excluded from candidates; washing resets.

## Phase 3 — A stylist that knows you

- [x] **3.1 Style profile** — attribute preference weights learned from
  ratings: each value slug carries a mean rating signal in [-1, 1] damped
  by observation count; ±0.1/point (capped ±0.3) in wardrobe scoring.
- [x] **3.2 Structured explanation + confidence score** — a rule-based
  checklist (weather fit, rain readiness, formality, colors, patterns,
  loved pairs, style match) plus a deterministic 35-97% confidence, shown
  as a meter over the stylist's sentence.
- [x] **3.3 Wardrobe analytics** — ANALYSIS tab: stat tiles (total, laundry,
  never worn, cutouts), category and color distribution bars, most-worn
  list, and temperature-band gap analysis for the core categories.
  (Cost-per-wear deferred: no price data yet.)

## Phase 4 — Efficiency & platform (as needed)

- Near-term: photo-hash classification cache, weather/geocode caching,
  batching several photos per Gemini request.
- At mobile time: PWA packaging, device tokens, `user_id` columns, CI
  (GitHub Actions, vcpkg-cached CMake + tests).
- Done meanwhile: garment cutout extraction (local rembg/BiRefNet by
  default, Gemini image model + chroma keying as paid opt-in) with a
  lookbook wardrobe (cream gallery, category tabs, item detail panel with
  computed color palette) and an OUTFITS tab built from saved
  recommendations (now stored with wardrobe piece ids + explanation).
- Deliberately deferred: local ONNX/OpenCV models, SSE job queues, JWT/S3/
  Postgres, full outfit image generation (rendering a person wearing it).
