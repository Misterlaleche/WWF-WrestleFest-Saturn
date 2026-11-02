# Continuación exacta de Audio 2

**NOT PRODUCTION READY · HARDWARE PENDING · aprobación auditiva pendiente.**

## Punto de partida que debe conservarse

Trabajar sobre la rama `audio-2-production-ready` y leer primero `ESTADO.json`,
`AUDIO-FINAL-AUDIT.md` y `CODEX-INTEGRATION.md`. R04/A53, `audio-2`, las fuentes
de WP1979/WP1980 y el resto del repositorio se conservan. No partir de una
carpeta experimental por su fecha o su nombre.

El candidato seleccionado combina:

| Elemento | Identidad o contrato |
| --- | --- |
| Firmware 68000 sin instrumentación privada de tiempos | `cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090` |
| `src/reference_driver/z80-native.c` | `c408e4a8ad5d7b54a55706f4c96fd9f4d39271dcd0fb5e2de14331af138f4fad` |
| Compilación SH-2 | Núcleo Z80 `-O2`, secuenciador `-Os`, nueve admisiones escalares verificadas |
| API / mailbox / módulo de juego | API1 / mailbox nativo ABI3 / módulo dinámico ABI4; son tres contratos distintos |
| Ventana caliente del juego | 18 KiB, incluida la estructura de estado; pila slave separada de 8 KiB |
| Recurso de efectos | 440.710 bytes, generado desde el ZIP original; no se incluye en Git |
| Emulador | Mednafen 1.32.1 independiente, sin modificaciones de reloj ni memoria |

No seleccionar los lifts adicionales, la ventana de 24 KiB o el DSP ZOH por
sus resultados parciales. El experimento ZOH tiene evidencia causal útil,
pero no es el firmware de las matrices de aceptación.

## Primer experimento pendiente: banco OKI activo

El defecto está demostrado con el controlador original y una prueba causal:
2.663 muestras decodificadas y 14.680 frames de mezcla cambian al conservar el
banco de inicio de una voz. `docs/ACTIVE-OKI-BANK-AUDIT.md` contiene las
direcciones, rutas de entrada, generadores y límites de memoria. No hace falta
volver a investigar si el banco global afecta a voces activas: sí lo hace.

El siguiente cambio debe empezar por un prototipo host de resolución por
dirección original y reproducción del estado ADPCM. Usar el mapa privado
generado por `tools/reference/audit_oki_storage.py`; comprobar todas las
517.450 direcciones válidas de ambos bancos, las dos paridades de nibble,
fronteras de muestra/ring, cambios repetidos dentro de un intervalo de
checkpoint y el replay mixto de 64 comandos que demuestra el error.

Después, medir una implementación nativa aislada. Debe unificar la ruta
musical fechada y la ruta SFX inmediata, conservar la historia del decoder y
corregir los datos preparados después de la frontera efectiva. Cambiar sólo
`s->bank` no arregla los PCM ya escritos. El ring representa hasta 512 ms a
8 kHz; el lead musical representa 92,88 ms a 44,1 kHz. No son la misma unidad
ni el mismo horizonte de conocimiento.

La extensión de recursos estudiada necesita 704 bytes; 528 bytes es sólo un
presupuesto mínimo de checkpoints. No existe todavía una reserva, ABI de
recurso, algoritmo ni prueba de plazo aprobados para esa solución. El código
68000 también tiene márgenes pequeños de linker: no introducir código nuevo
en los huecos sin comprobar nuevamente ELF, pilas y reservas.

## Segundo bloqueo: `1F`, plazos y coste SH-2

El original `1F` provoca trabajo real del controlador: modifica RAM y
registros. Los fallos observados no se explican por una FIFO FM saturada; en
los diagnósticos ocupa sólo 0–22 entradas. No saltar el controlador, inventar
un fade lineal, eliminar notas ni aumentar plazos/buffers.

Para reproducir un caso completo desde una salida nueva, después de restaurar
el entorno y generar los recursos según `CODEX-INTEGRATION.md`:

```bash
python3 tools/run_music_matrix.py \
  --resources "$WF_AUDIO_BUILD/resources" \
  --firmware "$WF_AUDIO_BUILD/firmware/SCSP.ELF" \
  --sdk "$WF_AUDIO_ENV/sh2eb-elf" \
  --environment-root "$WF_AUDIO_ENV" --bios "$WF_AUDIO_BIOS" \
  --reference-report "$WF_AUDIO_BUILD/full-board/reference-report.json" \
  --out "$WF_AUDIO_BUILD/new-native-fade-01" \
  --first 0x01 --last 0x01 --score-seconds 600 --tail-seconds 8 \
  --presentation-speed 4 --displays 94 --wall-seconds 1800 \
  --profile-label cddd-scalar9-seqOs-fade-reproduction
```

Las variables deben apuntar a ubicaciones locales reales del propietario y
`--out` debe ser nuevo. `--presentation-speed 4` sólo utiliza el pacing
upstream; la prueba separada conservó 2.646.000 valores PCM idénticos. También
puede usarse velocidad1 ajustando el timeout de pared; nunca confundirlo con
tiempo Saturn. Revisar el resultado, no sólo el código de salida del emulador.

La equivalencia de 600 segundos contra R01 es una regresión importante, pero
R01/A53 conservan un MMIO parcial Timer-B. La referencia actual full-board
incluye Timer A/B y busy/IRQ. Verificar también esa frontera temporal antes
de declarar equivalencia arcade completa. La igualdad del prefijo de valores
de registros no sustituye la igualdad de sus instantes.

## Regresión mínima tras una corrección del runtime

1. Ejecutar las pruebas host de la parte cambiada y después los 18 comandos
   durante 600 segundos más `1F`. Conservar el resultado negativo anterior.
2. Repetir las 98 órdenes SFX contra inicios físicos del Z80 original, la
   decodificación completa y las pruebas de banco activo. La regresión A15
   sola no detectaba los 17 reinicios adicionales `F1` ya corregidos.
3. Reconstruir firmware, diagnósticos y módulos/master ABI4 juntos. Un PASS
   histórico no puede atribuirse a un ELF distinto.
4. Capturar nuevamente la matriz larga, transiciones, mezcla y sobrecarga.
   Usar el auditor que vincula ELF, firmware, F5, WAV, memoria y contadores
   completos; conservar errores observados antes de cualquier recuperación.
5. Repetir las parejas TAG y Royal base/4 MiB con sus fuentes originales y
   perfiles de memoria separados. TAG dispone de cinta original idéntica;
   Royal conserva política original y estado/actores/productores idénticos,
   pero todavía carece de evidencia de entradas por llamada. No elevar esta
   última comprobación a una cinta idéntica inexistente.
6. Generar nuevos WAV sin normalización y comparar tono, tempo, ataques,
   envolventes, colas, mezcla y latencias. La aprobación auditiva del dueño y
   la Saturn NTSC-U con MODE siguen siendo criterios separados.

Los resultados actuales de combate y sus límites están en los informes v2:
TAG entrega64 comandos pero conserva un retraso y55,330 pasos/s; Royal base
pierde4 FM y termina el worker; Royal4MiB entrega133 comandos pero conserva
6 retrasos. El cartucho no resuelve el requisito de ejecución sin expansión.

## Preservación de evidencia y publicación

No sobrescribir salidas de compilación, referencias o capturas. La campaña
larga inicial sufrió ENOSPC en sus dos últimos casos: la colección reconciliada
identifica los reintentos con los mismos ELF/ISO, sin convertir el intento
interrumpido en un éxito. Los temporales retirados se comprobaron byte a byte
contra los vídeos originales retenidos; sus manifiestos conservan tamaños y
hashes. Se preservan todos los fallos reales del motor.

El repositorio contiene fuente, herramientas, licencias, hashes y metadatos.
ROM, BIOS, WAV, discos y ELF con datos originales son privados. Los ZIP de
validación/listening identifican perfiles y aceptación pendiente. Si una
entrega privada está bloqueada por autorización del destino, conservar los
archivos revisables y solicitar sólo esa autorización; no publicarlos en Git
ni cambiar de medio para eludir la restricción.

No fusionar a `main` ni cambiar el estado a PRODUCTION READY mientras exista
un criterio de software fallido o sin comprobar. Ningún trabajo continúa
ejecutándose después de cerrar una sesión sin una herramienta activa.
