# P1-01 — corrección del diagnóstico de progresión TAG

La conclusión inicial de que el owner `$11B6` terminaba TAG después del primer
combate era incorrecta. La ejecución que produjo ese diagnóstico no tenía los
34 recursos FIX2R restaurados; los scripts fallaban al intentar usar `cd/`.

Con los recursos verificados y la política física conservada, la misma base
FIX2R completa el primer combate y entra en el segundo:

- `match01-terminal`: owner `$11B6`, frame 11636.
- `match02-stage1-f8c`: stage 1.
- resumen: `entries=2`, `results=1`, `stages=003`, `failed=0`.

No se aplica ningún parche de gameplay para `$11B6`. La prueba de campaña queda
como regresión obligatoria para cambios posteriores.
