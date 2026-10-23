# Continuar el combate desde f88acad

> Ejecución autorizada por el usuario en este turno, sin pausa de aprobación.

**Objetivo:** resolver las acciones $3F/$5F con ROM intacta, continuar los
recorridos corregidos hasta el resultado del combate y verificarlo en
Mednafen con vídeo. Sin frontend, audio, código provisional ni entrega BIN/CUE.

**Arquitectura:** conservar los owners C nativos y el dispatcher real.
Usar los emuladores68000 exclusivamente como oráculos HOST. Capturar estados
naturales y registrar expresamente las regiones y registros comparados.

**Base:** f88acadb8c3c95a3e4a1fe10eaf6e3e3960dbf1a.
**ROM SHA1:** a726c46f350723ea73158bc78055d17d418b14e2.

- [x] Ampliar `tests/r18/allies_trace.c` y `allies_run_probe.py` para observar
  todos los actores opcionalmente y conservar siempre el primer fallo.
  Reproducir703/$3F y560/$5F sin alterar entradas ni estado de gameplay.
- [x] `$3F`: test nuevo `action3f_verify_rom.py` y snapshot inmutable.
  Ver RED por dispatcher ausente, implementar descriptor/callback y sus
  continuaciones demostradas en `wf_r17_actions_r11.c`, verificar GREEN
  con RAM, registros y efectos realmente alcanzados.
- [x] `$5F`: mismo ciclo con `action5f_verify_rom.py`, incluyendo la víctima
  natural de `$22`, transiciones, daño, temporizadores y enlaces recíprocos.
  Las ediciones de ambos callbacks en el archivo común serán secuenciales.
- [x] Repetir `allies_run_probe.py NUEVO --all-actors --frames 100000
  --require-complete`, sin entrada y con `--attack`. Cada siguiente fallo
  requiere snapshot, investigación ROM, RED→GREEN y owner nativo antes de
  repetir; ninguna omisión se convierte en éxito.
- [x] Consolidar solo recursos observados. Verificar bytes originales,
  capacidad de caché y lecturas CD, conservando fallback exacto. Ejecutar
  suite general, oráculos afectados y revisión independiente.
- [x] Compilar SH-2 con SDK fijado, ejecutar en Mednafen y verificar endpoint,
  actores, HUD y comandos/texturas VDP1 reales. Capturar vídeo a tiempo real,
  conservar métricas, hashes y checkpoint local. No declarar prueba física.

Riesgos que deben cubrir los tests: adquisición/liberación de enlaces,
flags BYTE frente a WORD, temporizadores negativos y underflow, scratch/RNG
y registros vivos, capacidad/orden de colas y recursos nuevos. La prueba
de rutinas aisladas no demuestra por sí sola combate completo o render Saturn.

## Cierre basado en el resultado observado

La ruta natural satisface el combate completo solicitado:3008 en HOST y
Mednafen, con resultado original y vídeo real. Se prioriza entregar ese
checkpoint probado. La ruta adicional con mando queda diagnosticada hasta
3373/raw1C y no se declara completa; no se sigue ampliando su investigación
durante el cierre. Ver informe final para pruebas, precarga y límites.
