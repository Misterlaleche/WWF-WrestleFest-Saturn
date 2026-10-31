# Sesión06 — transporte vivo de vídeo y eventos Saturn

Goal: conectar wf_session a CD, SMPC, reloj compartido y VDP1/VDP2, desde RESET
hasta el límite original AC0 de selección. No cambiar los propietarios HW1.
La integración posterior de escenas/combate seguirá desde esa misma máquina.

Decisión técnica: el bitmap TEST03 mezcla identidades de paleta y une los slots
8/9. Para mostrar la sesión original se necesita transportar los mapas, bancos,
scroll y registros de sprites que ya produce HW1. Se reutilizan el formato de
celdas/paletas y la decodificación ROM de Intro R04; no se reproduce SEQ.DAT.
TEST03/disco04 permanecen recuperables y su build queda separado. Nuevo backend
nativo compartirá main/plataforma/CD/reloj; no concatena ejecutables.

Eventos: referencia MAME0264 ddragon3.cpp:272 líneas, IRQ2 cada16, IRQ3 en248,
VBlank activo en bajo fuera de8..247. No copiar el calendario artificial de
session_probe. SRAM física inicial, DIP e input son responsabilidades explícitas
del adaptador; ningún crédito, equipo, RNG o confirmación se inyecta por lógica.
Mantener ACK/latches y prioridades; reanudar tras cambios de generación.

## Tareas

- [x] Adaptador portable de bus/eventos con pruebas de fase, IRQ/ACK/SR,
  lecturas big-endian, mandos activos en bajo y errores de rango.
- [x] Compilador de assets ROM, atlas/caché y proyección viva de mapas/paletas/
  sprites. Pruebas de equivalencia de formatos y límites sin color deduplicado.
- [x] Integrar en Saturn con el CD/SMPC/reloj existentes; presupuesto de RAM y
  VRAM comprobado al enlazar. Cold state y calendario documentados.
- [x] Mednafen1.32.1, disco único None/1MiB/4MiB, coin/START/TAG/compañero y
  Royal9 con mando; capturas, logs, hashes y comparación de imagen.
- [x] Suite host/sanitizers, revisión final independiente y estado recuperable.

Evidencia final: `evidence/session06`. Alcance funcional de este plan completo;
la cadencia durante arranque/setup queda explícitamente OPEN (deuda hasta101).
Publicación y empaquetado conservan el hito sin declararlo beta completa.

Review Focus: señal VBlank invertida; IRQ enmascarado perdido; autoría de input
con A mantenido; transparencia/banco de paleta y slot9; textura/memoria fuera de
límites o fallo CD oculto. No afirmar que AC0 es gameplay ni reutilizar la tasa
histórica Royal como rendimiento integrado. La Intro R04 sigue pendiente de su
handoff explícito; no se declara sustituida por el BOOT de la sesión.

Base:5bc06faeafa185a776b2cd17338bbd1df431607a.
Ledger: docs/PROGRESS.md. Autonomía y rama existentes autorizadas en SPEC12/15.
