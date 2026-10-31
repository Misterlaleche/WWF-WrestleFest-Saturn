# Recuperación del checkpoint05

Integración del estado original desde RESET hasta la entrada de combate F8C.
No es una beta completa ni un nuevo disco arrancable. El último disco visual
validado sigue siendo checkpoint04, que termina al confirmar un luchador.

## Cambio de producción

`wf_session` conserva una sola RAM de16KiB, registros D/A, contexto de control
y vídeo sombra entre frontend y preparación de combate. Comienza en el RESET
original654, deja que moneda/START/modo/roster produzcan el estadoAC0 y conecta
los propietarios nativos de preparación hasta F8C. Sólo entonces importa los
actores y globals al runtime tipado. No rellena a mano participantes, créditos,
semilla ni equipos. El llamador proporciona explícitamente RAM inicial y bus,
esperas, envíos de vídeo y eventos de plataforma.

Se enlazan las fuentes exactas de HW1 bajo `modo Royal/wp1980/work/hwfix01/native`.
Esta versión conserva la corrección de esperas breves que falta en R3. Las
extensiones98BA/8EB6/285DA usan sus implementaciones originales y los mismos
buffers/registros. Los comandos de sonido siguen pasando por el bus; el nuevo
adaptador no añade reproducción de audio.

START acreditado durante attract invalida el cursor anterior por generación
y devuelve el control al frontend conservando el nuevo contexto. Un fallo
de dependencia deja la sesión en FATAL; no se reejecuta un prefijo parcial.
Preparar match antes de AC0 se rechaza. El handoff no ejecuta todavía frames
de gameplay ni conecta las animaciones visibles de Entrada/Presentadora.

## Reproducción host

Desde production/full_game, con compilador C11 y Python3:

```sh
WF_ROM=/ruta/wwfwfest.zip make test sanitize
python3 tools/run_session_checks.py --rom /ruta/wwfwfest.zip --sanitize --out /ruta/nueva/prueba
```

Sin WF_ROM la suite ejecuta las pruebas de contratos de sesión y omite
explícitamente los recorridos que necesitan ROM. Cada ejecución usa una carpeta
nueva, impresa al terminar. No escribe en las referencias. El bus del harness
controla entradas/IRQ/niveles de pantalla, registra MMIO, utiliza vídeo sombra
y recibe los registros originales de sprites. Los pulsos de mando esperan que
el propietario de input observe la liberación antes de volver a pulsar.

Los recorridos incluyen TAG normal, TAG desdeP2, Demolition desdeP1 yP2,
ROYAL Hogan/Crush, START durante attract, otra semilla explícita, espera que
entrega tres eventos y fallo de recurso durante match. DemolitionP2 conserva
el orden original11/10 después de la normalización de actores. El fallo
esperado se produce en7BF8/26E66, sin importar actores ni volver a ejecutar.

Para el diferencial directo de crédito/START (Unicorn2.1.4):

```sh
python tools/run_start_oracle.py --rom /ruta/wwfwfest.zip --out /ruta/nueva/oracle
```

408 casos comparan RAM persistente por debajo de3AD4, D0..D7, A0..A6 y carry
contra las instrucciones68000 originales055A/0978. Incluyen los cuatro puertos,
créditos insuficientes, configuración dedicada y patrones de registros/RAM.
La captura de carry ejecuta MOVE SR,D7 tras el retorno: no depende de la
representación diferida de CCR de Unicorn. Los snapshots de pila temporal no
se comparan. Este oracle no demuestra equivalencia ROM de todo el recorrido.

## Núcleo SH-2

Con SDK Yaul/GCC13.2.0 suministrado, en Linux:

```sh
YAUL_INSTALL_ROOT=/sdk python3 tools/build_session.py --sh2 --out /ruta/nueva/sh2
```

El script verifica los2023 archivos de referencia, compila los propietarios,
enlaza de forma relocatable desde las tres funciones públicas de sesión y
comprueba dependencias pendientes. Se eliminan símbolos sin referencias que
GNU ld conserva de secciones descartadas, manteniendo explícitamente la API.
Sólo quedan `memcpy` y `memset`; no se admiten propietarios ausentes. El archivo
generado reemplaza por completo cualquier archivo anterior del mismo build,
evitando miembros obsoletos. Los checkpoints congelados permanecen separados.

Archivo `libwf_session.a`:183464B, SHA256
`86f9259f148d56f9f326c5111ebf3a9bd02da68a5d3f2e69d2744f54bf66a452`.
No contiene startup, main, backend completo Saturn ni un disco. Flags, fuentes
y hashes están en evidence/session05/sh2-build.json. SDK y Docker siguen la
configuración documentada en RECOVERY-04.md; compilación sin red.

## Límites y continuación exacta

Las semillas1085/305419896, RAM inicial y calendario de eventos del harness
son fixtures. El adaptador exige una entrada de arranque explícita; todavía
falta cerrar cómo la Saturn proporciona ese estado y los IRQ de placa con el
reloj lógico compartido. No se convierte una semilla de test en política real.

La revisión independiente encontró una expectativa incorrecta para DemolitionP2
y la verificación de símbolos descartados; las correcciones y resultados
quedan registrados en PROGRESS.md. No hay cambios de selección en los módulos
originales para acomodar los tests. Los ZIP contienen fuentes, núcleo y
evidencias; excluyen ROM/BIOS/recursos protegidos externos y nunca renombran
un disco04 como disco05.

Siguiente: conectar el bus/eventos de esta sesión al propietario Saturn del04,
transportar las paletas/compañero y arte Royal correctos, integrar la Intro
R04 con su parada VDP1 y las escenas aprobadas. Luego presupuesto dinámico
estándar/4MiB, gameplay/progresión y todas las pruebas del encargo. Sigue
pendiente el port completo, su rendimiento y la validación física.
