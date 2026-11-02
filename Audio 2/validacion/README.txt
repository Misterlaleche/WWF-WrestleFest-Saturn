VALIDACION LOCAL DE18 PARTITURAS / SFX / GRAFICOS
Estado2026-10-29: HARNESS PREPARADO; SUITE18 TODAVIA NO COMPILADA NI EMULADA.
Hay pruebas HOST del propio tooling, lectura de un estado real y compilacion
68000 de los offsets PCM. Esto no sustituye los36arranques futuros de la suite.
No es un certificado de reparacion. El candidato nativo aun debe pasar su
prueba fria de plazos antes de autorizar estas ejecuciones largas.

DECISION
Usar18 copias de MUSIC.ELF con un master fijo, no una cola nueva en el master.
Cada copia congela builder, secuenciador, ROM/lifts y synth del candidato final;
solo cambia wf_music_module.c. No se modifican produccion, R04 o medios.
El VFS de cold_overlay.py abre el disco original rb y crea sectores solo RAM.
No hay generador de BIN/CUE/ISO, state loads, escrituras guest ni mute.

MODOS
remap: solo redirige comandos1..18 a la pista del caso. STOP0 y fade1F siguen
su ruta original. El master puede truncar una pista; una generacion corta NO
permite aprobar su cobertura completa.
hold-first: TEST ONLY. Redirige el primer comando musical a la pista, luego
consume comandos musicales/STOP/fade durante el horizonte y cuenta cada clase
suprimida en globals wf_test_*. Al terminar, vuelve al despacho original con
remapeo musical. Esto NO prueba semantica STOP/fade. Se necesita ademas un caso
remap normal y las pruebas host R01 de18x600s +fade/wrap ya existentes.

DURACIONES
Derivadas de catalogue180/template-analysis.json + ciclo del handler de prefijo
en handlers.csv, con margen max(10%,5s). Los bucles de fanfarrias inactivas NO
se interpretan como musica.04 usa el primer RAM repeat de long-4/handlers.csv.
01=101s,02=75s,03=75s,04=221s,05=30s,06=75s,07=75s,08=30s,09=30s,
0A=95s,0B=605s,0C=605s,0D=75s,0E=75s,0F=86s,10=75s,11=605s,12=75s.
0B/0C/11 exigen600s reales de ciclos Z80 observados; hold605s da margen de
captura. Son observaciones finitas, sin afirmar cierre de bucle. La evidencia
host de1200s no encontro repeticion RAM completa en esas tres.
Los restantes loops exigen intro+un periodo observado, no duracion infinita.
Frames de cada perfil: max(9000,4200+hold_seconds*60+120). El minimo9000 permite
intentar alcanzar el combate300..600 aun para fanfarrias. Si no se llega, falla.
Con36 arranques emparejados, es una bateria larga; ejecutar primero una pista
exigente y su control SFX despues del green de rendimiento.

REPRODUCCION (Python con -B; comandos FUTUROS, no ejecutados aqui)
1. track_suite.py prepare --repo FINAL_REPO_AISLADO --out NUEVA_SUITE
   --mode hold-first
   Congela fuentes y crea18 overrides+case.json+prepared.json. No compila.
2. track_suite.py build --suite NUEVA_SUITE --toolchain SH2_TOOLCHAIN
   --sdk SH2_SDK --rom SOUND_ROM_COMPLETA
   Usa el build_music_module.py congelado y compila native_layout.c a ensamblador
   para obtener offsets ABI del compilador. NO reconstruye el master.
   PRECONDICION: el linker FINAL de ese repo debe apuntar al workspace HWRAM
   real del master pinneado. El build falla si DEBUG ELF conserva direcciones
   de transporte VDP1. El relink HWRAM del builder experimental debe resolverse
   antes; no se ha probado aqui la compilacion ni su integracion remota.
3. track_suite.py plan --suite NUEVA_SUITE --runtime MASTER_ELF
   --scsp MUSIC_SCSP_ELF --sfx LEGACY_PROFILE_SCSP_ELF --cue ORIGINAL_CUE
   --scsp-debug MUSIC_SCSP_DEBUG_ELF --scsp-layout MUSIC_PCM_LAYOUT_JSON
   --sfx-debug SFX_DEBUG_ELF --sfx-layout SFX_PCM_LAYOUT_JSON
   --core BEETLE_DLL --input tag --wav
   Escribe execution-plan.json. Fija hashes master/SCSP/core/CUE/disco, conserva
   el mismo MUSIC por par y prepara comandos del observer. No carga emulador.
4. SOLO TRAS AUTORIZACION ROOT:
   track_suite.py run --suite NUEVA_SUITE --tracks 1 --execute
   Quitar --tracks para las18. Cada salida debe ser nueva; nunca sobreescribe.
   El entorno debe mantener BIOS iguales. El plan fija tambien todos los
   fuentes congelados, casos, overrides, ELF y DEBUG, layouts y observadores.
   Si falta o cambia uno, se rechaza antes de crear salidas o cargar el core.
   Los planes antiguos que no contienen esas identidades quedan rechazados.
5. assess_tracks.py --suite NUEVA_SUITE
   Salida0 solo si los18 pares pasan; ausencia de runs produce INCOMPLETE.

OBSERVACION Y ESQUEMA
observe_cold.py ejecuta en memoria una extension con anclas verificadas del
cold_overlay.py existente. Reutiliza VFS, opciones full/exact/JIT disabled e
inputs originales. Cada60frames serializa solo para LEER RAM, firmware y DSP.
Mide callbacks de audio sin alterar muestras. --wav copia TODO callback stereo
16bit y obtiene sample_rate de retro_get_system_av_info; no silencia el guest.
report.json conserva formato original y anade validation_observer y wav_capture.
Cada observation anade generation/ack, FIFO, eventos, late/deadline counters,
mascara FM, mailbox SFX aceptado, test.*, sequence_active/fade/cycles y
score_elapsed_seconds. La identidad de hotcode compara TODOS sus bytes HWRAM
con DEBUG ELF; la identidad de transporte usa las muestras del observer base.
counter-trace.json anade scene_sha256, frame_stage y frame_ready.
Los simbolos privados .lto_priv.N obtienen alias solo cuando el nombre es
unico; los ambiguos se conservan sin alias y nunca sustituyen uno publico.
La identidad de cargas ejecutables distingue SCSP/VDP1/HWRAM para ABI4.
La huella cubre globals+todos los objetos hasta m68k_regs; excluye callbacks y
contexto de registros. Solo en frontera scheduler stage1/frame_ready1; se usa
la ultima muestra por game_frame. Se enmascaran handshake y enable de VBlank.

GATES
Exige perfil nativo correcto, MCU READY, musica RUNNING tras iniciar, codigo
entregado correcto, generacion no cero/ack, progreso eventos y mascara FM.
No admite disable/reason/drops/ignored/error, desfases worker/DSP grandes,
late_count/late_key_events/dropped_fm/dsp_jump_count durante la pista.
PCM: lee wf_sfx_engine.underruns32 directamente desde SoundRAM con layout
derivado por el compilador68000 exacto, y comprueba tambien el buzonC05C.
Exige coincidencia DEBUG/release de todas las cargas y hash de layout.
Contador positivo, ausente, sin identidad verificada o que retrocede bloquea
la aprobacion; un buzon obsoleto con0 ya no puede ocultar un underrun real.
One-shots05/08/09: debe observar activa y luego RAM Z80 inactiva+mascara FM0
tras su final host, en la misma generacion. RMS0 despues es NORMAL y no error.
RMS>0 solo acredita salida callback en algun tramo; no es un oraculo de timbre.
SFX: exige palabra aceptada en gameplay; control SFX debe tener salida audible
en el tramo comparado. No demuestra todos los efectos posibles.
Par grafico: master/MUSIC/boot/assets/inputs/core identicos, unica diferencia
SCSP.ELF. Tramo monotono exacto game_frame300..600, fase/seed constantes e
iguales, sin fatal/resets. Debe haber300 huellas de escena iguales y conteos
emitted_commands iguales, ventanas completas del contador render existente,
ratio VBlanks music/SFX<=1.0 y delta mediana render>=0. Datos ausentes bloquean.
La huella estricta aun necesita validarse en un par real; si diverge, investigar
la frontera/estado antes de atribuirlo a graficos o relajar el criterio.

LIMITES
Sin ejecucion Saturn emulada de este harness aun. Se verificaron solo sintaxis
Python, las inserciones del observer y el calculo de duraciones contra datos
host existentes. No se reclama green, rendimiento fisico Saturn, escucha
completa, fidelidad timbrica, todos los escenarios/SFX ni bucles completos de
0B/0C/11. El estado final permitido hoy es PREPARED_NOT_BUILT_NOT_EMULATED.
