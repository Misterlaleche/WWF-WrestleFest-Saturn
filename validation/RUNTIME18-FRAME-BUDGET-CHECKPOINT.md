# RUNTIME-18 — construcción de comandos y presupuesto de frame

Continuación de5ab045c. Arranque directo con mando; sin frontend ni audio.
El siguiente cuello medido estaba en la construcción de la lista VDP1: búsqueda
lineal repetida de tiles y borrado de memoria que no llegaba a enviarse.

## Cambio probado

El planificador usa un índice hash512entradas para sus256tiles máximos.
Mantiene exactamente el primer uso, slot, orden, coordenadas, flags, paletas,
culling y límites anteriores. Sólo cambia el coste de localizar un tile.
Reinicia los contadores y el índice; inicializa íntegramente cada entrada usada.
VDP1 inicializa los dos comandos de control, cada sprite y END al emitirlos.
No borra los512comandos no enviados. Un aborto no envía la cola parcial.

No se modifica ninguna decisión de CPU, animación, física, cámara, reloj,
entrada, resultado, prioridad o frecuencia lógica. No hay frames descartados,
simulaciones aceleradas ni gráficos sustitutos. La instrumentación sólo se
compila en los discos diagnósticos; el mapa normal confirma su ausencia.
El ejecutable normal termina en060FD834:10188bytes hasta el límite HWRAM.

## Medidas en Mednafen1.32.1

| Secuencia | 5ab045c | Nuevo | Alcance |
|---|---:|---:|---|
|30–120,90frames, sin decoradores |131VBlanks |111VBlanks |15,3%menos VBlanks;1,233porframe |
|900–1080,180frames |sin medida comparable |203VBlanks |1,128porframe |
|30–120, perfil detallado |157VBlanks |114VBlanks |Mismos decoradores; diagnóstico únicamente |

No se alcanza60fps sostenidos: ambas medidas de producción superan un
VBlank por frame. Son medidas del Saturn emulado; el FPS del host sin throttle
no prueba velocidad del juego. No se extrapola el tramo inicial a todo el combate.

En el desglose detallado, presentación205371→155205ticks; inicio del backend
11909→344ticks; bytes borrados por el planificador947880→197280. Los tiempos
anidados se solapan y no deben sumarse. El parser no se ha sustituido.

## Fidelidad y verificación

-1540casos de equivalencia contra la fuente congelada/hash verificado5ab045c:
 todos65536IDs, colisiones, frames consecutivos, bordes de culling, duplicados,
256tiles y512comandos, incluida la conducta al exceder límites.
-786432píxeles RGB idénticos en la captura del frame120. Hashes idénticos de
 todos los comandos VDP1 activos y sus texturas, y del mapa/fuente HUD VDP2.
 Se verifica la presencia de los payloads dentro de la VRAM emulada real.
-CORE48/48 y48/48 conASan/UBSan; preparación de comandos, DMA, HUD y transporte.
 El test de trabajo redundante fallaba antes; tras el cambio prepara una lista
 de cuatro comandos borrando128bytes en el doble HOST, dentro de su prefijo.
-D1FC365casos, cámara2304, display800 yHUD256 contra instrucciones originales.
 Pipeline ROM independiente R17 completo. No se declara equivalencia universal
 de todos los framebuffers arcade a partir de estas muestras.
-Partidas completas HOST yASan:4391 sinentrada y14409 con la entrada controlada;
 cero lecturas CD de programa/sprites durante ambas partidas.
-Revisión fresca sin fallos Critical/Important. Prueba física/CD-R pendiente.

El disco normal definitivo completa4391frames enMednafen: resultado8001,
cuenta0300, reloj0021, sin owner/dependencia fatal. Conserva1024escriturasCRAM
y9.870.080bytes de texturas del checkpoint anterior; la mejora procede del
trabajo deCPU al preparar comandos, no de reducir los gráficos enviados.

## Reproducir

```sh
python3 validation/run-host.py src/runtime --out validation/results/runtime18/core-host
python3 validation/run-host.py src/runtime --sanitize --out validation/results/runtime18/core-sanitizers
python3 src/runtime/tests/r18/verify_plan_equivalence.py
make -C src/runtime test-runtime18-command-work test-runtime18-yaul-dma test-runtime18-display-asan
make -C src/runtime test-runtime18-noinput-complete-match test-runtime18-complete-match test-runtime18-noinput-complete-match-asan test-runtime18-complete-match-asan
DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
R18_PROFILE_TEST=1 R18_PROFILE_START=900 R18_PROFILE_STOP=1080 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

`R18_PROFILE_DETAIL=1` junto a`R18_PROFILE_TEST=1` activa el desglose.
El disco normal no incluye el profiler. El test de equivalencia funciona desde
el ZIP sinGit: incorpora una referencia exclusivamente de test, congelada y
verificada porSHA256; no se enlaza en el juego.

Usar `observe-mednafen-runtime18-graphics.py` conPID,A.BIN,mapa exactos y el
layoutSH2 para verificar datos reales. `check-runtime18-profile.py` admite
`--start 900 --stop 1080`, y un presupuesto
explícito deVBlanks/frame. Los perfiles y sus ejecutables se conservan separados.

El paquete incluye fuente consolidada, cinco assets canónicos, ROM de referencia,
Gitbundle y evidencia. SDKYaul fijado, Docker yBIOS son dependencias externas.
Abrir/grabar elCUE con suBIN al lado: una pistaMODE1/2352. Se comprueban sectores
EDC/ECC,IP ypayloadsISO9660; A.BIN debe coincidir tras reconstruir el ZIP frío.
Los timestampsISO pueden variar. **Saturn físico/CD-R: pendiente del usuario.**

## Disco normal

A.BIN `30d9c249866a940597bd46ab4c26c0e649ba37ea022cca3f8e239f67f76281e4`.
BIN `9714fbff24df446dea6d53c700d90b1fc07d7ccd9a5533c8416b5fa84c425937`.
CUE `331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14`.
