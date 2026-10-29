# Selección de fuentes y fixtures para el paquete reproducible

Documento de empaquetado por inspección de fuentes, índices ZIP y dependencias.
No se han cambiado implementaciones ni ejecutado pruebas para redactarlo.

## Recomendación

Conservar un único árbol `wp1979/` con las fuentes nativas finales de candidate4,
la referencia original, el contenido de los dos ZIP compactos y los extras
enumerados aquí. Los verificadores usan rutas relativas a su propio archivo;
preservar esa estructura permite reconstruir los binarios host sin depender de
los objetos o ejecutables de esta sesión.

La selección final de juego es revisión 4, cinta de 7372 pasos y atlas unión de
6474 tiles. Los fixtures anteriores se conservan únicamente para las regresiones
y los hashes históricos que comprueba `verify_final.py`.

Las listas de rutas de este documento son relativas a `wp1979/`, excepto la
lista expresamente marcada como relativa a `audit_gameplay/`. Una ruta que acaba
en `/` significa incluir sus archivos recursivamente. No hace falta incluir
otros directorios de trabajo por asociación de nombre.

## 1. Contenido ya incluido en los dos ZIP compactos

| ZIP | Extracción correcta dentro del paquete | Contenido |
|---|---|---|
| `audit_gameplay/handoff/wp1979-gameplay-evidence-v4.zip` | `audit_gameplay/` | 72 entradas, incluido `PACKAGE-INDEX.json`; 725720 bytes |
| `audit_gameplay/residency_takeover_v1/wp1979-residency-takeover-v1.zip` | `audit_gameplay/residency_takeover_v1/` | 24 entradas, incluido `PACKAGE-INDEX.json`; 611737 bytes |

El índice de cada ZIP contiene la lista exacta de archivos, tamaños y SHA-256.
Para entregar pruebas ejecutables directamente, incorporar sus contenidos ya
desempaquetados en los destinos indicados. Conservar además los ZIP originales
es opcional; no son sustitutos de la estructura que esperan los scripts.

El ZIP v4 ya contiene:

- CPU final `fixes/state6/src/wf_cpu_runtime.c`, humano final
  `fixes/src/wf_r17_human_raw23.c`, parches combinado e incremental y hashes.
- La CPU v3 de `fixes/src/wf_cpu_runtime.c`: conservarla porque es la base roja
  de las pruebas state6, no instalarla como CPU final.
- `host/build_tag_probe.py`; los builders, harnesses y adaptadores bajo `fixes/`
  y `fixes/state6/`; el ejecutor de ROM `rom68k_oracle.c` y su builder.
- `test_rom_fragments.py`, `test_human_producer.py`, `test_real_inputs.py`,
  `state6/test_state6.py`, `compare_causal_phases.py` y `state6/verify_final.py`.
- Los pequeños fixtures necesarios para los casos 6065 y 6093: RAM, registros,
  caché de registros observada y entrada state6 producida por la ROM original.
- Resultados rojo/verde, resultado de las 984 fases, logs, tres cintas de inputs
  que antes se bloqueaban, cinta final, timeline y traza completa del combate.
- Atlas v4 de 6357 tiles y cabecera ROM de 211 páginas.

El ZIP de residencia ya contiene:

- Atlas unión de 6474 tiles, IDs originales/adicionales/unión, manifiesto,
  dimensiones, logs y resultados de residencia.
- CSV lógico manual, correspondencia temporal, procedencia y los 35836 valores
  observados que se comparan contra la reproducción host.
- Trazas completas manual y principal, baseline manual y `verify_union.py`.

Todos los archivos referidos por los hashes del manifiesto de residencia están
en la combinación de ambos ZIP. Sus dependencias de ejecución —fuentes,
assets canónicos y binarios host reconstruibles— se enumeran a continuación.

## 2. Extras imprescindibles para reconstruir las pruebas host

### Referencia de código

El paquete nativo ya incorpora la referencia. Mantener estas rutas originales;
los builders host no deben sustituirlas por el código final modificado:

```text
reference_r18/src/runtime/Makefile
reference_r18/src/runtime/include/
reference_r18/src/runtime/src/
reference_r18/src/runtime/generated/
reference_r18/src/runtime/tools/build_r18_hot_sprites.py
```

La referencia corresponde al commit
`d95b8c37f62402bce4dead13cf7c0567b8fc2439`. El Makefile declara las 114 TUs
`CORE` + `R18_HOST`. El directorio `generated/` es obligatorio: contiene cinco
TUs y `wf_cpu_decision_data.h`. No basta con conservar `src/` e `include/`.

### Dos archivos host ausentes del ZIP v4

```text
audit_gameplay/host/tag_role_probe.c
audit_gameplay/host/wp1979_rom_hot_pages_both_tags.h
```

El primero completa `host/build_tag_probe.py`, que genera los objetos originales
reutilizados por los otros builders. La cabecera local es una copia de la
cabecera ya incluida en `audit_gameplay/assets/`; se puede materializar con una
copia después de extraer, pero es más sencillo conservar este archivo de 1577
bytes en su ubicación actual.

### Assets canónicos para los harnesses

```text
audit_gameplay/assets/ROM68K.BIN
audit_gameplay/assets/SPRITES.PAK
audit_gameplay/assets/R18RING.BIN
audit_gameplay/assets/R18MAP1.BIN
audit_gameplay/assets/R18MAP2.BIN
audit_gameplay/assets/R18MAP5.BIN
```

Los seis existen también entre los assets del staging nativo en
`native/src/runtime/audit/runtime18-cd/`. Se pueden copiar de allí conservando
estos nombres y rutas; suman 9366498 bytes. Los harnesses seleccionados no
necesitan FONT8, FRONTSPR, POSTSPR, menú ni audio para reconstruir sus pruebas.
Esos assets sí pertenecen al paquete nativo de producción según su propio build.

Además debe existir el fixture de ruta de escenas:

```text
audit_gameplay/host/audit/runtime18-cd -> ../../assets
```

Es un enlace relativo existente, no un enlace al scratch de esta sesión. Si el
formato de distribución no conserva enlaces, recrearlo después de extraer o
copiar R18RING/R18MAP1/R18MAP2/R18MAP5 en ese directorio. El harness abre las
escenas desde su cwd `audit_gameplay/host` mediante `audit/runtime18-cd/`.

### Fuentes del ejecutor de ROM independiente

El builder ya incluido espera exactamente estas fuentes bajo `tooling/`:

```text
tooling/yabause-master/yabause/src/musashi/m68k.h
tooling/yabause-master/yabause/src/musashi/m68kcpu.c
tooling/yabause-master/yabause/src/musashi/m68kcpu.h
tooling/yabause-master/yabause/src/musashi/m68kops.h
tooling/yabause-master/yabause/src/musashi/m68kops.c
tooling/yabause-master/yabause/src/musashi/m68kopac.c
tooling/yabause-master/yabause/src/musashi/m68kopdm.c
tooling/yabause-master/yabause/src/musashi/m68kopnz.c
tooling/yabause-master/yabause/src/musashi/m68kconf.h
tooling/yabause-master/yabause/src/musashi/readme.txt
```

Son nueve archivos de implementación más el README con la licencia: 941931
bytes. `build_oracle.py` copia las fuentes al directorio de trabajo y activa el
hook de instrucciones en esa copia. No necesita el árbol completo de Yabause,
sus binarios ni el directorio generado `fixes/musashi/`. Este ejecutor sólo
pertenece a las pruebas; no se enlaza al juego Saturn.

## 3. Trece fixtures históricos requeridos por el verificador existente

Se autoriza conservar estos 13 archivos para que `verify_final.py` funcione sin
modificaciones. Su recorrido de hashes incluye las selecciones v2 y v3
registradas dentro del manifiesto v4. No basta con las fuentes y la cinta final.

**Lista exacta relativa a `audit_gameplay/`:**

```text
host/tag-after3600-input.csv
assets/HOTSPR-both-tags-full-32.BIN
host/both-tags-events.csv
host/both-tags-full-resident.log
host/both-tags-full-resident-roles.csv
host/tag-long-input.csv
assets/HOTSPR-tag-full-4.BIN
host/early-tag-final-header.log
host/tag-corrected-after3600-input.csv
assets/HOTSPR-both-tags-corrected-32.BIN
host/both-tags-corrected-events.csv
host/both-tags-corrected-confirm.log
host/both-tags-corrected-confirm-roles.csv
```

Total exacto: **10216432 bytes sin comprimir, aproximadamente 9,74 MiB**.
Etiquetarlos en el inventario de entrega como «fixtures históricos de
verificación; no assets seleccionados». No cambiar sus contenidos ni hashes.

`assets/HOTSPR-both-tags-full-32.BIN` tiene además una dependencia directa:
`test_real_inputs.py` lo usa como atlas de sus tres regresiones. Contarlo una
sola vez. Los otros atlas históricos y las capturas de las exploraciones no son
necesarios para estos scripts.

## 4. Selección compacta de audit_rom

Ningún harness v4 o de la unión importa `audit_rom`: su comparación de
instrucciones originales utiliza ROM68K.BIN y Musashi. Para conservar también
la evidencia independiente de movimiento, temporización y referencia MAME,
recomiendo añadir este conjunto, ausente de los dos ZIP compactos:

```text
audit_rom/ROM-AUTHORITY-AND-ORACLE.md
audit_rom/AUDIT-DELIVERABLES-SHA256.txt
audit_rom/wp1979-rom-reference.json
audit_rom/build_reference.py
audit_rom/analyze_oracle.py
audit_rom/oracle_capture.lua
audit_rom/run_oracle.sh
audit_rom/mame_ddragon3.cpp
audit_rom/mame_ddragon3_v.cpp
audit_rom/oracle_verified/
audit_rom/verified/
```

`oracle_verified/` tiene **78 archivos y 1898157 bytes**: CSV de actores y
movimiento, resumen, validación, logs y nueve conjuntos de imagen/RAM/MMIO.
Conservarlo completo: `analyze_oracle.py` exige nueve binarios de cada tipo;
guardar sólo los CSV y validation.json rompe esa comprobación. Los frames son
0000, 0120, 0240, 0360, 0480, 0600, 0720, 0840 y 0960; cada uno incluye PNG y
binarios ram/sprites/fg/bg/text/palette/scroll. `verified/` conserva 12 logs y
desensamblados de procedencia, sólo 34457 bytes.

La captura es el fixture original de attract state2 con sustitución declarada
del ID06 por ID09 antes de la inicialización. No convertir su etiqueta en una
afirmación de combate pagado original o de hardware físico. El documento de
autoridad detalla el alcance.

### Si se exige recapturar MAME, además de analizar la evidencia guardada

Conservar también los originales ya disponibles dentro de wp1979:

```text
audit_rom/src_01/
```

Son 15 ROMs, 10158080 bytes. Evitan depender de la copia exterior
`project_sources/01-wwfwfest.zip`. Para `build_reference.py` y MAME pueden
empaquetarse como un ZIP `wwfwfest.zip` dentro de wp1979, con los nombres de ROM
en la raíz. No hace falta duplicar `audit_rom/main68k.bin`: el programa canónico
ya está incluido como `audit_gameplay/assets/ROM68K.BIN`.

Recapturar MAME requiere una instalación íntegra de **MAME 0.264** con sus
bibliotecas y datos. Es una dependencia de sistema adicional, no parte de los
dos ZIP ni necesaria para las pruebas host v4. No recomendar el binario de
workspace cuya truncación se documentó durante el trabajo.

El launcher histórico tiene estas dependencias externas explícitas:

- Binario predeterminado `/tmp/wp1979-mame-package/usr/games/mame`, sustituible
  con `WF_MAME`.
- ROM fija en `<raíz del workspace>/project_sources/01-wwfwfest.zip`.
  `WF_MAME` no cambia esa ruta.
- Bibliotecas y plugins bajo `wp1979/tooling/host/`.
- `oracle_capture.lua`, si se invoca sin `WF_ORACLE_OUT`, usa el path absoluto
  de scratch de la sesión original.

Para una recaptura portable, invocar MAME con un `-rompath` dentro del paquete y
`-autoboot_script` apuntando al Lua incluido; fijar `WF_ORACLE_OUT` a un nuevo
directorio dentro de wp1979. Esto evita depender de los defaults históricos sin
modificar el script ni el juego. Si se usa literalmente `run_oracle.sh`, deben
materializarse sus rutas y proporcionar `WF_MAME` válido. Documentar esta
dependencia en el README general; no presentar `/tmp` como contenido entregado.

## 5. Procedencia de la secuencia de control y mediciones finales

Para ejecutar `verify_union.py` bastan los valores observados y hashes que ya
incluye su ZIP. Para volver a derivar la correspondencia de input desde el
informe original, añadir también:

```text
measure/candidate-2-controller/report.json
render_opt/takeover/controller-schedule.json
```

Son fuentes de procedencia de la unión, no la medición final de candidate4.
La prueba final de vídeo, audio, BIN/CUE, cadencia y cero lecturas pertenece a la
selección de `measure/` y scripts de captura que integra el responsable del
paquete nativo. Los dos ZIP de auditoría no contienen esos ejecutables Saturn,
el core Yabause final ni la captura final de candidate4.

## 6. Orden de reconstrucción previsto

Los comandos siguientes son instrucciones para un receptor del paquete;
**no se ejecutaron durante esta tarea de selección**. Requieren Python 3.10+
y un compilador C del host (`cc`, C11/C99 y biblioteca C de desarrollo). Los
scripts Python usan la biblioteca estándar. El toolchain SH-2/Yaul del build
Saturn se documenta en el paquete nativo y no interviene en estas pruebas host.

Desde la carpeta que contiene `wp1979/`, con los assets y enlace de escenas ya
materializados:

```bash
python3 wp1979/audit_gameplay/host/build_tag_probe.py
python3 wp1979/audit_gameplay/fixes/build_oracle.py

python3 wp1979/audit_gameplay/fixes/build_fixes.py --baseline
python3 wp1979/audit_gameplay/fixes/test_rom_fragments.py --baseline
python3 wp1979/audit_gameplay/fixes/test_human_producer.py --baseline

python3 wp1979/audit_gameplay/fixes/build_fixes.py
python3 wp1979/audit_gameplay/fixes/test_rom_fragments.py
python3 wp1979/audit_gameplay/fixes/test_human_producer.py

python3 wp1979/audit_gameplay/fixes/state6/build.py --baseline
python3 wp1979/audit_gameplay/fixes/state6/test_state6.py --baseline
python3 wp1979/audit_gameplay/fixes/state6/build.py
python3 wp1979/audit_gameplay/fixes/state6/test_state6.py

python3 wp1979/audit_gameplay/fixes/test_real_inputs.py --binary wp1979/audit_gameplay/fixes/state6/guard_probe_v4 --out wp1979/audit_gameplay/reproduced/input-regressions
python3 wp1979/audit_gameplay/fixes/state6/verify_final.py
python3 wp1979/audit_gameplay/residency_takeover_v1/verify_union.py
```

El orden importa: state6 reutiliza el objeto de humano corregido que genera
`build_fixes.py`; éste reutiliza los 114 objetos originales generados por
`build_tag_probe.py`. Los casos `--baseline` son demostraciones rojas esperadas,
no un fallo de empaquetado. No se necesitan objetos o binarios host precompilados.

### Regenerar las 984 fases sin empaquetar miles de snapshots

Los ZIP ya incluyen los resultados y los fixtures puntuales indispensables.
Para repetir las 984 fases, usar el harness v4 con la cinta incluida
`host/tag-after3800-input.csv` y estas variables:

```text
WP1979_SEED=1085
WF_PROBE_FRAMES=6128
WF_PROBE_PAD_SCHEDULE=<path absoluto al CSV anterior>
WP1979_CAUSAL_TRACE=<nuevo directorio>/phase-trace.csv
WP1979_CAUSAL_START=5800
WP1979_CAUSAL_END=6127
WP1979_CAUSAL_RAM=<nuevo directorio de snapshots>
WP1979_CAUSAL_RAM_START=5800
WP1979_CAUSAL_RAM_END=6127
```

Crear los directorios antes y ejecutar desde `audit_gameplay/host`:

```bash
../fixes/state6/guard_probe_v4 ../assets/ROM68K.BIN ../assets/SPRITES.PAK ../assets/R18RING.BIN ../assets/HOTSPR-ROM-v4-32.BIN
```

Después invocar `fixes/compare_causal_phases.py` con `WP1979_PHASE_INPUT` igual
al directorio de snapshots, `WP1979_PHASE_OUTPUT` a otro directorio nuevo y
`WP1979_PHASE_NO_TRACE=1`. Todos los directorios pueden estar bajo
`wp1979/audit_gameplay/reproduced/`. El comparador produce results.json; comprobar
explícitamente 984 filas, terminación de los 984 oráculos, cero accesos sin mapa
y listas `differences` vacías. El comparador registra diferencias, no las
convierte por sí mismo en un código de salida de fallo.

## 7. Excluir del ZIP general

- `audit_gameplay/fixes/experiments/` y el experimento rechazado de raw16.
- Capturas masivas `continuous-snapshots`, `corrected-snapshots`,
  `state6/observed-v4` y sus volcados de ROM, salvo los fixtures puntuales ya
  enumerados en el índice v4. Sólo `state6/observed-v4` ocupa 32881353 bytes y
  5906 archivos; es regenerable con las instrucciones anteriores.
- Directorios de casos RAM rojo/verde generados; los tests generan de nuevo
  esos casos desde los fixtures pequeños. Conservar los resultados y logs que
  ya van en los ZIP.
- Rutas alternativas completas, atlas exploratorios, búsquedas de semillas y
  perfiles intermedios, excepto los 13 fixtures aprobados de la sección 3.
- `audit_rom/oracle/`, duplicado de la captura conservada; las extracciones de
  workpacks, documentación genérica MAME y experimentos de compresión/allposes.
- Objetos host, ejecutables reconstruibles, cachés de paquetes y árboles
  completos de toolchains que no estén seleccionados por el build nativo.

Con esta selección quedan presentes las dependencias existentes de los
verificadores sin cambiar sus fuentes, junto con la procedencia y el conjunto
pequeño de ROM independiente. Las mediciones finales del candidato se conservan
como evidencia distinta de los fixtures históricos de regresión.
