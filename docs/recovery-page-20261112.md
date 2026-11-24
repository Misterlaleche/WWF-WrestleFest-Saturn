El objetivo sigue siendo cerrar la velocidad arcade de Royal sin cambiar píxeles, lógica, publicaciones, audio, input, IRQ, esperas ni el reloj 37609/39168. El entorno de ejecución quedó inaccesible el 12 de noviembre de 2026 mientras se medía la combinación nueva. La última versión aceptada y guardada es C24. C26 conserva las salidas nativas exactas, pero su velocidad todavía no está validada.

# Última versión aceptada

| Medida | C22 | C24 aceptado |
| --- | ---: | ---: |
| Ticks completos de Royal | 14.409 | 14.409 |
| Campos completos de Royal | 15.216 | 15.207 |
| Campos sobre el calendario nativo con esperas originales | 141 | 132 |
| Retraso correspondiente | 2,356829959 s | 2,206394004 s |
| Ventana común 8003–9993 | 2.163 campos | 2.163 campos |
| Velocidad en esa ventana común | 95,815587565 % | 95,815587565 % |
| TAG completo | 10.397 ticks / 10.833 campos | 10.397 ticks / 10.833 campos |

C24 ahorra nueve campos en el recorrido completo y no mejora la ventana tardía comparada. Ambas capturas terminaron con exit 0, terminal MATCH, strict logic/audio PASS y cero incidencias de audio o faults. El calendario nativo de Royal es 15.075 campos; es una referencia de ejecución con coste cero y esperas originales, no una medición de PCB. El objetivo de velocidad sigue abierto y no hay aceptación en Saturn físico.

# Checkpoint confirmado

[WrestleFest_Saturn_Royal_WIP_checkpoint.zip](https://chatgpt.com/library?file_id=libfile_56ce728f25c08191b73b5ed87320c2eb&account_id=personal)

Versión confirmada: **13**. Fuente principal incluida: `6ec7464a0bcaa7a34a8577f611c642b7faf133c8`. Tamaño: **277.726.939 bytes**. SHA256: `2ac732dfc34cc4c45fb112d58e31583aa64fe3bddaaa9112b2813f4872ddbf67`. Se verificó el ZIP completo: 2.246 miembros, sin error. La operación de guardado terminó con éxito y confirmó la versión 13.

Incluye la producción C24 aceptada y las capturas completas de C22/C24. No se ha confirmado que contenga los commits finales de C23 y C25, posteriores al inicio de ese checkpoint. Los dos parches exactos que siguen permiten recuperar la producción candidata sobre C24 aunque esos commits no estén en el bundle. Sus archivos de prueba y evidencia posteriores pueden seguir en el entorno inaccesible; deben comprobarse al recuperarlo.

# Combinación preparada antes de la caída

Fuente congelada C26: `5860f70d34b9328d5d836de800c440b4df5ce928`.

Worktree anterior: `/workspace/scratch/53b5fb345a0c/wf-joengine-royal/build/variants/c26-composed-royal`.

La combinación añade a C24 el empaquetado explícito de tablas C23 y el incremento XOR C25. Sólo cambia `port/zlib/inftrees.c` respecto a C24. Antes de iniciar la captura se completaron, con exit 0:

- Una ejecución nativa nueva de Royal y TAG: **126 + 102 archivos, todos idénticos byte a byte a C22**.

- La construcción normal Royal MODE2/frontend/capture, NTSC, sin profiler ni timeline.

- La comprobación de **452 inputs** y **35 payloads ISO/BIN**; 34 assets inmutables y `cd/0.bin` privado.

- La procedencia de 152 objetos privados C24 Royal sin cambios; `inftrees.c` se recompiló.

`_end = 0x060dbc60`, con **928 bytes libres**. Estos resultados confirman equivalencia y construcción; no confirman una ganancia de velocidad.

| Archivo C26 Royal congelado | SHA256 |
| --- | --- |
| game.elf | `70bfcbf68d5709d32a2b32447c913ee235d726f30feb3d600ffbc298eb8721b2` |
| game.raw | `d9249293b921a90f07811295e4e07cd1fe72f79a90a6bdf971834b402ce17406` |
| game.bin | `19d317067ed59557666824090c40367ebe406f5e6ede48f519b2304efb01ba6f` |
| game.cue | `a963ecf449a707428122f1b2d06917b37334f1f523c2512b2da5b508b61424c8` |

# Procesos con resultado pendiente

Royal C26 se lanzó en la sesión **71726**. El último progreso leído fue a 80,035795 segundos de ejecución, tick 645 y campo 4.099, sin incidencias de audio. No se observó un recibo final de captura ni se ejecutó su análisis completo. No usar ese fragmento para afirmar velocidad.

TAG C26 quedó en `/workspace/scratch/53b5fb345a0c/wf-joengine-royal/build/variants/c26-composed-tag`, rama `royal-c26-composed-tag`, base `5860f70`. Se verificaron los 452 inputs, 34 assets y 153 objetos privados heredados exclusivamente de C24 TAG MODE1. El build se lanzó en sesión **89320**; no se observó su terminación ni se ejecutó el inventario/freeze posterior. No hay una imagen TAG C26 declarada lista.

El intento de crear el archivo de evidencia de construcción C26 recibió un fallo de creación de proceso por desconexión del servidor. No existe confirmación de archivo ni commit de esa operación. Al recuperar acceso, comprobar el estado real antes de reintentar.

La rama principal local quedó en `3263565` tras preservar los paquetes de evidencia C23/C25; su producción seguía siendo C24. Esos dos paquetes se añadieron después del checkpoint confirmado.

# Commits y pruebas de los cambios nuevos

| Entrega | Commit |
| --- | --- |
| C23 fuente y fixture independiente | `ffc8599d0c8982fd8e9a9ec49a36528590fb60a2` |
| C23 evidencia | `f1228723e65c7e17dcfa23177f14436d28333962` |
| C25 incremento XOR | `29ca60e7768248028988f783bcc8035dc4c22db3` |
| C25 evidencia | `23942ad8999314b28359a01cddc56700ea671417` |

C23 pasó cuatro configuraciones de **80.688 tablas** cada una, ocho controles negativos semánticos, assets con ASan/UBSan, los cuatro schedules de worker y las 228 salidas nativas. El explícito final conserva las mismas secciones cargadas y el mismo constructor SH2 del probe auditado. Las primeras variantes union fueron rechazadas por su coste; no deben recuperarse como candidatas.

C25 pasó de nuevo las cuatro configuraciones de 80.688 tablas y ocho negativos, con ASan/UBSan y strict aliasing. La prueba independiente de los **65.534 pares legales** de longitud/código confirma el mismo huff, incr y número de iteraciones. Root y HUD aprobaron el alcance, la álgebra y las pruebas. Los conteos completos de instrucciones fueron favorables, pero no son una medición de ciclos ni ganancias sumables.

Runner: `tests/test_inftrees_store.py`; referencia: `tests/native/reference_c23_inftrees/`. El worktree de esas pruebas quedó en `build/variants/c23-table-store`, commit `23942ad`.

# Recuperación exacta del constructor

Sobre el `port/zlib/inftrees.c` de C24, aplicar primero el parche C23 y después el parche C25. El archivo inicial tiene SHA256 `1402d46f971ea81032b020aea64dec36268d90ac92832122dbf809dcb80202bf`.

Después de C23, el SHA256 esperado es `01514df58d229f68e9116cbb0e94895c4ced868d396c662b98c06b36ab992489`.

Después de ambos, el SHA256 esperado es **`854ed3593f7f2c748ac18efcced3f61f751ddd9d6c256bedd8f59d7f5e95fa10`**. No sustituir el archivo por el probe explícito antiguo: carece de los guards y del fallback para macros de endian ausentes.

## Parche C23

```diff
diff --git a/port/zlib/inftrees.c b/port/zlib/inftrees.c
--- a/port/zlib/inftrees.c
+++ b/port/zlib/inftrees.c
@@ -7,6 +7,25 @@
 #include "inftrees.h"
 #include "wf_sh2_bits.h"
 
+/* Each private table entry is an aligned four-byte object. Preserve its
+ * native representation while storing each replica once; SH-2 otherwise
+ * repeats three member stores and a stack load for every copied entry. */
+static inline __attribute__((always_inline)) unsigned
+wf_table_pack(code entry)
+{
+#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
+    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
+    return ((unsigned)entry.op << 24) | ((unsigned)entry.bits << 16) | entry.val;
+#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
+      __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
+    return entry.op | ((unsigned)entry.bits << 8) | ((unsigned)entry.val << 16);
+#else
+    union { unsigned word; code entry; } packed;
+    packed.entry = entry;
+    return packed.word;
+#endif
+}
+
 #define MAXBITS 15
 
 const char inflate_copyright[] =
@@ -48,6 +67,7 @@
     unsigned used;              /* code entries in table used */
     unsigned huff;              /* Huffman code */
     unsigned table_index;      /* hoisted code index for replication */
+    unsigned packed;           /* complete native representation of here */
     unsigned incr;              /* for incrementing code, index */
     unsigned fill;              /* index for replicating entries */
     unsigned low;               /* low bits for current root entry */
@@ -234,9 +254,10 @@
         fill = wf_zlib_power[curr];
         min = fill;                 /* save offset to next table */
         table_index = (unsigned)WF_ZLIB_DROP(huff,drop);
+        packed = wf_table_pack(here);
         do {
             fill -= incr;
-            next[table_index + fill] = here;
+            *(volatile wf_zlib_code_word *)(void *)&next[table_index + fill] = packed;
         } while (fill != 0);
 
         /* backwards increment the len-bit code huff */
```

## Parche C25

```diff
--- a/port/zlib/inftrees.c
+++ b/port/zlib/inftrees.c
@@ -262,14 +262,11 @@
 
         /* backwards increment the len-bit code huff */
         incr = wf_zlib_power[len - 1];
-        while (huff & incr)
+        while (huff & incr) {
+            huff ^= incr;
             incr >>= 1;
-        if (incr != 0) {
-            huff &= incr - 1;
-            huff += incr;
-        }
-        else
-            huff = 0;
+        }
+        huff ^= incr;
 
         /* go to next symbol, update count, len */
         sym++;
```

# Punto de reanudación

1. Recuperar el entorno anterior si vuelve a estar disponible. Leer los recibos y salidas reales de las sesiones pendientes antes de asumir que una captura terminó o repetirla.

2. Si hace falta reconstruir, partir del checkpoint C24 confirmado; recuperar los commits experimentales disponibles o aplicar los dos parches y comprobar sus hashes. Mantener intactos el reloj, las esperas, todas las publicaciones y los datos de disco bloqueados.

3. Comprobar los informes de C26 que ya terminaron: `build/composition-build-exit.json`, `build/composition-native-exit.json`, `build/composition-native-comparison.json`, `build/composition-verification.json` y `build/frozen-identity.json`.

4. Completar Royal C26 con una sola instancia de Mednafen. Analizar sólo después de exit 0 y recibo final. Exigir terminal MATCH, strict logic/audio PASS y cero incidencias. Comparar la ventana tardía únicamente en ticks observados comunes contra C24; no interpolar.

5. Si Royal mejora, terminar y verificar la imagen TAG C26 y ejecutar su captura completa. Integrar los bytes exactos medidos sólo tras esos resultados. Conservar también cualquier regresión o fallo.

6. Preservar los paquetes completos de C23/C25 y las nuevas capturas en el siguiente checkpoint, usando la versión actual 13 como guard de reemplazo mientras siga vigente.

Mantener una sola instancia del emulador y consultar el proceso activo periódicamente. No leer ni archivar outputs de un proceso que sigue produciéndolos.

## Comandos de análisis ya usados

```bash
python3 tools/analyze_royal_performance.py capture-full \
  --native /workspace/scratch/53b5fb345a0c/wf-joengine-royal/evidence/royal-recovery/c09-native \
  --output capture-full/analysis.json --require-terminal

python3 tools/compare_capture_windows.py \
  --before ../c24-inflate-refill/capture-full \
  --after capture-full --from-tick 8000 --to-tick 10000 \
  --output capture-full/window-late.json
```

# Investigaciones para no repetir

**C27, tabla inversa de potencias:** rechazado antes de target. La ruta literal completa sigue en 32 instrucciones; frame 88→100 bytes, texto +72 y rodata +128. Evidencia integrada en `6281289` y en el checkpoint confirmado.

**C28, Adler32 agrupado:** probe enlazado completado, sin promoción. Por 4 KiB: 18.001→17.247 instrucciones alineado, pero 18.783 no alineado; añade 768 lecturas y 256 escrituras de stack, aumenta función 700→1.064 bytes y frame 12→32. El margen es pequeño y no se midió velocidad. El intento posterior de generar el informe perdió acceso; no dar ese informe por creado. Conservar el probe y ASM ya completados si se recuperan.

**ARC para CPU cache:** sólo se revisaron las trazas y se fijó una familia; ningún replay nuevo llegó a ejecutarse. Configuraciones previstas: ARC entero con p inicial 0, ghosts 138 por solicitud; promoción entre batches con ghosts 138; y esa variante con ghosts 96. Los 49 words libres de slot_age equivalen a 196 bytes; 96 IDs consumen 192 bytes y todavía falta justificar contadores, memoria y coste de instrucciones. Comparar Royal/TAG completos y picos locales contra SLRU16 bajo el mismo warm start neutral; no usar sólo los misses ni confundir ese inicio con el historial real desde bind.

**C29, join privado sin purga total del master:** sólo lectura, sin aprobación para implementar. Todos los resultados del worker, incluido stage, requerirían lecturas cache-through; mantener el wait público con purga y toda la semántica de sincronización. Falta cerrar las escrituras de los jobs VDP1 y su consumo por el master. Los tests host actuales no modelan caché stale; harían falta mocks de MMIO/CCR y vistas cached/uncached con controles negativos. No hay una ganancia medida.
