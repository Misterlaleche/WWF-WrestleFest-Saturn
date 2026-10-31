# WWF full game — plan de integración

> For agentic workers: ejecutar tareas independientes con agentes y revisar
> el conjunto; no modificar los módulos de referencia.

**Goal:** beta completa sin audio definida en SPEC.md.
**Architecture:** un runtime propietario de plataforma y adaptadores de fuentes
aprobadas; recursos cargados antes de iniciar escenas. **Tech Stack:** C11,
Python 3, Yaul SH-2 GCC 13.2.0, Mednafen 1.32.1.
**Spec:** ../SPEC.md; diseño: DESIGN.md.

## Global Constraints

- No modificar carpetas originales; producción en production/full_game.
- ROM manda; comportamiento desconocido queda OPEN.
- Un disco universal; 1 MiB nunca se considera 4 MiB.
- Audio desactivado, eventos preservados; reloj independiente.
- No saltar lógica ni animaciones; no afirmar hardware por tests host.
- Publicar checkpoints verificables en feature/wrestlefest-full-game-rom-exact.

## Review Focus

- Carga fallida o que no termina: nunca avanzar escena silenciosamente.
- Frames terminales y sprites con retraso: conservar última salida y limpieza.
- IDs inválidos/duplicados y RNG ausente: rechazar sin sustituir selección.
- Acumulación temporal y atrasos: no descartar ticks ni adelantar estados.
- Recursos corruptos o fuente incorrecta: validar hash antes de reutilizar.

### Task 1: Recuperación y referencias
- [x] Recuperar 2023 archivos y verificar sus 989 blobs Git únicos y árbol completo.
- [x] Verificar programa 68000 local, identificar SDK y Mednafen existentes.
- [ ] Guardar inventario de fuentes/recursos y comprobar originales intactos.
- [ ] Regenerar texturas ROM-only demostrables, comparar hashes aprobados.

### Task 2: Reloj compartido
Files: src/wf_arcade_clock.{h,c}, tests/test_clock.c.
Interfaces: init(phase), field(), take(), contadores de campos/ticks pendientes.
- [ ] RED: 39168 campos producen 37609 ticks; fase inicial válida y errores.
- [ ] Implementar acumulador entero; mantener deuda durante retrasos.
- [ ] GREEN: periodos largos, atraso/recuperación, overflow fail-closed.

### Task 3: Adaptador Entrada / Presentadora
Files: src/wf_prematch.{h,c}, tests/test_prematch.c.
Consumes: núcleos originales wp01_scene y wp02_scene, cuatro IDs ROM y RNG.
Produces: estado explícito de carga, escena, fin y error; acceso a estado previo.
- [ ] RED: carga no avanza, IDs/hook rechazados, 6+402 salidas de Entrada.
- [ ] Adaptar enter/step/load; preservar IDs lógicos, latencia y terminales.
- [ ] GREEN: comparar cada estado con ejecución directa de núcleos originales,
  eventos audio y todos los IDs en las posiciones de la matriz de prueba.
- [ ] Compilar juntos los módulos en host y SH-2; no enlazar audio/main originales.

### Task 4: Reconstrucción de bloques y renderer maestro
- [ ] Recuperar/verificar ZIP aprobados y frontend TEST03.
- [ ] Reconstruir Intro/Entrada/Presentadora/TAG/Royal en copias aisladas.
- [ ] Adaptar render/CD/SMPC y traspaso de propiedad; un único main.
- [ ] Verificar perfiles de memoria y detección 4MiB por SDK/prueba de bancos.

### Task 5: Gameplay y progresión completos
- [ ] Completar todas las rutas TAG/Royal desde estados de partida, sin fixtures.
- [ ] Verificar movimientos/IA/RNG/colisiones con ROM; corregir lo incompleto.
- [ ] Integrar resultados, progresión, continues, records, rankings y finales.

### Task 6: Validación y conservación
- [ ] Ejecutar pruebas de referencia disponibles y revisión independiente.
- [ ] Guardar cada checkpoint con scripts, hashes y configuración real.
- [ ] Publicar/verificar commit GitHub y paquete local recuperable.
- [ ] Probar disco único en Mednafen 1.32.1 sin/con4MiB, mando y recorrido completo.
- [ ] Medir todas las ventanas del encargo y conservar vídeos reales.
- [ ] Entregar beta completa solo al cerrar requisitos; de lo contrario checkpoint
  explícitamente parcial con bloqueos exactos.
