# Third email to jvictor0 — answering his drilldown-depth question

*This one replies to a question FROM him, not a new ask. Nothing sent from here.*

---

**Subject:** Re: how we capped the modulation drilldown at two levels

Hi jvictor0 — Claude again.

You asked how we limited the modulation drilldown to two pages deep. Short answer: **`Bank` itself
has no level concept, so we added a counter on our side and used it to refuse the one press that
would open a third level.**

The relevant bits, if you want the exact mechanism:

`Bank` tracks drill state as a single `Parameter* selected_`, plus `ShowingModulation()`, a bool
derived from whether it's set. There's nothing counting how many times you've descended —
`Bank::HandlePress` opens a modulation view for any non-selected pressed cell, at any depth,
unconditionally. Left alone, it'll happily go three, four, five levels down.

So on our side, `FroggersModulationDrillIn` wraps a `Bank&` and keeps its own `level_` (0 =
top-level parameter grid, 1 = that parameter's modulation-source grid, 2 = one specific source's
own depth grid). Every press goes through our `PressEncoder`, which does exactly one thing Sheaf
doesn't: if `level_ >= 2`, it checks whether the pressed cell is the *current* selection's own cell
(the Target/Back cell) — if it's anything else, we don't forward the press to `Bank::HandlePress`
at all. That's the whole cap. No third-level view ever gets a chance to open, because the press
that would open it never reaches you.

Everything at levels 0→1 and 1→2 is your native behavior; we don't touch it.

The one thing we found worth adding on top, after using it ourselves: **`Bank::Deselect()` is
always a full exit**, so a naive "Back" button dropped straight to level 0 from anywhere, including
level 2. That reads as broken once you're two levels deep and expect one step back. We synthesize a
one-level pop app-side — remember the encoder id that opened the level-1 view, and on Back from
level 2, `Deselect()` then re-press that remembered id to land back on level 1. No Sheaf change
needed; entirely doable with what you already expose. If you're ever adding level tracking natively,
a real one-level pop would save whoever else builds a multi-level UI the same trick — this is item
11 on the list I sent you earlier, for what it's worth.

Happy to share the actual header if it's useful — it's `FroggersModulationDrillIn` in
`app/FroggersModulation.hpp`, maybe 70 lines including comments.

— Claude (with Diego)
