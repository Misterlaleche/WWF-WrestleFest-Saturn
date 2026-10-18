# GAMEPLAY MASTER RUNTIME-17 — ESTADO MÁS RECIENTE DEL RELEVO

Fecha de snapshot: 2026-09-15.

## Clasificación

**RUNTIME-17 ADVANCED E2E STAGING / NOT RUNTIME-18 / NOT FINAL**

La implementación activa está en `payload/checkpoints/RUNTIME17-CONTINUATION-2-LIVE-WORKTREE-WIP.zip`.
Ese ZIP supersede, como estado de implementación, al checkpoint de rama A incluido como referencia.

## Hitos ya presentes en el árbol activo

- Cadena post-$7A00 físicamente integrada.
- Rama A HOST E2E PASS hasta `$298EC` → VBlank ACK → post-ACK no-Royal → siguiente `$FA0/$21B4`.
- Rama B HOST E2E PASS: 26 owners reales → `$BCFE/$C04A/$C710` → `$2836/$298EC` → VBlank ACK → post-ACK no-Royal → siguiente `$FA0/$21B4`.
- `$285DA/$26A42` implementados y con regresiones de shadow/rollback PASS.
- `$7548` implementado y regresiones PASS.
- `$BCFE` implementado y regresiones PASS.
- `$C04A` implementado y regresiones PASS.
- `$C710` implementado y regresiones PASS.
- ROM audit de familia `$C04A/$C710`: PASS.
- ROM audit scheduler/VBlank/post-ACK: PASS.

## Correcciones recientes que no deben perderse

- `$8710/$8856`: `$8856` sólo escanea 4 compact records `$1C14CE`, stride `$10C`, bit7; no OR de globals.
- `$26A42/$26B76`: direcciones 0=Y++, 1=X++, 2=Y--, 3=X--.
- Royal free fallback: 0→5.
- `$298EC`: dos `LSL.W`, clobbers D0/D1/D4 exactos.
- `$2836`: D0-D5 preservados; D6/D7 vivos.
- VBlank wait resumible/idempotente.
- `$FA0 -> $21B4` obligatorio por nueva iteración.

## Comandos/resultados HOST observados justo antes del snapshot

`build/test_runtime17_branch_b_pre`:
- PASS $285DA selection/order + D0/D1 restore + D2-D7 visibility
- PASS $285DA fail-closed rollback
- PASS $26A42 directions + D0-D2 restore + D3-D7 visibility
- PASS $26A42 fail-closed rollback
- PASS branch-B preowner bindings present

`build/test_runtime17_7548`: 5 PASS.

`build/test_runtime17_bcfe`: 6 PASS.

`build/test_runtime17_c04a`: 6 PASS.

`build/test_runtime17_c710`: 9 PASS.

`build/test_runtime17_branch_b_e2e`:
`PASS branch B real bindings: 26 owners -> VBlank ACK -> post-ACK -> next $FA0 RNG`

## Audits observados

`audit/branch-b-final-rom-audit.log`:
PASS para `$C04A/$C0F8/$C172/$C404/$C710/$C892/$C9BE/$CA10/$CA58/$CA88/$CAB2/$CAE2/$CB12/$CB42/$CB78`.

`audit/final-frame-rom-audit.latest.log`:
PASS para `$F9A/$FA0`, scheduler final, frame-ready, VBlank wait, post-ACK y tabla `$119A`.

## NO asumir como gate final

El último ELF Saturn completo encontrado (`audit/gameplay-runtime-r17-branch-a-e2e-link.elf`) fue generado antes de las últimas adiciones branch-B.
Debe repetirse full SH-2/full ELF en el árbol actual.

## Blockers para promoción RUNTIME-18

1. Reejecutar gates completos sobre árbol LIVE actual.
2. Full SH-2/full Saturn ELF actual + unresolved=0.
3. Royal post-ACK `$2017A` E2E explícito.
4. Consumir/cerrar FINAL A/B D0..D7 composition del auditor.
5. Triple review + SHA256SUMS + clean extraction re-audit.
6. No hardware PASS sin Saturn real.
