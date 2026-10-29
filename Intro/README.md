# Intro — WWF WrestleFest WP0 R04

Código fuente de la intro independiente para Sega Saturn NTSC, desde la pantalla antidroga hasta el desarrollo completo de la pantalla de título y su animación. Corresponde a la entrega **WF-WP0-R04**, tras la cual Ale confirmó: **«Intro ok.»**

La confirmación procede de la prueba del usuario y se limita a esta intro. No representa una medición instrumental de cada plazo ni valida la integración con el juego completo.

## Empezar

1. Obtener el [ZIP original R04](https://chatgpt.com/library?file_id=libfile_51aacfacc41c8191a67f2943b27d5678&account_id=personal).
2. Desde `Intro/`, ejecutar `python3 tools/restore_assets.py /ruta/WF-WP0-R04-BIN-CUE-Y-FUENTES.zip`.
3. Configurar el SDK Yaul suministrado y `xorrisofs`, siguiendo [COMPILAR.md](COMPILAR.md).
4. Ejecutar `bash source/build_disc.sh /ruta/nueva/disco-r04`.

El ZIP externo contiene los recursos necesarios. El enlace requiere acceso a ese archivo en ChatGPT; no es una descarga pública ni permite que Codex acceda automáticamente a la cuenta. Si el entorno de compilación no tiene el ZIP, hay que proporcionárselo. El restaurador no descarga nada y no acepta otra revisión.

## Contenido de esta carpeta

| Ruta | Contenido |
|---|---|
| `source/` | 34 archivos de texto de las fuentes entregadas, conservados byte por byte: nueve unidades C de producción, headers, Makefile, herramientas y pruebas. |
| `tools/restore_assets.py` | Recupera ocho recursos del ZIP R04 y verifica tamaño y SHA-256 antes de escribir. |
| `ASSETS-MANIFEST.json` | Identidades del ZIP, seis recursos de compilación y dos recursos de pruebas. |
| `audit/` | Pruebas host, contraste del decodificador, revisión ASan/UBSan e informes seleccionados. |
| `docs/original/` | Documentos históricos de la entrega completa; algunas rutas citadas solo existen en el ZIP. |
| `VALIDACION_R04.txt` | Auditoría original sin modificaciones. |
| `COMPILAR.md` | Dependencias, restauración, compilación y comprobación del BIN/CUE. |
| `INTEGRACION_CODEX.md` | Alcance y puntos de integración con el port. |
| `PROCEDENCIA.json` | Identidad de la entrega y confirmación del usuario. |

La política del repositorio conserva los payloads gráficos y sonoros fuera de GitHub. El checkout incluye todo el código; necesita restaurar los recursos antes de fabricar el disco. `source/cd/A.BIN` se vuelve a compilar. BIOS, emulador, SDK y productos de compilación tampoco se suben.

## Arquitectura y audio

La Saturn dibuja mediante VDP1/VDP2. `SEQ.DAT` contiene cambios de mapas, paletas, valores de scroll y listas de sprites derivados de la secuencia arcade; no almacena fotogramas de un vídeo. Esta demostración reproduce esa secuencia exportada y no constituye por sí sola el controlador interactivo completo del frontend.

El audio es **PCM premezclado de 16 bits, mono, 44,1 kHz**, conservado con compresión Rice sin pérdida. El SH-2 descomprime y alimenta el búfer, y el SCSP lo reproduce. No se ejecuta síntesis de los canales arcade en tiempo real. Este WP0 R04 es independiente de las revisiones R04 del subsistema WP24.

La ROM original sigue siendo la autoridad. La integración posterior debe conservar sus datos, cadencia y límites; se realiza desde fuentes y nunca concatenando imágenes BIN/CUE.

## Evidencia y cronología

La entrega original comprobó dos reproducciones completas con reinicio, todos los campos visibles contra las referencias ROM, continuidad PCM, memoria y protección de plazos. Los informes distinguen medidas emuladas y observaciones físicas.

`VALIDACION_R04.txt` se escribió antes de que Ale respondiera «Intro ok.». Su frase final sobre la prueba física pendiente refleja aquel momento y se conserva como registro histórico. La confirmación posterior se registra en `PROCEDENCIA.json`.

Las verificaciones específicas de esta publicación se registran en `PUBLICACION_VERIFICADA.json`. `UPLOAD-MANIFEST.json` identifica los archivos publicados; no incluye su propio hash.

Esta aportación añade únicamente `Intro/` al repositorio. No modifica las otras escenas, los módulos existentes ni el estado general del port.
