# Relevo RUNTIME-18 — 23 octubre 2026

## Leer primero: estado real

Objetivo pendiente: completar un combate con entrada real de mando en Mednafen y entregar vídeo. No entregar BIN/CUE ni volver al frontend/audio. No forzar stamina, posiciones, estados ni resultados. No declarar ROM-EXACT global o aptitud Saturn físico sin sus pruebas.

Base Git: **805be2a6e1f25c2c98471a5bf3747011376f69bf**, rama `runtime18/direct-combat`. Los avances siguientes eran cambios locales, todavía sin commit. **Clonar únicamente805be2a pierde todo este trabajo: aplicar el parche y el overlay del paquete.** Los antiguos CURRENT-STATE-LATEST y HANDOFF describen la base, no este relevo.

El relevo se solicitó antes de terminar la partida activa. No hay vídeo final nuevo ni compilación nativa final de todas las fuentes. La compilación nativa de diagnóstico que sí se probó está congelada aparte y es anterior a CPU B7 y al último ajuste de copia de tiles.

## Punto exacto para continuar

1. **CPU `$1D078`, estado1/AE5, frame3050 slot4.** Snapshot inmutable `src/runtime/tests/r18/cpu_1d078_snapshot_active.bin`, SHA256 `5638056746d6dcf6ad8746dd73abdcd31b518a5595efec3d0bc17c5f355c9761`. Manifest y oracle están al lado. La entrada de la tabla `$1C908[5]` apunta a `$1D078`.
2. Existe un intento rechazado en `evidence/handoff-investigation/cpu_1d078_attempt.patch` y su fuente completa. El código de producción del relevo está restaurado a la versión CPU B7 validada, SHA256 `78091edfd2e6c06ce24cc5001f91861b02f612e77e36c8c43734c9f56564014d`. No presentar el intento como implementación correcta.
3. Fallo concreto del intento: fixture `init-b5-9-0-False`, byte RAM `$1C0A0D`: C=`E0`, ROM=`60`. La ROM sola pasa4161 casos/21 sitios, lo cual **no** valida el C. Reaplicar el intento solamente para corregirlo y obtener GREEN RAM/FG/D0-D7/RNG. El diagnóstico apunta al cálculo del ángulo/anchuras, pero es una hipótesis; rastrear las instrucciones originales.
4. Cerrar también la continuación conectada AE6 con evidencia directa de ROM, cuando se alcance. No detenerse deliberadamente en cada subestado si se puede resolver la cadena original completa. Mantener los demás guards hasta contar con evidencia.
5. Repetir la ruta activa descrita abajo. Cada bloqueo natural nuevo debe conservar su snapshot y resolverse con ROM intacta. Continuar hasta el resultado original `$11B6`, después presentación `$11CE..$122B`.
6. Ruta alternativa histórica: RIGHT60..179/A180..183 y luegoFF llega7691/slot1/raw04. **Raw04 todavía no está implementada.** Snapshot `evidence/natural-next/raw04-frame7691-slot1.bin`. Su investigación se interrumpió antes de escribir código. Esta ruta no debe declararse completa.

No añadir CPU1D078 a la colección de oráculos GREEN hasta resolverlo. Actualmente tiene un test RED de investigación; la colección validada no lo incluye. Añadir su target después de GREEN.

## Cambios ya integrados y comprobados contra ROM

ROM68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.

| Owner | Casos que pasan |
|---|---:|
| raw1C /1567C |3305|
| raw52 /18D2E y helper2103E |11606|
| raw58 /191C8 |2987|
| raw6D /19D06 |5223|
| raw09 /135EC, dos snapshots reales |2485|
| raw5D, alias ROM de5F |711|
| raw67 /1978A |2584|
| raw75 /1A0AC y helper10FC6 |2427|
| raw34 /174AA, descriptor9frames |2671|
| state4 /1BE7C |9178|
| CPU1D6E4 y conexión1F45A |2229|
| raw78 /1A192, dos snapshots reales |4685|
| CPU estado0 FLAGS34.bit4 |4404|
| raw79 /1A28E y descriptores de12 personajes |7611|
| raw21 /15B34 |777|
| raw05 /12F40 |4578|
| raw50 /18B94 |2991|
| raw2E /16A7A, revisión ampliada |3874|
| CPU estado0 B7.bit7,1DF1E/1DF7E |9695|

Pruebas nuevas en `src/runtime/tests/r18/`; snapshots y manifests inmutables. Se comparan las regiones que cada script declara, normalmente RAM1C0000..1C1FFF, FG0C0000..0C3FFF, D0-D7 y RNG. No se comparan universalmente A0-A7/CCR ni framebuffer completo. Las secuencias aisladas de callbacks no equivalen a frames completos de Saturn. Los snapshots HOST contienen campos representados del modelo C y ceros en el resto: no son volcados de la placa arcade.

CPU1D6E4 reveló un error anterior: reciprocal_link eliminaba el byte alto del puntero target. Ahora conserva el LONG original, después de validar el actor. Los nuevos predicados FLAGS34/B7 sólo enrutan las ramas demostradas, sin eliminar el guard agregado restante.

Revisión2E detectó un fallo del test: comparar estadoWORD con5 cortaba8005 al primer tick. Se corrigió a BYTE: ahora34ticks naturales y980 derivados pasan. No era un fallo de producción.

Cuatro oráculos CPU históricos congelaban CPU actual junto a state5_exec antiguo de f88acad, causando un undefined symbol para1F45A. Se corrigió únicamente la congelación de su dependencia/header actual, registrandoSHA; fixtures y ROM intactos. La colección histórica completa terminó exit0.

## Recorridos y prueba real de entrada

Scripts HOST:
`src/runtime/tests/r18/allies_run_probe.py` congela las fuentes y deja source-manifest.json. Nunca reutilizar un directorio de salida con trace.csv; usar uno nuevo.

Ruta **ring-active-pad**: RNG inicial0; port0 FF inicialmente; RIGHT60..179; A180..183; DOWN1000..1049; A durante4frames cada60 desde1200 hasta19980, despuésFF. CSV exacto en `evidence/input/ring-active-pad.csv`. La última observación válida alcanza3050/slot4 y bloquea1D078. Es la ruta prioritaria para obtener combate con entrada activa.

Ejemplo desde raíz del checkout, después de preparar los assets:

```sh
WF_PROBE_PAD_SCHEDULE=/ruta/absoluta/ring-active-pad.csv \
WF_PROBE_ROM_DEMAND=/ruta/absoluta/proxima-ROM.txt \
python3 src/runtime/tests/r18/allies_run_probe.py /ruta/absoluta/proxima-evidencia \
  --frames 100000 --all-actors --trace-tiles --require-complete
```

`WF_PROBE_PAD_SCHEDULE` aplica máscaras active-low, inicialFF, mantiene cada valor hasta el próximo evento. `WF_PROBE_ROM_DEMAND` registra páginas1KiB realmente demandadas, incluye boot/resultado y excluye preload; no confundirlo con el antiguo ROM_TRACE que suma calentamientos históricos. La herramienta de demandas/input fue verificada por equivalencia de stdout, trazas y snapshots.

Ruta **late-pad**, CSV adjunto: alcanza el final original en HOST, frame14409/result8001, sin bloqueo de gameplay. Con los hints canónicos viejos falla residencia (ROM4/Sprites13030). No es prueba de partida terminada en Mednafen. Humano queda mucho tiempo inactivo; no usarla como única demostración de combate controlado.

La regresión sin entrada de805 terminaba3008 con RING OUT/GAME OVER; conservarla. La nueva fuente final debe volver a pasar normal y ASan, no atribuirle automáticamente la ejecución nativa antigua.

**Bug real del mando corregido:** Yaul `pressed=current`, `held=current&(current^previous)`; el código usaba held, por lo que las pulsaciones mantenidas sólo llegaban un frame. controls() ahora usa pressed para los8botones. PruebaHOST extraída del cuerpo real:1244 muestras GREEN; evidencia de offsets/instrucciones del SDK en `evidence/audit/smpc/`.

Preflight real en Mednafen, fuente congelada posterior a50/2E pero anterior aB7:120frames seguidos RIGHT,50 DOWN, ataques posteriores; movimiento humano611,336→730,307→785,357; bloqueo3049/CPU1C2CC. `fixed-level-live-pad.json` conserva eventos y muestras. Hubo variaciones observadas de1frame (primerA181, otroA2941), sin impedir llegar al mismo bloqueo. No llamarlo replay nominal idéntico por frame.

Método: tecladoX→mando emulado Mednafen→SMPC Yaul. El driver sólo observa memoria; no escribe RAM. Es control automatizado, no una persona usando un mando físico. Semilla0 es fixture de validación, sin cambiar stamina/resultado. `live_capture_entry.c` no compila replay de entradas.

**Pendiente de control transferido:** el adaptador llena input_hw_low[0]. Si TAG/refill transfiere el rol humano a otro slot con fuente original140020 (+8A), esa entrada no está demultiplexada/verificada. Investigar la conexión segúnROM antes de declarar control de TAG completo. No hay arreglo nuevo de esto en el relevo.

## Recursos y memoria: implementación lista, consolidación pendiente

Los assets/hints canónicos siguen siendo los de805: R18H4883tiles y206páginas históricas. **No están actualizados para el combate nuevo.** Los nuevos formatos y tablas de demandas están preparados; generar los recursos finales tras completar la ruta.

Unión natural actual: sinentrada3008 + principal7691 + late14409 + activa3050 = **8052 IDs**, ROM**210páginas**. Listas estables en `evidence/audit/dictionary/union-through-main2e.txt` y `rom-demand-union-through3047.txt` (ésta última sigue siendo210 con todas las trazas observadas).

Raw128B/tile no cabía. R18D comparte bloques exactos4/16/32B, sin pérdida. Mapping BE16 por tile, primeros112KiB en HWRAM; resto al principio LWRAM; diccionario después. Un scratch128B se copia inmediatamente al stagingHWRAM antes de leer el tile siguiente. Render VDP1 y políticas de transferencia permanecen.

8052:63789chunks4B; mapa515328B (114688HWRAM +400640spill); dict255156B; residente655872B;74slots fríos. Archivo786868B, SHA y exactitud en spill-8052-verification.json.63casos de límites normal+ASan/UBSan, CDFS una ventana y SDKobjetos GREEN; revisión independiente también GREEN.

Partición contigua:
- escena00201000, capacidad1C100=114944B; máximo realR18RING114918, ringside95978;
- ROM0021D100,34800=210KiB;
- staging00251900,C000;
- tiles0025D900,A2700=665344B hasta00300000;
- tabla de índicesHWRAM112KiB, pagada liberando7ventanasCDFS; BSS conjunto sólo+72B vsbase.

Límites duros actuales:8180IDs por header16KiB,65536chunksBE16 y al menos1slot frío. **8052 está cerca: medir la demanda del combate completo; no truncar listas ni eliminar guards para hacerlas caber.** Si crecen, resolver la capacidad con evidencia real antes de declarar residencia o fluidez. No introducir soluciones aproximadas.

## SH-2 / Mednafen: qué está probado

SDK/GCC13.2.0, imagen fijada:
`ijacquez/yaul@sha256:b6d272204767cd0a734139dd3a2ab298ef5912fb7b7224b78e6d9fac1a7825a6`.

Último preflight aislado: A.BIN SHA256 `3e90357fdc5b0ce1cfe7d5137c9d5bba8068987c347502fc3e25fc1774499831`; ___end060FE634 (6604B hasta06100000). Disco interno pasó estructura/EDC/ECC/CDFS y unresolved0. **No es el ejecutable de todas las fuentes actuales.** Hubo avisoLOAD/RWX normal del SDK.

Observer sólo lectura,4096bytes de instrucciones exactas: escena,210páginasROM,7945tiles reconstruidos originales y mappingHWRAM/LWRAM coinciden en la misma imagen de memoria real emulada. Ver fixed-level-memory.json. Esto no demuestra Saturn físico ni60fps.

Linux local disponible: Mednafen1.32.1, Xvfb:99, contenedorwf-runtime18-emu; BIOS enperfil local. La nube no hereda el contenedor ni la BIOS. Mednafen debe usar -nothrottle0 en vídeo final, sin acelerar/interpolar. La grabación puede recortar BIOS y final estático; conservar combate/resultado íntegros.

Build final (sólo cuando esté estable):
```sh
R18_CAPTURE_RESULT=live sh validation/build-runtime18.sh /ruta/sh2eb-elf
```
El build usa imágenes internas BIN/CUE para arrancar; **no entregarlas al usuario**. Entregar MP4 de la ventana real. Driver `validation/drive-mednafen-runtime18.py`; capturador `validation/capture-mednafen-window.py`; observers de memoria/actores/gráficos existentes.

Verificar resultado original y finished_presentation=1/fatal=0, entrada sostenida/movimiento, VDP1comandos/texturas/VDP2HUD/mapas/cámara del binario exacto. Luego fullchecks normales/ASan, informe, hashes y checkpoint consolidado. No sustituirlo por otro demo que sólo avance hasta un bloqueo.

## Trasladar a la nube

El paquete contiene bundle Git con la historia disponible hasta805 y su frontera shallow, parche de cambios validados, overlay de archivos nuevos, evidencia seleccionada y el intento1D078 separado. No contiene ROM, BIOS, SDK, ejecutables ni imágenes de disco. Los archivos.bin dentro del overlay/evidencia son snapshots de RAM para los oráculos, no discos ni ejecutables.

```sh
python3 restore_checkout.py /ruta/nueva/wrestlefest
cd /ruta/nueva/wrestlefest
```
Conservar `evidence/` al lado o copiarlo al directorio de resultados elegido. Verificar manifestSHA256 del paquete. Los oráculos históricos necesitan la historia Git incluida; no copiar únicamente src/ a un repositorio vacío. El repo original es shallow: una clonación directa del bundle pierde esa frontera y falla al recorrer un padre no disponible. restore_checkout.py conserva shallow-boundary.txt antes de importar y verificar el árbol. No se inventa historia ni se necesita red para restaurar.

Aportar desde los archivos autorizados del proyecto: wwfwfest.zip; SDKsh2eb-elf-yaul si la nube puede compilar Saturn; BIOS Saturn válida paraMednafen cuando el entorno permita ejecutarlo. Python usado:machine68k0.4.1 ycapstone; instalar versiones compatibles con el sistema de la nube, no copiar extensionesMacOS.

Para regenerar recursos desde el ZIP autorizado: ejecutar `python3 prepare_assets.py /ruta/wrestlefest /ruta/wwfwfest.zip` desde el directorio del paquete. El helper usa Rom.p para el programa, pack4(sprite_tile) paraSPRITES, build_r18_graphics paraR18RING/escena2 y build_r18_font paraFONT8. No usa gráficos nuevos. HOTSPR se regenera inicialmente con los hints canónicos antiguos; actualizar su lista sólo después del recorrido completo. ConfirmarSHA1ROM ySHA256SPRITES130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31. No modificar sources/ sincronizado.

Al reanudar: primero leer este documento y STATUS.json del intento. El test1D078 debe mostrarRED sobre la producción restaurada (owner ausente). Reaplicar sólo su patch de investigación y corregir la diferencia documentada. Mantener los tests yaGREEN y continuar la ruta activa hasta el vídeo final.
