# CODEX-INTEGRATION — Audio 2

## Estado y alcance

API pública versión1; candidato nativo en validación. No integrar como audio final ni inferir aprobación por el nombre de la carpeta/rama. Consultar `AUDIO-FINAL-AUDIT.md` y los resultados concretos del ejecutable que se utilice. El diagnóstico estático ya ha arrancado en Mednafen1.32.1; la integración dinámica y el rendimiento del juego tienen comprobaciones independientes.

La unidad de entrega es esta carpeta. Ningún paso necesita recuperar código musical de documentación antigua. Los recursos protegidos se generan desde `wwfwfest.zip` del propietario. El secuenciador no admite una ROM parecida: verifica la identidad del controlador usado por sus optimizaciones.

## API que debe usar el juego

Incluir `include/wf_audio2.h`; compilar con los directorios `include/` y `src/` en la búsqueda de cabeceras. El ejecutable de prueba completo está en `tests/saturn_harness.c` y muestra los callbacks reales de Yaul.

| Función | Contrato |
|---|---|
| `wf_audio2_init(context, config)` | Inicialización fría, antes de habilitar productores. Valida recursos/ventanas, arranca 68000, carga SFX, lanza el worker y devuelve STARTING. Puede esperar durante SMPC/arranque; nunca llamarla desde combate o VBlank. |
| `wf_audio2_command(context, original_word)` | Publica un comando original sin esperar su ACK. Usa el byte bajo; conserva la palabra completa para SFX/diagnóstico. Devuelve ACCEPTED, BUSY, NOT_READY, UNSUPPORTED o FAILED. |
| `wf_audio2_service(context)` | Trabajo acotado en el contexto normal del master: ACK, errores, inicio/parada y watchdog. No interpreta Z80 ni sintetiza audio. |
| `wf_audio2_vblank(context, video_frame)` | Única operación de audio permitida en VBlank: publicar el contador de vídeo de16bits. No copiar datos ni esperar SMPC. |
| `wf_audio2_status(context, out)` | Copia el último estado observado. Leer después de `service` para observar cambios. |
| `wf_audio2_stop(context)` | Solicita desmontaje del sistema y salida del slave. Es idempotente durante STOPPING. Continuar `service` hasta STOPPED o FAULT. |
| `wf_audio2_recover(context)` | Reinicio frío tras comprobar que el worker realmente volvió. No reanuda una cola o línea musical que perdió integridad. |
| `wf_audio2_resource_valid(pack, bytes)` | Validación estructural y CRC32 de todo el payload SFX antes de escribir hardware. |

`ACCEPTED` significa publicado en el transporte; no significa que un efecto haya ganado la prioridad de voz. `WF_NATIVE_PRIORITY_REJECTED` es una decisión de la lógica original, se contabiliza separadamente y no equivale a un error de transporte.

`BUSY` significa **cero efectos laterales de ese intento**. El productor/concentrador debe conservar el comando y reintentarlo en orden. No convertir BUSY en éxito, descartar silenciosamente, duplicar el productor ni ampliar la cola para ocultar presión. El límite histórico del FIFO musical sigue siendo64. La frontera del runtime puede conservar su cola original de64 comandos para reunir productores del mismo paso de lógica.

Los comandos permitidos por esta API son `00`, `01..12`, `1F`, `20..81`. El `1F` se entrega al controlador original, que decide la curva y duración del fade. `stop()` es desmontaje de infraestructura, no una implementación alternativa del fade.

## Lugar y orden de integración

La frontera existente es `wf_r18_scsp_boot`, `wf_r18_scsp_vblank`, `wf_r18_scsp_service` y `wf_r18_scsp_audio`; en el runtime auditado, `gameplay.audio_command` se asigna al último callback. Sustituir únicamente esa frontera/backend de audio en una copia aislada. Conservar todas las llamadas y propietarios del gameplay.

1. Reservar explícitamente Sound RAM, el slave SH-2, la ventana caliente y su pila. Comprobar el mapa del master antes de leer recursos.
2. Leer firmware y recursos con el cargador normal, antes de que gráficos reutilice el área de staging. No hacer E/S de CD en callbacks SMPC ni en el bucle de muestras.
3. Construir `wf_audio2_config_t` con callbacks del reloj monotónico, barrera de bus y propietario SMPC. El arbitraje debe impedir que un INTBACK compita con SNDOFF/SNDON. El ejemplo usa FRT y el adaptador `wf_native_yaul_complete_platform`.
4. Inicializar el contexto a cero una vez. Llamar `init` y `service` hasta RUNNING, comprobando ambos estados, los errores y la identidad de perfil nativo `0x4D46`.
5. Habilitar los productores existentes. Cada comando entra exactamente una vez por `command`; servicio en contexto normal, publicación de tick en el hook VBlank ya existente.
6. Antes de devolver el slave o su RAM a otra escena, solicitar `stop`, esperar su salida real y después liberar los recursos. Nunca interpretar `cpu_dual_slave_clear()` como confirmación de salida: sólo elimina el callback futuro.

El adaptador estático `wf_audio2_yaul_worker_start` purga el rango caliente, entra mediante el trampoline no cacheado y publica un indicador de salida no cacheado **después** de retornar del worker. Una excepción o bloqueo real puede impedir esa salida; en ese caso `recover` falla de forma segura y se necesita reinicio frío de plataforma.

La configuración y los recursos apuntados deben seguir existiendo mientras puedan usarse para recuperación. Si el perfil de integración reutiliza el staging para gráficos después del arranque, su contrato debe prohibir `recover` con esos punteros y realizar la recarga completa en un reinicio de escena.

## Propiedad ROM-EXACT

El motor implementa el significado del comando; no demuestra su productor de gameplay. Mantener intactos `docs/AUDIO-PRODUCER-CLEANR01-OWNER-MAP.md`, R04 y las restricciones posteriores de PROJECT-STATE. Las filas OWNER_SCOPE_ONLY/UNRESOLVED/REGISTERED_NOT_IDENTIFIED continúan bloqueadas.

La matriz histórica inicial contiene173 filas,14 listas,4 de alcance y155 sin resolver; PROJECT-STATE incorpora revisiones posteriores con6 cierres y149 sin resolver. No fusionar esos estados ni elevar una fila por reconocer una voz al escucharla. En particular, `0x0263CC → 0x0B` no se convierte en el productor de baja resistencia; el productor demostrado es `0x009088 → 0x0B`.

No añadir audio desde render, colisiones gráficas o animaciones ya cubiertas por el propietario de gameplay. Registrar dirección/owner/comando en la frontera permite comprobar duplicados sin alterar la ROM.

## Memoria, ABI y recursos de CPU

El driver usa Sound RAM interna de512KiB; no necesita expansión de4MiB. El perfil estático de prueba embebe sus recursos para un disco autónomo y **no** representa su huella residente de integración. Los ELF del perfil estático no son módulos ABI3/ABI4 intercambiables.

El protocolo nativo conserva mailbox ABI3 de32bytes, con publicaciones de16bits. El compartido musical histórico conserva176bytes y `workspace` en offset172. La API pública versión1 es una frontera distinta de las versiones del contenedor ELF. Una coincidencia del nombre de archivo o de un entero ABI no autoriza mezclar master, driver, linker o módulo.

Ver `docs/MEMORY-AND-ABI.md` para las ventanas y validaciones. El camino caliente consta de código más estado3220bytes y debe caber en18432bytes. El módulo necesita propiedad exclusiva del slave; no se debe introducir junto a un trabajo gráfico que ya lo posea. El estado actual no acredita que toda esa CPU esté disponible para el combate.

La simulación del arcade ronda57,44 actualizaciones/s. El refresco NTSC de Saturn y el contador VBlank son relojes diferentes. El watchdog usa VBlank para detectar inmovilidad; el audio usa el contador DSP de44100Hz y ciclos originales Z80. No sustituir el paso de lógica del juego por60Hz para integrar audio.

## Compilación reproducible

Los ejemplos usan salidas privadas fuera del repositorio y el SDK suministrado. Ajustar sólo ubicaciones:

```bash
python tools/environment/restore_environment.py --out "$WF_AUDIO_ENV" \
  --sdk-archive /ruta/13-sh2eb-elf-yaul-1-.zip --build
source "$WF_AUDIO_ENV/env.sh"
python tools/generate_resources.py --rom-zip /ruta/wwfwfest.zip --out "$WF_AUDIO_BUILD/resources"
python tools/build_firmware.py --out "$WF_AUDIO_BUILD/firmware" \
  --runtime-source "$WF_AUDIO_ENV/src/gcc-lb1sf68.S" --no-timing
python tools/build_saturn.py --out "$WF_AUDIO_BUILD/music01" \
  --resources "$WF_AUDIO_BUILD/resources" --firmware "$WF_AUDIO_BUILD/firmware/SCSP.ELF" \
  --sdk "$WF_AUDIO_ENV/sh2eb-elf" --command 0x01 --seconds 35
python tools/run_host_tests.py --rom-zip /ruta/wwfwfest.zip \
  --resources "$WF_AUDIO_BUILD/resources" --firmware "$WF_AUDIO_BUILD/firmware/SCSP.ELF" \
  --out "$WF_AUDIO_BUILD/host-tests" --seconds 600
python tools/reference/reproduce_all_references.py --help
python tools/run_native_test.py --build "$WF_AUDIO_BUILD/music01" \
  --environment-root "$WF_AUDIO_ENV" --bios /ruta/mpr-17933.bin \
  --out "$WF_AUDIO_BUILD/captures/music01" --display 86 --wall-seconds 90
```

Definir `WF_AUDIO_ENV` y `WF_AUDIO_BUILD` como directorios absolutos propios antes de ejecutar. La instalación no descarga BIOS ni ROM. El runtime GCC68000 se recompila desde fuente fijada para evitar objetos de otra CPU. `--no-timing` retira sólo observación privada; no desactiva contadores ni guardas de fallos.

El constructor rechaza rutas de entrada/salida coincidentes, incluso enlaces duros. La cabecera de disco conserva el orden regional requerido por los bloques del SDK; Mednafen se configura explícitamente en NTSC-U. Cualquier ELF que incluya SOUND.ROM o SFX.DAT es privado.

## Errores que deben verse

Los errores de FM tardío, FIFO lleno, transporte lleno y key tardío son15/16/17/18 en el worker. El firmware informa `0xB040` para su plazo duro perdido. Se conservan el primer error y los contadores de eventos descartados/rechazados; B03A por sí solo cuenta lotes, no notas.

La API diferencia fallo de worker, fallo nativo, timeout de mailbox, timeout de inicio/parada, protocolo y estancamiento de worker/DSP. El watchdog usa120 VBlanks con comparación modular y una lectura ordenada de los stamps. Tras FAULT se solicita salida, y ninguna nueva configuración puede eludir la comprobación del worker anterior.

Guardar estado y contadores antes de recuperar: una preparación nueva reinicia las métricas de la nueva generación. No volver a declarar RUNNING para continuar una secuencia que ya perdió datos.
