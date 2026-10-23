# RUNTIME-18 — cámara, ring y sprites sincronizados, 23 de octubre de 2026

Continuación de `426ed17`. Las fases de cámara publican ahora los cuatro
scrolls originales que también consumen el ring y el streaming. Se elimina
la divergencia entre una cámara de sprites móvil y un ring inmóvil. Los límites
unsigned y la escritura conjunta de ambos canales siguen `$2983C` de la ROM.

El nuevo oráculo compara **2.304 casos** directamente con la ROM intacta,
incluidos D0–D7. Las partidas completas conservan sus finales4391 sin entrada
y14409 con entrada programada; se comprueba la coherencia de cámara en cada
presentación. Pasan las48 regresiones HOST y sus48 versiones ASan/UBSan.

El nuevo replay SH-2 completa el combate en **Mednafen** en frame4391,
cuenta0300, resultado8001 y reloj0021, sin bloqueos. También se inspecciona
el disco normal en movimiento: ring, postes y luchadores comparten el
scroll actualizado. Se entrega BIN/CUE normal con mando y replay separado.
Arranque directo, sin menús ni audio; el sonido ya probado en Saturn pertenece
a sus componentes anteriores, fuera de este ejecutable.

La precarga mantiene2944 tiles más128 huecos en384KiB. La partida controlada
conserva cero lecturas frías; la secuencia sin entrada mantiene11984 lecturas
de983 tiles adicionales. No se presenta esta corrección como velocidad arcade.

Evidencia, hashes y reproducción:
[validation/RUNTIME18-CAMERA-CHECKPOINT.md](validation/RUNTIME18-CAMERA-CHECKPOINT.md),
[validation/RUNTIME18-DISC.md](validation/RUNTIME18-DISC.md).
**Sin declaración ROM-EXACT ni prueba de este nuevo disco en Saturn físico.**
El siguiente ensayo es en emulador del usuario y Saturn con CD-R.

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
