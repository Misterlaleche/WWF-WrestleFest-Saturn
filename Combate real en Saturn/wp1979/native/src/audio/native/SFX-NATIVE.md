# Efectos y voces SCSP nativos — R01 experimental

Se implementan los 98 comandos 0x20–0x81 del modelo recuperado A15/A16, sus 165 eventos y las 114 entradas de bancos (95 muestras distintas). El 68000 decodifica ADPCM MSM6295 a PCM16 y ejecuta bancos, prioridades, tres secuencias lógicas y la regla de supresión F1. Hay cuatro voces PCM físicas. El comando 0x20 conserva su operación vacía original.

El motor está conectado al arranque de la copia aislada de R18. `wf_r18_scsp_boot` carga el ELF previamente leído, arranca el 68000 y, si detecta el perfil residente, transmite `SFX.DAT` en bloques de 1 KiB. Verifica CRC32, publica la cabecera al final y ordena BIND antes de habilitar la cola de comandos. Los efectos posteriores no leen CD ni descomprimen audio en SH2.

R04 permanece como referencia. No se ha generado BIN/CUE/ISO. Los ELF entregados son ejecutables para integración y pruebas, no una imagen arrancable.

## Estado musical

Continúa disponible el primer instrumento FM de cuatro operadores. La onda se reubica; sus registros de frecuencia, envolvente y modulación conservan el plan observado. No se conectan las músicas del juego a ese instrumento de diagnóstico. Los comandos musicales 0x01–0x12 y fade 0x1F devuelven UNIMPLEMENTED y la cola los cuenta.

Falta convertir el secuenciador musical y todos sus instrumentos/canales a SCSP, resolver la asignación de slots para el conjunto completo y contrastar la mezcla resultante. Esta entrega no completa todas las músicas.

## Mapa exclusivo del perfil residente

| Rango 68000 | Uso |
|---|---|
| 0x00000–0x02FFF | Vectores, código y tabla ADPCM |
| 0x03000–0x0AFFF | Cuatro anillos PCM16 de 4096 frames |
| 0x0C008 / 0x0C040 | Comunicación heredada / buzón de 32 bytes |
| 0x0C100–0x0FFFF | Estado del motor |
| 0x10000–0x117FF | Onda FM de 6144 bytes |
| 0x12000–0x13FFF | Pila |
| 0x14000–0x7F985 | Pack residente de 440.710 bytes |

Quedan 1.658 bytes al final del pack. Slots 0–3: instrumento FM; 4–7: PCM; 30: reloj silenciado, sin escritura en el anillo de modulación. El anillo R04 en 0x60000 pertenece al perfil anterior y sus comandos se rechazan como UNIMPLEMENTED en este perfil. El constructor anterior sigue disponible.

## Reloj, consumo y final de muestra

El slot 30 usa PCM8, OCT=6, FNS=1023 y un loop de 15 páginas de 4096 muestras. Su cursor CA ofrece un reloj independiente de VBlank. La conversión racional es 65536/2047 frames SCSP por página CA; se calcula con DIVU del 68000. El Timer A lógico avanza a 3579545/(64×253) Hz para las secuencias.

Cada voz registra su inicio después del prefill, antes de KEYON. El final físico depende de frames consumidos, con una reserva conservadora por cuantización CA, para conservar la cola de la muestra. El productor prepara 256 muestras al iniciar y repone como máximo 64 muestras por voz y vuelta; solo reutiliza memoria que el consumidor ya dejó atrás, con guarda de 64 frames. Si una voz agota sus muestras, se detiene esa voz y se incrementa `underruns`; el firmware y el juego continúan. Los errores de escritura o del controlador siguen siendo fallos globales.

El paso PCM es 1486/8192 de muestra por frame SCSP: aproximadamente 7999,585 Hz. Los volúmenes OKI se aproximan con TL={0,9,16,25,32,39,48,55,64}; volúmenes 9–15 se silencian. Es una cuantización de amplitud, no una afirmación de igualdad de la mezcla analógica.

Las definiciones CA, loop y KEYONEX se contrastaron con [MAME SCSP](https://github.com/mamedev/mame/blob/master/src/devices/sound/scsp.cpp). La conversión TL se contrastó con la [tabla 4.18 del manual de SEGA](https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_24.htm). Son referencias técnicas; la validación física sigue pendiente.

## Buzón ABI 3

Se mantienen offsets y tamaño. `reserved[2]=0x53F1` identifica este perfil; `reserved[0]` registra el último raw16 y `reserved[1]` los underruns. Resultado 4 significa rechazo esperado por prioridad y no envenena la cola.

| Comando | Operación |
|---|---|
| 1 / 2 | Iniciar / parar instrumento FM de diagnóstico |
| 3–6 | Incompatibles con el pack residente |
| 7 | Latch arcade raw16; bits bajos 0x20–0x81 ejecutan efectos |
| 8 | Validar y vincular pack residente |
| 9 | Parar efectos y desvincular pack antes de subirlo |
| 10 | Consultar máscara de voces; bit8 indica pack vinculado |
| 11 | Inicio OKI directo: `(voz<<12)|(banco<<11)|(sample<<4)|volumen` |
| 12 | Parar máscara de voces PCM |

El arranque SH2 silencia MVOL durante copia/reset y lo restaura tras READY. El firmware deshabilita SCIEB y MCIEB antes de habilitar sonido. El cargador de ELF exige los tres rangos del perfil para proteger anillos, comunicación, onda y pila.

## Reproducir las pruebas

Desde la raíz del repositorio, con Python, MSVC Build Tools y el compilador m68k-elf compatible:

```powershell
python tools/audio/native_probe/build_sfx_pack.py --rom RUTA/31j10.ic73 --manifest tests/audio/native-driver/reference/ROM_OKI_MANIFEST.csv --model tests/audio/native-driver/reference/wf_audio_sfx.c --out RUTA/pack
python tools/audio/native_probe/build_sfx_driver.py --toolchain RUTA/m68k-elf --instrument RUTA/first-instrument --out RUTA/firmware
python tools/audio/native_probe/verify_sfx.py --pcm RUTA/WF_OKI_PCM16BE.PAK --pack RUTA/pack/SFX.DAT --out RUTA/tests --vcvars "RUTA/VC/Auxiliary/Build/vcvars64.bat"
python tests/audio/native-driver/test_sfx_firmware.py --build RUTA/firmware --pack RUTA/pack/SFX.DAT --audit tests/audio/native-driver/reference/WF_OKI_AUDIT.json --pcm RUTA/WF_OKI_PCM16BE.PAK --report RUTA/tests/cpu-tests.json
```

La última orden requiere `machine68k`. El PCM auditado se identifica por SHA256 antes de comparar. Las fuentes de referencia solo se usan como oráculo de tests; no se enlazan en el firmware.

Las pruebas incluyen PCM completo de las 114 entradas tanto en C como ejecutando el ensamblador real, las 98 secuencias y 30.000 ticks con comandos mezclados, bancos/prioridades/F1, niveles, final a fase arbitraria, prefill, refills, starvation, pack inválido, CRC, errores de I/O y propietario del bus. El test 68000 ejecuta cuatro voces largas durante dos segundos y todos los comandos con un cursor SCSP sintético.

El resultado de CPU es una comprobación de instrucciones y plazos bajo ese modelo. No emula sonido SCSP, esperas/arbitraje de RAM compartida ni certifica funcionamiento en Saturn real. Se necesita esa prueba antes de considerar cerrado el backend físico.
