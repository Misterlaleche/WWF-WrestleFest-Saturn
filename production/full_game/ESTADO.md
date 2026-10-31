# Estado de integración — 2026-10-31

Estado: CHECKPOINT DE FUENTES PARCIAL. La beta completa sigue en desarrollo.
No hay todavía disco universal, gameplay integrado ni nueva prueba Mednafen.

## Verificado en esta sesión

- Recuperados 2023 archivos del commit aedca682; sus 989 blobs y árbol Git
  coinciden. Las carpetas originales permanecen intactas por SHA256.
- ROM del usuario: programa68000 SHA1 a726c46f350723ea73158bc78055d17d418b14e2.
- Nuevo reloj racional37609/39168 con deuda conservada;100 períodos exactos,
  errores y overflow comprobados; C11 y ASan/UBSan.
- Adaptador Entrada→Presentadora enlaza directamente los núcleos aprobados;
  6+402 salidas Entrada, latencia de sprites, IDs lógicos y terminales conservados.
  53 recorridos contra núcleos originales. Continuidad RNG entre propietarios OPEN.
- Presentadora original:20 casos/24922 ticks contra ejecución ROM mediante
  Unicorn2.1.4; estados,RNG y comandos comparados.
- Core SH2 GCC13.2.0 compilado y enlazado relocatable sin backend audio.
  No es un programa arrancable; necesita todavía startup/libc/plataforma/renderer.
- Ocho texturas411712B regeneradas desde ROM y hashes aprobados;6 tests pasan.
- Cinco ZIP suministrados por el usuario coinciden con sus SHA256. Recursos
  visuales19/19 restaurados y verificados. TAG es R3; R6 NO recuperada.
- Royal: inflate fijo378 comprobaciones,LZ4 1536,dinámico ASan/UBSan13005;
  estas pruebas iniciales no incluían atlas original.

## OPEN y trabajo siguiente

1. Reconstruir IntroR04,EntradaR02,PresentadoraR02,TAGR3 y RoyalHW1 intactos
   en directorios nuevos. Registrar binarios,mapas,presupuestos y hashes.
2. Runtime maestro con único CD/SMPC/reloj/VDP1/VDP2 y traspasos de propiedad.
3. Detección4MiB y presupuesto dinámico; Royal usa dos builds por macros y no
   cabe sumar sus máximos estáticos en la Saturn base.
4. Frontend completo y escenas gráficas integradas. Eventos Presentadora
   conservados; contrato prepare/start/stop y registros de Entrada OPEN.
5. TAG/Royal con todos los participantes,acciones,IA y transiciones originales;
   resultados,progresión,continues,records,rankings y finales.
6. Disco universal real;Mednafen1.32.1 None/4MiB,mando,vídeos y todas las métricas
   pedidas. Las tasas históricas no son medidas de este nuevo core.

Sin sonido en producción. Reconstrucciones de referencia pueden conservar el
código original para auditoría; no se promueven como beta de esta integración.

## Reproducción del checkpoint

Desde este directorio: `WF_ROM=/ruta/wwfwfest.zip make test sanitize`.
`make test` sin WF_ROM omite explícitamente cinco tests que requieren la ROM.

En Linux con Python3,make y SDK suministrado:
`YAUL_INSTALL_ROOT=/ruta/sh2eb-elf make sh2`.
El resultado es `build/sh2/libwf_full_game_core.a`, no un disco.

Para repetir los diferenciales originales, usar un venv con Unicorn2.1.4 y
compilador host/liblz4 disponibles:
`python tools/run_reference_checks.py --rom /ruta/wwfwfest.zip --out /ruta/nueva`.

Para restaurar los cinco ZIP a una copia nueva:
`python tools/restore_packages.py --packages /ruta/paquetes --out /ruta/nueva`.
Los binarios protegidos y la ROM permanecen externos a Git.

Revisión menor pendiente: pruebas adicionales de confinamiento del recuperador.
Su publicación por renames no es atómica ante un error de disco; toda salida de
una ejecución fallida debe considerarse incompleta y revisarse antes de usarla.
