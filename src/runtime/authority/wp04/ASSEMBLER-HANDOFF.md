# CLEAN-R01 — ASSEMBLER HANDOFF — WORKPACK 04

## Integrar ahora

Implementar el subsistema con estas invariantes exactas:

1. `+$4C/+4D` como WORD con **high runtime flags + low profile ID**; no truncar a un byte.
2. Profile table `$24EF6`: 34×7 bytes exactos desde `tables/collision-profiles.csv`.
3. Source boxes `$24FE4` y target boxes `$25010`: 11×4 s8 cada una.
4. Source selector: 9 slots, profile WORD!=0, `+$20.bit7`, row bit7.
5. Target gates de `$24126`, incluido profile14 `+$0A-8`, `<$0C` en axis0A y `<$50` en axis06.
6. Pair mask `+$8E` por slot `$8000>>slot`.
7. Fine overlap exactamente como `$24264/$242DE`; **NO ordenar endpoints** tras mirror.
8. FIRST = source row byte5; carry set cancela SECOND/tail.
9. SECOND = target row byte6; tras SECOND el dispatcher fuerza carry clear.
10. Precedencia de writes: FIRST -> FIRST normalization -> SECOND write -> SECOND normalization -> generic tail.
11. Damage row byte4 se escribe con **BYTE** a `target+$69`; `+$68` se consume como WORD.
12. Generic tail `$24DEC/$24E02`: clears, pair bit, reciprocal `+$92`, target `+$C6++`, critical bit7 latch.
13. Scheduler: actor callbacks/world correction -> `$24090` maintenance -> `$24062` collision -> `$24E58` damage/tier.
14. World resolver `$280DC/$28124` y tabla de 14 pointers exacta; conservar `NEG.W +$3E` in-place.
15. `+$37.bit4` sólo de `$2818E/$28288/$2831C/$2851E`; `$28480` Z gate pone bit1.

## Fail-closed obligatorio

- No inventar un writer para profile `$02`; si la integración aún no tiene un productor demostrado, no activarlo por default.
- No convertir profile IDs en etiquetas `strike/grapple` salvo autoridad externa explícita.
- No inventar object/referee pair hurtboxes; referee no está en los nueve slots de `$24062`.
- No usar prioridad numérica; respetar slot scan + FIRST carry.
- No añadir timers/cooldowns para ventanas de hit: profile writers + `$24090` + pair mask gobiernan la ventana.
- Si un field runtime requerido por FIRST/SECOND todavía no está integrado (`+$33`, `+$52`, `+$26`, `+$2E`, etc.), bloquear esa rama en vez de aproximarla.

## Campos byte-perfect / width-sensitive

```text
+$4C/+4D  WORD profile runtime flags + ID
+$69      BYTE pending-damage low-byte write
+$68      WORD pending damage accumulator
+$64/+65  WORD/BYTE victim state field per victim65 FINAL
+$8E      WORD per-source pair mask
+$92      LONG reciprocal collision target/source link
+$74      WORD flags/state, some SECOND/tail paths
+$76      LONG linked pointer
+$36/+37  WORD rebuilt world-contact/result bytes
+$38/+3A/+3C WORD corrections
+$3E      WORD mutated in-place by resolver when +$2E.bit7
+$40/+42  WORD offsets added before resolver
```

## Dependencias FINAL

- victim65 FINAL: obligatorio para state4 subtype lifecycle/high-byte latch.
- Animation FINAL: callback/timing semantics; no cambiar HOLD/state timing desde este workpack.
- Referee/Announcer FINAL: special actor boundary.
- CPU FINAL: mantener callback→collision→damage order; collision fields no deben inferirse desde CPU heuristics.

## Tests mínimos antes de aceptar integración

1. `tools/audit_collision_rom.py` PASS contra `wwfwfest.zip`.
2. `tools/test_collision_model.py` PASS.
3. Test de mirror que pruebe endpoint order invertido sin min/max.
4. Test de FIRST carry set: SECOND y generic tail no corren.
5. Test de FIRST success + target SECOND selector independiente del source profile.
6. Test de successful pair: source/target profiles clear, reciprocal `+$92`, source pair bit set.
7. Test pending damage: row byte4 write a low byte y WORD apply posterior.
8. Test scheduler: tier/critical latch del tail observa tier pre-daño de esa misma frame.
9. Test `+$37.bit4`: thresholds `$140/$100`; `$28480` no produce bit4.
10. Test mode1 + `+$32.bit2=1` -> `$2818C` RTS.

No integrar ningún legacy collision approximation si contradice estas reglas.
