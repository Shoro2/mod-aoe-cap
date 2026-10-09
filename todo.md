# Todo

No open work known (last change 2026-09-24). The module ships disabled by decision (vault
`forgotten-land/10-mass-pull-performance-plan.md` §3, decision 6).

- (low) (suggestion) Before the cap is ever enabled: a mod-fl-testbots scenario that pulls more creatures
  than `AoECap.MaxTargets` and counts how many take damage; the enabled path has never run.
- (low) (suggestion) A nearest-N cap needs a core edit at the two `RandomResize` sites in `Spell.cpp`
  (README, limitation 1); only if the operator ever wants a low cap.
