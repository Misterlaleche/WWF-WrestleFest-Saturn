# Corrección aislada del audio — plan de continuación

> Ejecución autónoma autorizada por el propietario el 2026-11-02. Se corrige
> Audio 2; Codex integrará el resultado en ROM Graphics Final.

**Objetivo:** cerrar la reproducción de los 18 comandos musicales, sus
transiciones y fades originales, y la fidelidad de los 98 comandos SFX.

**Arquitectura de partida:** candidato 68000/SCSP cddd y secuenciador SH-2
scalar9. Se conserva íntegra la copia publicada en
`2b1c6ecacb674e829553b6f46ab25d66b500888f`. La copia git local es una
recuperación parcial; su historial no se empujará a GitHub.

**Especificación:** encargo del propietario, `AUDIO-FINAL-AUDIT.md`,
`NEXT-SESSION.md`, `docs/NATIVE-LONG-FINAL-CANDIDATE.md` y
`docs/ACTIVE-OKI-BANK-AUDIT.md`. La ROM original sigue siendo la autoridad.

## Restricciones

- No modificar gameplay, gráficos, frontend, TAG, Royal ni otras escenas.
- R04/A53 y todas las capturas anteriores se conservan.
- Mednafen 1.32.1 sin alterar relojes para pruebas Saturn.
- No elevar buffers, umbrales ni tolerancias para esconder errores.
- No reproducir música completa pregrabada como motor de Saturn.
- No publicar ROM, BIOS, PCM, WAV ni otros datos derivados protegidos.
- No fusionar a main; conservar HARDWARE PENDING y aceptación auditiva pendiente.

## 1. Causa y corrección del fade nativo

Responsable: coordinador. Archivos de investigación: `src/wf_music_module.c`,
`src/wf_music_seq.c`, `src/wf_music_time.h`, `src/wf_music_clock.h` y el núcleo
Z80. Prueba de comportamiento: `tests/test_music_module_deadline.c` y nuevas
pruebas sólo para la causa demostrada. Instrumentación de ensayo separada
del motor seleccionado.

- [x] Reproducir un fallo `1F` en el reproductor aislado, con medidas de reloj,
  ciclo original, momento de generación, tiempo de servicio y cola pendiente.
- [x] Separar coste de ejecución, salto de reloj, MMIO y bloqueo de servicio.
- [x] Escribir la regresión que falla por esa causa, corregir sólo esa causa
  y verificar que la regresión pasa sin cambiar los límites de aceptación.
- [x] Repetir la captura causal y después las 18 músicas y transiciones.

## 2. Frontera de temporización del controlador original

Responsable: agente de diagnóstico. Comparar `full` y `a53-mmio` del productor
existente con el mismo comando `02`, el mismo `1F` y 608 segundos. Conservar
orden, valores y tiempos de eventos. Este contraste host no sustituye una
medición nativa ni atribuye automáticamente la causa del retraso.

- [ ] Ejecutar y conservar el contraste.
- [ ] Identificar el alcance de la diferencia y su prueba de regresión.

## 3. Banco OKI de las voces activas

Responsable: agente de prototipo. Primera entrega en archivos nuevos bajo
`tests/oki_address_replay/`, sin cambiar aún el runtime ni la ABI de recursos.

- [x] Prueba fallida de resolución por dirección original y cambio de banco.
- [x] Resolver las 517.450 direcciones requeridas mediante el recurso
  existente y la extensión WFBA, comparando bytes contra la ROM original.
- [x] Verificar ambas paridades de nibble, estado ADPCM, checkpoints,
  fronteras y cambios repetidos; repetir el caso mixto causal.
- [ ] Sólo después de revisión, medir coste y memoria de una integración
  nativa aislada antes de seleccionarla.

## 4. Finales y bucles con el modelo de placa completo

Responsable: agente de referencia. Nuevos archivos
`tools/reference/probe_full_board_music_state.c`,
`tools/reference/analyze_full_board_music_structure.py` y sus pruebas.

- [x] Probar que el observador conserva CPU, RAM, trazas y PCM.
- [x] Observar ramas y finales originales con Timer A/B, busy e IRQ acoplados.
- [x] Distinguir repetición de punteros, bucle de partitura y síntesis completa;
  no afirmar una composición completa sólo por grabar 600 segundos.

## 5. Selección y entrega

- [x] Revisar cada corrección con evidencia antes de sustituir el candidato.
- [x] Reconstruir juntos los objetos afectados y comprobar mapas y contratos.
- [ ] Verificar 18 músicas, 98 comandos, fades, cambios y mezcla aislada.
- [ ] Generar comparativas largas con versión, hashes y fallos preservados.
- [ ] Guardar código y metadatos en la rama independiente, sin integración
  de Graphics Final ni publicación de audio protegido.

## Registro de decisiones

- Se permite corregir el código de audio aislado: el propietario lo ha
  confirmado explícitamente. No se solicita otra autorización técnica.
- La campaña anterior conserva 4 pases/14 fallos y transiciones fallidas.
  Los ensayos nuevos no reescribirán esa evidencia.
- Las 18 referencias nuevas de 608 segundos conservan el primer tramo de
  30 segundos y el fade original. Sus archivos se han comprobado; esto no
  certifica por sí mismo composición completa, bucles ni fidelidad Saturn.

## Avance comprobado del candidato d9bd

Los cuatro puntos de causa/corrección se completaron para la matriz corta:
18 músicas y transición pasan. El host largo pasa14 grupos con600s+8s por
comando. La prueba nativa larga terminó:18 músicas y transición pasan, con fuentes e
identidades estables y cero contadores de error.
El prototipo host de banco activo pasa; su integración nativa sigue pendiente.
Los18 finales/bucles originales están clasificados con el observador full-board.

El replay mixto actual entrega64/64 palabras y registra0FM/0underruns, pero
2avisos tardíos: continúa FAIL. La reproducción bajo combate corresponde a
la integración posterior de Codex y conserva los fallos históricos visibles.
