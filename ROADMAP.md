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
- [ ] **1.4 Outfit memory** — learn pair affinities from highly rated
  recommendations (items that shine together get a pair bonus in 1.2).
- [ ] **1.5 Temperature auto-calibration** — "too hot/cold" feedback nudges a
  per-type personal offset on the comfort range (bounded, cumulative).

## Phase 2 — Real-life state

- [ ] **2.1 "I wore this" button + wear history** (`wear_history` table).
- [ ] **2.2 Variety** — decaying recency penalty (−C/days), weighted-random
  choice among near-tied candidates, small exploration bonus for rarely worn pieces.
- [ ] **2.3 Laundry state** — clean/dirty flag, auto-dirty after N wears,
  dirty items excluded from candidates.

## Phase 3 — A stylist that knows you

- [ ] **3.1 Style profile** — attribute preference weights learned from ratings.
- [ ] **3.2 Structured explanation + confidence score** — checklist reasons
  and a confidence percentage instead of a single sentence.
- [ ] **3.3 Wardrobe analytics** — distribution dashboard, gap analysis,
  missing-piece suggestions; optional cost-per-wear.

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
