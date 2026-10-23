# HUD, aliados y stamina — 23 de octubre de 2026

Investigación y correcciones sobre `95328a2`. Entrega solicitada: capturas
reales de Mednafen; no paquete BIN/CUE. No hay declaración de partida
completa, ROM-EXACT universal, rendimiento nuevo ni validación física.

## HUD: causa y corrección

`branchb_emit_75ee_actor` limpiaba seis celdas avanzando el destino y luego
rellenaba desde la séptima. Eso dejaba la barra roja en su sitio y escribía
la parte amarilla sobre el retrato/barra contiguos. La ROM, en `$7624/$7628`,
vuelve a ejecutar `$7680` y añade `$20C`: el relleno reinicia en el origen.
Se restaura ese paso. No se cambian dimensiones, energía, paletas ni reglas.

La expectativa histórica de `test_runtime17_7548.c` contenía las direcciones
equivocadas y también se corrige contra esas instrucciones. El nuevo test
`test_hud_stamina_separation.c` comprueba los retratos y seis celdas de ambos
lados hasta los nombres de patrón VDP2. Falló antes del arreglo; pasa normal
y con ASan/UBSan. El primer sanitizer encontró además una expresión de
aritmética de punteros incorrecta en el propio test; está corregida y la
ejecución final pasa sin diagnósticos.

## Stamina: valores y presentación

58 fixtures ejecutan las instrucciones originales de `$24E58`, `$7548`,
`$8ECC`, `$7506`, `$75EE` y `$782C`, sin sustituir sus decisiones. Se comparan
RAM, FG0, paletas, D0–D7 y escrituras ordenadas; los helpers también A0–A6.
Además, 2196 invocaciones reales del owner de daño, con 21 eventos de daño,
se capturan del HOST y se reejecutan en ROM: cero diferencias finales.

Se corrigen dos omisiones demostradas: sin daño pendiente no se recalcula
el tier tras recuperar energía; los temporizadores `$54/$52` y `$D0/bit3`
se mantienen también durante el lockout de daño.

El A/B aislado de esas dos correcciones conserva los 4391 frames y todos
los valores de stamina de la referencia anterior. El jugador empieza en
110; el primer daño es en 139, con energía real109 y visible110. El aliado
conserva106 durante esa referencia. La barra larga/roja era un problema
de escritura del HUD, no prueba de una pérdida acelerada de salud.

Este contraste prueba la aplicación del daño con las mismas entradas al
owner; no prueba que toda la frecuencia de impactos de una partida completa
sea idéntica al arcade. La cadencia de recuperación detallada se verifica
para Royal `$8ECC`; no se extrapola a todo el modo TAG.

## Aliados: causa de las decisiones con distancia falsa

La fase CPU omitía `$1C19C -> $1C228`, justo antes de decidir por cada actor.
Las distancias B0/B2 y clase B4 quedaban obsoletas, a menudo cero. En el
snapshot real del frame654, el aliado usaba `0/0/0`, donde la fase completa
ROM produce `300/9/2`. La siguiente decisión cambia a movimiento y coincide
con las instrucciones originales. En900 produce `313/20/2`.

Se integra el helper existente, en el orden ROM por actor. Se corrigen
también los registros vivos de seguimiento `$1D398`, la restauración
incorrecta del bit1 de B7 tras `$1C2F4`, el scratch `$15E4` del cálculo de
dirección y la selección/tick de poses retenidas FF. El target nulo del
arranque lee los valores reales del vector ROM; no se inventa un rival.

50 snapshots CPU, cuatro de poses retenidas y seis fases CPU completas,
incluidas dos decisiones siguientes aisladas, coinciden con la ROM. Pasa
también la regresión existente de 888 decisiones. La cobertura es muestreada.
Los gestos normales de espera, dos rótulos1P y cuerdas delante del aliado
en el apron también existen en el vídeo arcade y se conservan.

## Capturas y comprobación real de Saturn emulado

Mednafen1.32.1, render software sin shader/scanlines. Capturas XWD reales,
convertidas con FFmpeg. `HUD-antes-despues-Mednafen.png` apila dos recortes
de esas capturas; arriba antes y abajo después, ampliados con vecino próximo.
No es un framebuffer reconstruido por un test HOST.

- Comparación HUD: frame1080, semilla0, secuencia controlada RIGHT60–179 y
  A180–183. Antes A.BIN `5c19c683...`, corrección HUD intermedia `c568cc58...`.
  3696 píxeles cambian, sólo en el HUD; fuera de esa zona la imagen es idéntica.
  Esta comparación aísla el HUD y precede a la última corrección de la fase CPU.
- Versión final: `Mednafen-HUD-aliados-frame600.png`, semilla0, nueve entradas
  del adaptador en FF, sin entrada del usuario. El test de captura detiene el runtime tras
  ACK600; no fuerza posiciones, animaciones, salud ni resultado. El estado
  real leído es jugador90/90 real/visible y aliado106/106, sin owner fatal.
  Los sprites VDP1, texturas, fuente y mapa HUD se verifican en VRAM emulada.

A.BIN de esta última captura:
`7a4712a11d8eb446b2b1136ff830c9df7d2ca0fc6d23477b4511ee267f280328`.
Los 4096 bytes iniciales identifican la WRAM del ejecutable exacto. Los
offsets del observador se extraen con el mismo ABI SH-2. Los observadores
sólo leen memoria. La captura no acredita un combate completo ni Saturn real.

## Bloqueos finales conocidos

Actualizar las distancias cambia legítimamente las decisiones y alcanza
owners que la secuencia anterior evitaba. Los endpoints4391/14409 anteriores
no se atribuyen a este árbol corregido:

- Sin entrada: primer bloqueo HOST en669, aliado slot1, estado2, `$1D7B6`.
- Secuencia controlada: primer bloqueo en460, slot5, selector4022, `$12614`.

Se conservan como bloqueos explícitos. No se ejecutan aproximaciones ni
saltos de fase para ocultarlos. La investigación identifica y corrige la
entrada de distancia falsa, pero no cierra todas las acciones de los aliados.

## Evidencia y reproducción

`validation/results/runtime18/hud-allies-investigation/` conserva capturas,
observaciones SH-2/VRAM, logs de compilación y pruebas, ciclos rojo/verde,
trazas finales y subcarpetas `stamina/` y `allies/`. El manifiesto
`allies/SNAPSHOT-MANIFEST.json` identifica los551 snapshots históricos,
su origen y SHA256; no se deben reemplazar por una nueva traza. Sus resultados
históricos con fallos son evidencia del antes, no tests verdes actuales.
La revisión independiente reejecutó los oráculos y no detectó nuevos defectos
Critical/Important de implementación; mantiene la limitación de partida incompleta.

```sh
make -j4 -C src/runtime test test-runtime17-7548 test-runtime18-hud test-runtime18-hud-yaul
make -C src/runtime test-runtime18-hud-stamina-separation test-runtime18-hud-stamina-separation-asan
python3 src/runtime/tests/r18/stamina_verify_rom.py src/runtime/audit/runtime18-cd/ROM68K.BIN
python3 src/runtime/tests/r18/allies_verify_cpu_rom.py validation/results/runtime18/hud-allies-investigation/allies
python3 src/runtime/tests/r18/allies_verify_held_rom.py validation/results/runtime18/hud-allies-investigation/allies
python3 src/runtime/tests/r18/allies_verify_phase_rom.py validation/results/runtime18/hud-allies-investigation/allies
R18_CAPTURE_STOP=600 DOCKER=/ruta/docker PYTHON=/ruta/python sh validation/build-runtime18.sh /ruta/sh2eb-elf
```

Los oráculos requieren `machine68k` de los requisitos ROM del proyecto.
La entrada de captura es sólo un fixture de prueba: el build normal sigue
usando `wf_saturn_entry.c`. No se entrega otro BIN/CUE en esta investigación.
