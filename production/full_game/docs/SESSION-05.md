# Sesión original: selección → preparación de combate

> Ejecutar con superpowers:executing-plans; revisión independiente final.

Goal: conservar una sola máquina RAM/D/A/control desde RESET hasta el handoff
original F8C, usando los propietarios nativos existentes y entradas externas.
Architecture: wf_session posee RAM, vídeo sombra, runtime, frontend y match;
la plataforma suministra ROM, bus, esperas, vídeo y eventos reales. El adaptador
compone las extensiones originales98BA/8EB6/285DA y controla las transiciones.
Tech Stack: C11, fuentes HW1 bloqueadas por SHA, Unicorn2.1.4, GCC13.2.0 SH-2.
Spec: ../SPEC.md, SELECTION-04.md y PLAN.md.

## Restricciones y decisiones

- Mantener el checkout/rama de integración existente y los originales intactos.
- Sin nuevas políticas de elección, créditos, semilla, IRQ ni callbacks exitosos
  ficticios. RAM inicial y eventos de plataforma son entradas explícitas.
- HW1 conserva la corrección de esperas transitorias que falta en R3; usar sus
  fuentes exactas, sin editar ninguna referencia. Los tests registran su origen.
- No sustituir todavía el renderer TEST03 por un transporte incompleto. Este
  tramo entrega núcleo nativo y pruebas de flujo; el disco04 sigue siendo el
  último disco visual aprobado. Integración visual completa continúa OPEN.
- El host controla el bus externo en las pruebas. Sus relojes/semillas son
  fixtures declaradas; no se promueven a política de arranque Saturn.

## Review Focus

1. No arrancar match antes de AC0 ni importar actores antes del F8C real.
2. Nunca resetear RAM/D/A/RNG entre selección y preparación de partida.
3. START acreditado durante attract cancela el cursor viejo por generación.
4. Dependencia fallida queda fatal: no repetir efectos parciales.
5. Mando mantenido no confirma compañero; Demolition TAG y Royal son distintos.

## Tareas

- [x] Tests RED de wf_session: inicio inválido y fallo de plataforma sin avance,
  conservación de RAM inicial y estados entre propietarios; compilación host.
- [x] Implementar src/wf_session.{h,c}, con init/step/prepare_match, extensiones
  originales y transición explícita FRONTEND→MATCH_READY→GAMEPLAY o FATAL.
- [x] Harness tests/session_probe.c con ROM externa y bus controlado: ejecutar
  RESET, moneda, START, modo, compañero TAG o Royal, AC0 y F8C. Registrar
  snapshots y contexto, sin escribir participantes/IDs/créditos desde el test.
- [x] Comparar helpers de crédito/START nativos con ROM ejecutada para créditos
  insuficientes, cuatro puertos y configuración dedicada; preservar RAM/D/A.
- [x] Compilar núcleo SH-2 con fuentes exactas y comprobar símbolos pendientes;
  suite host normal/ASan/UBSan, originales intactos y revisión independiente.
- [x] Guardar logs/snapshots/hash y procedimiento de publicación/recuperación. No
  etiquetar el núcleo como disco arrancable ni como beta completa.

Ledger: scope05 conserva el límite visual04 para no mostrar una selección
que no representa su máquina lógica. Coste: el nuevo flujo se valida primero
en host/SH-2 y debe conectarse después al renderer/temporización compartidos.

## Resultado del alcance05

Diez recorridos pasan tanto en host normal como en ASan/UBSan. Los408 casos
de crédito/START coinciden con ROM en RAM persistente, registros y carry.
El núcleo SH-2 enlazado deja sólo memcpy/memset para libc; la reconstrucción
elimina miembros obsoletos. La revisión independiente detectó dos P2 del
harness/build, corregidos y comprobados; ver evidence/session05/review-resolution.md.
El manifiesto y la configuración del paquete final identifican el commit publicado.
No demuestra renderer, IRQ físicos, rendimiento ni equivalencia de la ruta
completa contra ROM. Esos requisitos permanecen abiertos en PLAN.md.
