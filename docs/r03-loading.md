# R03: carga inicial desde CD

La imagen manual R03 reduce trabajo del lector durante el arranque. No se ha
demostrado todavía la causa completa de los seis minutos observados en la
Saturn con CD grabado y Pseudo Saturn Kai. El usuario confirmó que sólo había
probado R01 físicamente; R02 no había sido probada.

## Cambios

- Las consultas de tamaño de cuatro formatos WF conocidos usan el índice ISO.
  El opt-in `WF_RAW_DISC_ASSETS=1` exige recursos crudos verificados; la API
  genérica y el modo sin opt-in conservan el comportamiento JoLZ.
- Una tabla de 864 bytes contiene las cabeceras y tamaños de las 18 músicas.
  El SDK reutiliza su parser y obtiene FAD/tamaño del índice ISO. Los hashes
  completos de los WAV y la tabla se verifican antes de compilar/empaquetar.
- `cd.sort` coloca programa, ROM, sprites, escenas, SFX y fuente en extents
  consecutivos. El orden relativo de los WAV permanece igual.
- Un diagnóstico de 64 bytes registra inicios, errores, reintentos y máximos
  de lectura. La pantalla lo muestra durante la carga y en el fallo restaurado.
  No imprime desde el lector ni después de ceder los mapas a la imagen del juego.

Los 34 recursos conservan sus bytes respecto a E02. No se integra el fundido
musical privado ni se cambian PCM, reglas, reservas de RAM o límites de reintento.

## Comprobaciones

La misma BIOS y Mednafen stock, con arranque frío y sin escrituras del observador,
dan 1.486 campos desde el callback de carga hasta recursos preparados en la
referencia, y 1.127 en R03: 359 menos, un 24,16 %. Son aproximadamente 25 y
19 segundos de ese tramo emulado, sin incluir todo el arranque de la BIOS.
Hay una captura por versión; no es una medición física del CD-R.

Las estadísticas conservadas registran 172.032 bytes menos de sectores
solicitados por la ruta síncrona: 135.168 de las consultas y 36.864 de cabeceras.
No representan todo lo que el mecanismo pueda leer por anticipación. R03
registra 139 lecturas públicas y cinco inicios de rango, incluidos los de
montaje, sin errores ni reintentos. Los comandos asíncronos de música tienen
sus propios contadores.

El binario manual enlaza con `_end=060DBA24`, 1.500 bytes antes de la reserva
fija `060DC000`; JoWork y la pila conservan sus límites. Ese margen estático
no representa memoria libre total. Los 35 archivos, programa, CUE y los
309.357 sectores MODE1/2352 pasan la comprobación completa, con siete
controles negativos del verificador EDC/ECC.

La imagen exacta pasa seis demos de attract, retorno al título y entrada
acreditada en TAG con más de 300 ticks accionados mediante teclas normales.
La prueba PAL entra en Royal y también supera 300 ticks. Se inspeccionaron
imágenes de carga, título y ambos combates. No se observaron fallos de
runtime, SMPC, acceso nulo, CD o audio en esas pruebas. La tabla de 18 músicas
coincide con los extents finales; los 114.688 bytes de los dos prefijos
coinciden con el ISO en la RAM de cartucho. TAG recupera los 148 bytes del
récord guardado y conserva la batería de origen.

El runner `tests/run_initial_load_tests.py --source . --assets cd --output
build/initial-load-tests --rom-oracle` reúne las 19 puertas host verificadas.
La opción de oráculo requiere `machine68k`. Incluye formato, errores, orden,
protocolo CD, textos ocultos, PCM y controles negativos; no sustituye al hardware.

Los recibos compactos están en
[evidence/initial-load-r03-20261113](../evidence/initial-load-r03-20261113/manifest.json).
BIN SHA-256: `991ed0c8bfe97d1a436fa237406fffc538c4434c70e4767b1936953b2603686a`.

## Pendiente

La fluidez arcade completa de Royal y la aceptación física siguen abiertas.
Las rutas completas de TAG y Royal previamente documentadas pertenecen a sus
compilaciones identificadas, no a un nuevo recorrido completo de R03. Una
franja blanca inferior durante la carga aparece también en la referencia
emulada; se investiga separadamente y no se atribuye aún al grabador ni al
hardware. El resto de comprobaciones gráficas muestreadas tampoco es exhaustivo.
