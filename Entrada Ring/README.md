# Entrada Ring · WP01 R02 · entrada TAG para Saturn

Fuentes de la revisión R02 aprobada por Ale, depositadas en este directorio para que Codex integre posteriormente la entrada TAG en el juego. El código ejecutable de la escena y del audio se conserva byte a byte respecto al paquete entregado. Esta subida añade un bloque de referencia; no conecta ni sustituye el runtime del repositorio.

**Empieza por [INTEGRACION_CODEX.md](INTEGRACION_CODEX.md).** Describe las APIs existentes, cadencia, selección de luchadores, mapas de memoria y los recursos que deberá coordinar el integrador. La ROM original sigue siendo la autoridad.

## Contenido

| Ruta | Función |
|---|---|
| `source/wp01_scene.c` y `.h` | Lógica de entrada de la pareja, movimiento, fases, luz y estado terminal |
| `source/main.c` | Renderer VDP1/VDP2 y ejecutable autónomo de referencia; sus controles/repetición no son el flujo final del juego |
| `source/wp01_pairs.h` | Las 29 parejas legales y la correspondencia de IDs internos |
| `source/wp01_audio*.c` y `.h` | Cola de registros y temporizador FRT para la síntesis SCSP nativa |
| `research_scsp/`, `research_inventory/`, `research_events/` | Conversión de instrumentos, muestras y eventos originales; fuentes de referencia y evidencias seleccionadas |
| `tools/build_native_audio.py` | Regeneración de `SOUNDRAM.BIN` y `AUDIOREG.BIN` |
| `source/tests/` | Pruebas de cola, parejas, escena y telemetría |
| `VERIFICACION.json`, `evidence/` | Resultados de la revisión entregada e identificación de sus artefactos |
| `ASSETS-MANIFEST.json`, `tools/restore_assets.py` | Recuperación local de recursos externos con verificación SHA256 |
| `COMPILAR.md` | Instrucciones originales de compilación de R02 |

## Recursos para compilar

El repositorio conserva su política de fuentes, contratos y hashes: los gráficos binarios y las muestras/bancos de audio permanecen en el paquete externo aprobado. No están codificados ni escondidos dentro del código subido.

Paquete completo: [WF-WP01-ENTRADA-TAG-R02-AUDIO-NATIVO-FUENTES.zip](https://chatgpt.com/library?file_id=libfile_4146d0d0bd40819182f312fdec377858&account_id=personal).

SHA256 del ZIP:

```text
dcacfe5d4de8067067194a9c0117fad95ad912982ebbb0030bf1e09cc4a8024d
```

Una vez disponible ese ZIP en el entorno de Codex, ejecutar desde la raíz del repositorio:

```sh
python3 "Entrada Ring/tools/restore_assets.py" /ruta/WF-WP01-ENTRADA-TAG-R02-AUDIO-NATIVO-FUENTES.zip
python3 "Entrada Ring/tools/restore_assets.py" --verify
```

El restaurador comprueba el ZIP entero y los 25 recursos individuales antes de escribir. Instala sólo las rutas del manifiesto y rechaza sobrescribir un archivo con contenido distinto. `.gitignore` mantiene esos recursos y los productos de compilación fuera del commit. El enlace del paquete requiere acceso a la cuenta del propietario; un entorno Codex sin ese acceso necesita recibir el ZIP localmente. No sustituir recursos ausentes por aproximaciones.

Para usar el Makefile autónomo, trabajar en una ruta sin espacios y conservar intacto este depósito:

```sh
wp01_build_dir=$(mktemp -d /tmp/wp01_entry_XXXXXX)
cp -a "Entrada Ring/." "$wp01_build_dir/"
cd "$wp01_build_dir"
```

Después seguir `COMPILAR.md`, con el SDK Yaul del proyecto. Los dos bancos restaurados ya permiten compilar; su regeneración opcional usa `python3 tools/build_native_audio.py`. Las capturas WAV y otros resultados extensos de investigación permanecen en el ZIP completo.

## Estado y límites para la integración

- La entrega R02 generó seis voces FM con 24 operadores SCSP y reprodujo las muestras OKI originales por separado. Su timbre todavía difiere del YM2151; no se declara audio idéntico bit a bit al arcade.
- La evidencia previa incluye estados de escena contrastados con MAME, cero VBlank perdidos en las pruebas instrumentadas y arranque/captura en Mednafen con BIOS real. La aceptación del usuario no especifica una plataforma de ensayo; no se añade un nuevo HARDWARE-PASS a los documentos del proyecto.
- El banco SCSP de esta entrada y su inicialización global colisionan con recursos del backend WP24 si se usan simultáneamente sin adaptación. La guía detalla el solapamiento, los 32 slots afectados por init/stop, el vector FRT y la vida del buffer de eventos.
- Al preparar esta subida, el repositorio no contenía el runtime canónico completo del juego. Codex debe integrar este bloque bajo su propietario de escenas real, cuando disponga de él; no convertir el `main.c` de prueba en el master.
- El nombre WP01 de esta entrada no cambia las responsabilidades del WP01 histórico de finishers del proyecto.

## Trazabilidad de la subida

`docs/R02-ORIGINAL-MANIFEST.json` conserva el manifiesto íntegro del ZIP original. `UPLOAD-MANIFEST.json` identifica los archivos depositados, sus hashes y su relación con ese paquete. La única prueba antigua excluida es `source/tests/test_audio_order.c`, que usa la interfaz PCM de R01 y no corresponde a R02; no interviene en la compilación del programa. Las fuentes de producción no se han cambiado para esta subida.

El código y las tablas de terceros conservan sus avisos en las fuentes y en `docs/LICENCIAS.md`, `docs/GPL-2.txt` y `research_scsp/YMFM-LICENSE.txt`.
