# FINAL — READ ME FIRST

Este paquete es la entrega acumulativa de **CLEAN-R01 / 04-COLLISION-HITBOX-ENGINE-FULL**.

## Qué queda cerrado

Se ha reconstruido y auditado desde la ROM arcade:

- lifecycle de `+$4C/+4D` y tabla de 34 collision profiles `$24EF6`;
- hitboxes `$24FE4` y hurtboxes `$25010`;
- selección source/target, pair mask, coarse gates y overlap exacto;
- FIRST/SECOND completos con independencia source/target y carry;
- daño pendiente, links, clears y generic collision tail;
- precedencia procedural por scheduler/slots;
- active-window mechanics e invulnerabilidad profile-local sin timeouts inventados;
- relación strike/paired choreography sin inventar una segunda geometría;
- boundary referee/presenter/partners;
- resolver de mundo `$280DC..$285D8`, collision modes y productores exactos de `+$37.bit4`.

## Autoridades preservadas

- `victim65 ROM-EXACT FINAL`: autoridad de `+$64/+$65`, FIRST/SECOND retypes, normalización y critical-tier timing. Este paquete la amplía, no la contradice.
- `Animation ROM-EXACT FINAL`: timing/state callbacks, sin reetiquetar campos por intuición.
- `Referee/Announcer ROM-EXACT FINAL`: referee fuera de los nueve wrestler slots; comparte world resolver `$280DC`, no pair engine `$24062`.
- `WIP160` sólo como referencia histórica y tests/anchors ROM; no se importó runtime legacy.

## Main ROM

```text
SHA1 a726c46f350723ea73158bc78055d17d418b14e2
```

## Leer después

1. `WF-Saturn-04-COLLISION-HITBOX-ENGINE-FULL-ROM-EXACT-FINAL.md`
2. `ASSEMBLER-HANDOFF.md`
3. `STATUS.json`
4. `AUDIT-LOG.md`
5. `tables/` y `tools/`

No contiene ROM completa, runtime Saturn modificado ni BIN/CUE.
