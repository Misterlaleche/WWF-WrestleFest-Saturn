# Recuperación del checkpoint04

Disco parcial MODE/ROSTER con CD/FRT compartidos y recursos precargados;
no beta completa. Continúa con el mismo terminal verde de TEST03. Expone modo,
slot e identidad original, pero no forma equipos ni abre combate.

## Herramientas y reproducción

Se conservan SDK Yaul/GCC13.2.0, BIOS, paquete TEST03, imágenes Docker y
configuración Mednafen1.32.1 del03; rutas y hashes en
evidence/platform04/configuration.json y los cinco informes de ejecución.
Compilación y emulación se realizaron sin red. SDK/BIOS montados sólo lectura.
No hay backend de audio de juego; el dispositivo dummy del emulador permite
grabar el tiempo nativo. No se acelera el vídeo ni se omiten fotogramas.

Desde production/full_game en Linux con SDK suministrado, Python3,make,xorrisofs:

```sh
export YAUL_INSTALL_ROOT=/sdk
export WF_FRONTEND_PACKAGE=/ruta/WF-SATURN-MODE-ROSTER-HARDWARE-PASS-HANDOFF-R01
export WF_DISC_OUTPUT=/ruta/nueva/disco
bash tools/build_saturn.sh
WF_ROM=/ruta/wwfwfest.zip make test sanitize
python3 tools/run_selection_oracle.py --rom /ruta/wwfwfest.zip --out /ruta/nueva/oracle
```

El oracle necesita Unicorn2.1.4 y compilador C host. Ejecuta ROM original y
compara las diez identidades con la función nativa compilada. Sus fixtures de
confirmación/setup son evidencia de investigación, no una prueba end-to-end.
La fuente ROM y los ZIP permanecen externos a Git. El build verifica los2023
originales y hashes SHA256 de los siete recursos antes de producir manifiesto
de tamaños/CRC. Los timestamps ISO pueden cambiar al recompilar.

```sh
python3 tools/run_platform_checks.py --reference-tools /ruta/wp1980/tooling/mednafen \
 --cue /ruta/disco/wrestlefest-full-game.cue --elf build/saturn/wrestlefest-full-game.elf \
 --out /ruta/nueva/prueba --cart none --sdk /sdk --bios /bios/mpr-17933.bin
```

Repetir con `--cart extram1 --royal`, `--cart extram4` y
`--cart none --confirm-during-scroll`. El runner usa mando mediante teclado,
Xvfb aislado y observación /proc de sólo lectura. No escribe RAM ni savestates.
Toda violación del reloj en una publicación coherente activa falla la prueba;
también falla si el contador de sectores cambia después de entrar en MODE.

Prueba negativa: usar tools/make_bad_resource_disc.py sobre una copia del ISO
bueno, con destino nuevo, y ejecutar el runner con `--expect-asset-error`.
Ese generador cambia un solo byte de SELPAL.BIN y recalcula EDC/ECC; A.BIN no
cambia. Consultar `--help` para sus rutas. El resultado negativo no es el disco
funcional y no se incluye en el ZIP de entrega.

## Evidencia final

Programa SH-2:394004B, SHA256
`a9867da8903c9e4a1f846d55bc18ca5504e0eb89b87bb43d9b28b025f4e77b05`.
Disco MODE1/2352,447 sectores, SHA256
`aee117821c75dceb2d438ad74c5eb44deb25b77029659ed370986a237f68059a`.
Todos sus sectores pasan EDC/ECC y corresponden al ISO. El informe estructural
disc-validation.json no ejecuta un emulador; los arranques están demostrados
por los informes Mednafen separados, no por sus campos boot_test.

| Prueba | ID/bytes SDK | 4MiB válido | Publicaciones activas distintas | Identidad final |
|---|---|---|---:|---:|
| None / TAG | 0 / 0 | no | 247 | 7 |
| 1MiB / ROYAL | 90 / 1048576 | no | 261 | 7 |
| 4MiB / TAG | 92 / 4194304 | sí | 246 | 7 |
| A durante scroll / None | 0 / 0 | no | 203 | 8 |

Cuatro rutas normales PASS, error0, deuda máxima observada2. Contador de
lectura96 sectores antes de MODE y al terminar cada ruta; delta0 durante la
selección. Esto no mide rendimiento de combate ni todas las ventanas del
encargo. Detección4MiB conserva el sondeo de16 puntos de03, no cada byte.

La quinta ejecución PASS valida el fallo esperado: CRC de recurso6, error
de plataforma21, estado0, cero ticks y siete punteros cero. Se comprueba así
una corrupción tardía sin publicar recursos anteriores. No es una prueba de
todos los errores posibles del dispositivo ni del arranque en Saturn física.

Arena de frontend:130848/131072B, alineación32, primeros128KiB LWRAM; punteros
válidos hasta el terminal actual. No reutilizarla al integrar otra escena sin
terminar antes ese propietario. Fin de estáticos HWRAM:0x06064314 frente a
0x0607DF34 en03, ahorro105504B. El binario baja105568B; los recursos se leen
del CD. Todavía falta el presupuesto compartido con escenas y combate.

Los cinco MP4 conservan cada PTS de sus MOV nativos en
work/platform04-tests/{none-01,extram1-01,extram4-01,scroll-edge-01,corrupt-01}.
Un fotograma RGB704x480 del retrato terminal coincide exactamente con03:
SHA256 `54ed84886144121dfdcce8c1069835b04e1ce4488ac2ef90cbaa394a425d6f79`.
La comparación se limita a ese fotograma. Suite host C11/ASan/UBSan y9tests
Python con ROM pasan. Revisión independiente y logs están en evidence/platform04.

## Continuación exacta

1. Conectar créditos/start/modo/roster al propietario original persistente:
  16KiB RAM, registros completos, reloj/input e IRQ/control. Investigar la
  procedencia del estado RNG inicial; no promover semillas de fixtures.
2. Transportar compañero TAG y flash/paletas originales; imagen y posición
  diferenciadas de los dos Demolition en ROYAL. Ver SELECTION-04.md.
3. Adaptar Intro R04 con parada explícita de su controlador PTMR/SPRITE_END
  antes de devolver VDP1 a Yaul. Su terminal no define por sí solo créditos
  ni transición a selección. Mantener eventos audio aunque no haya backend.
4. Ejecutar la cadena match_driver desde el mismo estado; conectar render y
  efectos de Entrada/Presentadora y después el handoff original al combate.
5. Arena/cachés dinámicas estándar/4MiB, gameplay/progresión completos y
  todas las pruebas de recorrido, cadencia y ventanas del encargo.

Direcciones contradictorias>10 siguen con error explícito, semántica ROM OPEN.
No hay vídeo de juego completo ni aprobación física de este disco. SPEC.md
continúa pendiente en todas las fases que exceden este checkpoint parcial.
