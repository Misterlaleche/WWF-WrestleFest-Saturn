# Implementación de «Combate real en Saturn» — WP1979

**Estado: CANDIDATE · EMULATOR-PASS · HARDWARE-PENDING.**

El usuario ha pedido conservar esta implementación antes de poder probarla en
Saturn física. Esta carpeta contiene las fuentes de la prueba nativa de combate
TAG de **Hulk Hogan + Ultimate Warrior contra Ted DiBiase + Mr. Perfect**, las
correcciones y optimizaciones comprobadas, y las instrucciones para que Codex u
otro chat puedan estudiar e integrar el código. La incorporación al juego
receptor y su validación física siguen pendientes.

La autoridad continúa siendo la ROM original. Esta incorporación añade una
carpeta independiente; no promueve el fixture de prueba a bucle maestro ni
cambia el estado de validación de las demás escenas o del audio del proyecto.

## Lectura para Codex u otro chat

1. [INTEGRACION-CODEX.md](wp1979/github_handoff/INTEGRACION-CODEX.md): archivos
   relevantes, entrada/salida del combate, memoria, vídeo, mando y audio.
2. [COMPILAR.md](COMPILAR.md): restauración de datos y build sin modificar el SDK.
3. [Informe de la entrega](wp1979/release/wp1979-informe.txt) y
   [revisión independiente](wp1979/tooling/review/independent-review-v3.md).
4. [Estado e identidad](ESTADO.json), [fuentes](FUENTES-SHA256.json) y
   [recursos externos](RECURSOS.json).
5. [Verificación del empaquetado](VERIFICACION-EMPAQUETADO.json): restauración
   de recursos, comparación de fuentes y rechazo de sobrescrituras.

Antes de integrar, leer también los contratos de la raíz del repositorio:
`README.md`, `PROJECT-STATE.md`, `HANDOFF.md` y `docs/SATURN-PIPELINE.md`.
El diagnóstico de este bloque no modifica ni cierra los pendientes globales.

## Qué está dentro de Git

- Árbol final `wp1979/native/`: fuentes SH-2, audio, cabeceras, tablas generadas,
  Makefile y herramientas de validación. Las fuentes se conservan byte a byte.
- Referencia original `wp1979/reference_r18/`, fijada en
  `d95b8c37f62402bce4dead13cf7c0567b8fc2439`.
- Scripts de captura/análisis, pruebas y resultados seleccionados, parches y
  procedencia del core empleado para el vídeo.
- Guía de integración y un restaurador con hashes de todos los archivos del
  paquete auditado. No hacen falta objetos de compilación de esta sesión.

El repositorio mantiene su política de no versionar ROM, packs protegidos,
PCM, imágenes de disco o vídeos. Esos recursos y la evidencia extensa están en
el paquete aprobado que se enlaza debajo. El restaurador los comprueba y los
coloca en sus rutas originales; no reemplaza archivos diferentes.

## Paquete externo exacto

**[wp1979-fuentes-pruebas.zip](https://chatgpt.com/library?file_id=libfile_a870ebfd541881919615a0b602b60f62&account_id=personal)**

SHA-256:

```text
83e231d0ba11826683aaa6d22c1746e2fb9fba9ebfc0540529a73f61ab181f33
```

Este enlace pertenece a los archivos de la cuenta del usuario. Un agente sin
acceso debe pedirle ese ZIP exacto; no sustituirlo por otro atlas, ROM o firmware.
El SDK14 del proyecto y `xorrisofs` son dependencias externas documentadas.

Desde esta carpeta, antes de compilar:

```bash
python3 tools/restore_resources.py /ruta/wp1979-fuentes-pruebas.zip --scope build
python3 tools/restore_resources.py --scope build --verify
```

`build` restaura los **19 archivos del staging final**. `--scope all` reconstruye
los **2.080 archivos de contenido** del paquete auditado, incluidos datos,
ejecutables congelados, fuentes del core, dumps y registros completos.
El índice original `SOURCES-SHA256.json` no se copia: sus tamaños y hashes
están incorporados en `RECURSOS.json`. Los archivos de código ya presentes se
verifican y se conservan. Para comprobarlos sin restaurar, añadir `--verify`.

El SDK no admite espacios en la ruta de build. Copiar el subárbol `wp1979/` a
una ruta sin espacios, como explica [COMPILAR.md](COMPILAR.md).

## Resultado conservado

| Medición desde el paso 0 | Resultado |
| --- | --- |
| Combate completo | 7.372 pasos/dibujos en 7.376 VBlanks |
| Media nativa a 60 VBlanks/s | 59,967462 fps |
| Mínimos móviles de 1 / 5 / 10 segundos | 58,0 / 59,6 / 59,8 fps |
| Relevos legales | Hogan→Warrior en 529; DiBiase→Perfect en 5.768 |
| Resultado original | Paso 7.372, `0x4001` |
| CD y fallos fatales durante las dos rutas comprobadas | 0 lecturas, 0 fallos |

La ruta de mando desde el inicio mide 560 pasos/dibujos en 562 VBlanks,
59,786477 fps; tras los primeros 30 pasos, 530/530 y 60,0 fps. El vídeo completo
contiene cuatro intervalos de dibujo de dos VBlanks. Se conservan desplazamientos
y tiempos de animación de ROM, sin multiplicador de velocidad.

La referencia arcade usada es MAME 0.264, 57,444852941 Hz, cuyos totales de vídeo
están indicados como estimados. No es una medición de PCB física.

**Límite de la prueba:** Yabause/libretro, intérprete SH-2, vídeo por software y
BIOS HLE. El renderizador no acredita el presupuesto de rasterizado VDP1 ni la
contención real de buses. Estos resultados no prueban la velocidad de una
Saturn física, una equivalencia completa con la ROM ni cero lecturas ante
cualquier entrada manual. Mantener `HARDWARE-PENDING` hasta la prueba del usuario.

## Entrada de prueba e integración

`Makefile.wp1979` construye un fixture reproducible: arranque TAG directo,
semilla declarada 1085 y cinta de entradas ordinarias. Cruceta/A/B/C/Start del
mando cancela la cinta y entrega el control al mismo intérprete de acciones.

Para el juego normal deben conectarse selección, RNG, progresión y salida del
combate del runtime receptor. **No trasladar la semilla fija, la cinta ni el
bucle final de espera a la campaña.** No concatenar BIN/CUE. Integrar fuentes
con un propietario común de VBlank, SMPC, CD, VDP1/VDP2 y SCSP/SH-2 esclavo.

## Entregables para la prueba física

- [BIN/CUE](https://chatgpt.com/library?file_id=libfile_f8e2dc4830888191b2617cf4a821a906&account_id=personal).
- [Vídeo completo con audio](https://chatgpt.com/library?file_id=libfile_eed8a67fac70819188e1174804ec039f&account_id=personal).
- [Vídeo de toma de control](https://chatgpt.com/library?file_id=libfile_e5da90bd971081919ca67546a5522eb0&account_id=personal).

BIN final, MODE1/2352, SHA-256:

```text
f1232edb31d6e3748b60001d8d82bca5f9b6171d16aaf812f618f3dbf858da5e
```

La misma imagen fue usada en ambas capturas. Las verificaciones del disco
comprueban sus 6.592 sectores, EDC/ECC y 19 archivos. La comprobación física en
Saturn NTSC del usuario sigue pendiente; PAL tampoco se ha validado.
