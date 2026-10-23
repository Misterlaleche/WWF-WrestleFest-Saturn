# Estado actual: primer combate completo desde f88acad, 23 de octubre de 2026

Raw3F y raw5F implementados con ROM directa, junto a los owners necesarios
para continuar la ruta natural: CPU, recepciones, ringside, árbitro y cuenta.
El combate sin entrada termina en HOST y Mednafen en el frame3008, por cuenta
fuera de20, seguido de la presentación original RING OUT / GAME OVER.

Precarga consolidada:4883 tiles y206 páginas. La partida natural registra
cero lecturas de ROM/sprites en gameplay, normal y con ASan/UBSan. La carga
CD de escena2 es independiente. Suite general:36 ejecutables, exit0.
SH-2 enlaza; Mednafen comprueba comandos/texturas VDP1, HUD y escena2 VDP2.
La entrega contiene vídeo65,3s, capturas y prueba reproducible; sin BIN/CUE.

Límite relevante: la ruta con mando llega3373 y aún bloquea en raw1C.
No se declara todo gameplay cerrado, ROM-EXACT universal ni Saturn físico.
Promedio observado:3008 frames/3464 VBlanks, ≈52,1fps a60Hz nominales,
incluyendo cambio de escena y esperas; no60fps sostenidos. Sin frontend/audio.

Informe y reproducción:
[validation/RUNTIME18-ACTIONS3F5F-COMBAT-CHECKPOINT.md](validation/RUNTIME18-ACTIONS3F5F-COMBAT-CHECKPOINT.md).

## Checkpoint anterior: movimiento CPU y llave $22

Sobre `a4be15f`: implementados `$1D7B6/$1DA66` y acción`$22`, con2263 y1946
comparaciones ROM respectivamente. Se consolida la precarga4513 tiles y176
páginas; cero lecturas CD en los recorridos HOST probados703/560 frames.
Mednafen verifica movimiento del aliado en700 y llave`4022` en550, comandos
VDP1 y HUD VDP2 reales. Se entrega vídeo y capturas, sin otro paquete BIN/CUE.

La partida sigue incompleta: los nuevos primeros bloqueos son acción`$3F`
en703 sin entrada y`$5F` en560 con control. Perfil450→550:100 frames/111
VBlanks, aproximadamente54fps en ese tramo; no60fps sostenidos, ROM-EXACT
universal ni validación Saturn físico. No se retoma el frontend.

Informe y reproducción:
[validation/RUNTIME18-MOVEMENT-ACTION22-CHECKPOINT.md](validation/RUNTIME18-MOVEMENT-ACTION22-CHECKPOINT.md).

## Investigación anterior: HUD, aliados y stamina

Sobre `95328a2`: corregido el origen de relleno de las barras, preservando
ambos retratos; comprobado en capturas reales de Mednafen. Se corrigen
también mantenimiento de daño/tier, distancias CPU que no se refrescaban,
registros de seguimiento y poses retenidas. La distancia falsa cero podía
provocar puñetazos lejanos; el caso ROM654 ahora pasa a movimiento.

La partida corregida aún está incompleta: sin entrada bloquea en669
`$1D7B6`, y la secuencia controlada en460 `$12614`. Los finales4391/14409
del checkpoint anterior no se atribuyen al nuevo árbol. No se ocultan esos
bloqueos. Entrega de esta investigación: capturas, sin otro paquete BIN/CUE.

Informe y reproducción:
[validation/RUNTIME18-HUD-ALLIES-INVESTIGATION.md](validation/RUNTIME18-HUD-ALLIES-INVESTIGATION.md).

## Checkpoint anterior: sprites nativos y escrituras del ring

Continúa desde `64d93b1`: caché de 32 cuerpos, comandos VDP1 sin clears
redundantes, recorte de filas transparentes y máscaras rápidas para SH-2.
El frame 120 conserva sus píxeles y reduce 1840 → 1474 filas VDP1.
CPU −6,9 %; la cadencia inicial solo mejora 99 → 98 VBlanks/90 frames.
El tramo avanzado conserva 198/180. Partida completa: 4887 VBlanks frente
a 4919, con 4391 frames. No son 60 FPS sostenidos.

Se conecta el parche original `$298EC` a VDP2 con 96 casos ROM. La apertura
de cuerdas todavía no se ha activado jugando en Mednafen; la comparación
se identifica como software. Se libera HWRAM manteniendo 4096 escrituras
en una zona separada de LWRAM.

Informe, límites, reproducción y evidencia:
[validation/RUNTIME18-SPRITE-WORK-CHECKPOINT.md](validation/RUNTIME18-SPRITE-WORK-CHECKPOINT.md).
El checkpoint incluye BIN/CUE directo, sin frontend ni audio. Saturn físico
con CD-R queda pendiente. Persisten rutas de acciones bloqueadas; no se
sustituyeron por éxito provisional.

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
