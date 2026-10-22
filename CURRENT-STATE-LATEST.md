# RUNTIME-18 — combate sin entrada, 23 de octubre de 2026

Continuación de `3435a09`, sin frontend ni código provisional. El bloqueo
`$19274` de la acción `$59` está resuelto: se conserva la diferencia entre un
link no usado, null literal y dirección física cero, conforme a la ROM.
También se han cerrado las dependencias que la secuencia abre después.

La partida **sin ninguna entrada**, semilla cero, alcanza `$11B6` en el frame
**4.391** en HOST. El árbitro cuenta uno, dos y tres en los frames 3.989,
4.077 y 4.165. Resultados: `8001,8000,4000,4000`; el reloj conserva `0021`.
El jugador pierde por inmovilización, sin agotar el tiempo. No se fuerzan
actores, stamina, elecciones CPU, cuenta ni resultado. Las dos partidas
completas pasan ASan/UBSan; las 48 regresiones de núcleo también.

El disco de validación SH-2 sin entrada también termina en Mednafen, con
frame 4.391, cuenta 0300, resultado 8001 y reloj 0021. La
validación de consola física con CD-R sigue pendiente. Se entrega por separado
el disco normal con mando y el replay con semilla cero/entradas desactivadas.
Al terminar se conserva el ring; el replay añade un informe de color.

La precarga mantiene 2.944 tiles originales y 128 huecos adicionales en
384 KiB. La partida controlada de 14.409 frames conserva su prueba de cero
lecturas de sprites tras la precarga; la nueva partida sin entrada necesita
11.984 lecturas de 983 tiles adicionales. Esa diferencia limita la fluidez y
no queda presentada como precarga universal ni velocidad arcade.

Evidencia y reproducción:
[validation/RUNTIME18-CPU-CHECKPOINT.md](validation/RUNTIME18-CPU-CHECKPOINT.md),
[validation/RUNTIME18-DISC.md](validation/RUNTIME18-DISC.md).
**Sin declaración ROM-EXACT, igualdad de framebuffer ni aptitud acreditada
para hardware.** El texto TIME UP aún no se publica en VDP.

## Registro histórico de RUNTIME-17

El texto siguiente describe exclusivamente el cierre RUNTIME-17.

# RUNTIME-17 — checkpoint verificado R02, 18 de octubre de 2026

Cerrados los pendientes de este checkpoint: owners nativos $2017A, $6E34 y $9132, integración $18C4, composición de registros de un frame activo y compilación/enlace SH-2. No se ha iniciado RUNTIME-18. Este cierre no declara equivalencia ROM-EXACT universal ni aptitud para hardware.

## Procedencia y cambios

Base GitHub exacta b85df2d23e4c64ecf24b5d2d41c4dc7b9000189e; recuperación anterior a35ffa429037986f55f09254106e0400c04427fe. Rama local recovery/runtime17-participants-20261018. El commit final consta en CHECKPOINT.txt del paquete. No hay cambios publicados en GitHub. Se conserva la historia recuperada con su límite shallow.

La ROM autoridad mantiene SHA1 a726c46f350723ea73158bc78055d17d418b14e2. TAG $6E34 y Royal $9132 se implementan en C nativo; los 22 helpers compartidos tienen generación reproducible y auditoría de fuente contra la ROM. No se enlaza un emulador con el runtime. Los emuladores MC68000 se usan exclusivamente como oráculos HOST independientes.

$18C4 usa los owners nativos por defecto, lee entradas actuales y detiene el recorrido después del primer pago. Las operaciones de vídeo se preparan antes del commit; un fallo conserva el estado de gameplay. Los callbacks de transporte no sustituyen decisiones de gameplay. Se corrigieron omisiones de anchura y preservación de registros en input, cámara, iteradores, árbitro, raw $7C, $F560 y $7A00. El ACK sincroniza los alias del contador de frames y el selector A/B.

## Evidencia y alcance

- 48 tests HOST con Clang, 48 con ASan/UBSan y 48 con GCC Linux; análisis GCC -fanalyzer de las fuentes.
- TAG: 28 casos contra ROM original, RAM/actores, D0–D7, FG0/paletas y orden de transferencias por canal.
- Royal: 45 casos contra ROM original en Musashi, los 22 helpers alcanzados, RAM/FG0, D0–D7/A0–A6 y orden de escrituras.
- Frame activo: ejecución real de $F9A a $10E8; D0–D7 y RNG coinciden en los 29 puntos de salida y tras ACK. Actor visible con acción $7C, movimiento $0200→$01E0, árbitro, objetos fijos y TAG nativo. Se verifica el selector de la siguiente rama B, no un segundo frame completo.
- Contrato de registros del render $D1FC: 144 comparaciones ROM para records simples/compactados sin enlaces y 18 rechazos de casos fuera de ese contrato. El adaptador del test lee records reales; no reemplaza registros por tuples esperados. No es un renderizador Saturn completo.
- Cuatro regresiones ROM adicionales verifican midpoint del árbitro y pago/refill, incluidos éxito y fallo.
- 14 auditorías estáticas de ROM y reproducción byte a byte de la fuente generada.
- Enlace limpio SH-2 GCC 13.2.0 con cero símbolos sin resolver. El SDK conserva avisos de segmentos RWX. El ELF es un harness de enlace, no un combate arrancable.

Los resultados completos y hashes están en evidence/ y SHA256SUMS del paquete. validation/README.md permite repetir las pruebas con la ROM y el SDK originales.

## Lo que no acredita este cierre

No se han probado igualdad de framebuffer, todos los records de sprites, todas las acciones/ramas de gameplay, timing, ni un combate completo en Saturn. Los cambios de escena y otras dependencias externas conservan su bloqueo explícito cuando falta una implementación; no se sustituyen por éxito provisional. Los dobles de callbacks que quedan en tests históricos solo prueban contratos aislados.

No hay BIN/CUE ni disco jugable. Pruebas de emulador Saturn y Saturn real con CD-R: NO EJECUTADAS. No usar el harness como imagen para grabar. RUNTIME-18 permanece sin comenzar hasta una nueva instrucción.

La documentación histórica describe estados anteriores. R02 documenta el cierre RUNTIME-17; para el estado actual prevalece la cabecera RUNTIME-18 de este archivo.
