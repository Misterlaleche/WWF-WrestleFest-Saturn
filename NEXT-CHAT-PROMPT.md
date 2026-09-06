# Prompt recomendado para continuar en un nuevo chat

> Quiero continuar el desarrollo del port nativo **WWF WrestleFest arcade -> Sega Saturn NTSC-U** desde el último estado guardado.
>
> Repositorio privado: `Misterlaleche/WWF-WrestleFest-Saturn-Private`.
>
> **Primero lee `HANDOFF.md`, `WIP18-CHECKPOINT.md` y, para contexto histórico, `WIP16-CHECKPOINT.md` de la rama `test255-lod-handoff`. No continúes desde `main` ni desde ramas LOD temporales.**
>
> Después materializa desde Library:
> `/WWF-Saturn/WWF-Saturn-Test255-WIP18-source.zip`
> SHA256 esperado:
> `fc9191006caa757d056a1511c8a8bc486ed2c440abf7a1381273dc45f958caf8`
>
> La línea canónica sigue siendo **Test254 / V0.269** hasta que yo valide Test255 en Saturn real. **Test255 / V0.270 sigue siendo WIP.**
>
> El Doomsday Device ya está integrado en WIP18 mediante la cadena recuperada: Animal K/raw `$89` -> `+$26` -> Hawk state8/state9 -> 8 ticks -> partner `+$86` state5/raw `$89` -> `$12C22` -> profile `$17` -> target type `$12` / `+$92` -> `$12C0A` -> tercer actor por `victim +$26` -> daño/estados -> `LODTEAM.PAK`.
>
> **No vuelvas a investigar ese trigger ni lo sustituyas por una condición sintética.** Consulta `HANDOFF.md` para distinguir qué partes de la integración son ROM-EXACT y qué adaptadores Saturn siguen PROVISIONAL.
>
> Continúa directamente con:
> 1. moveset individual completo de Hawk;
> 2. moveset individual completo de Animal;
> 3. interacciones de ambos contra los otros diez luchadores;
> 4. cierre completo del roster.
>
> Mantén siempre las etiquetas **ROM-EXACT**, **PROVISIONAL** y **REGISTERED BUT NOT IMPLEMENTED**. No promuevas identidades de movimientos, timings, geometría, IA o efectos por plausibilidad.
>
> Guarda cada checkpoint importante en GitHub y `/WWF-Saturn/` de Library.
>
> **No generes BIN/CUE salvo que yo lo pida. Cuando lo pida, encárgate tú del source, commits, GitHub Actions, corrección de fallos, descarga y entrega directa; no me pidas que ejecute workflows manualmente.**
>
> Si necesitas continuar análisis byte-level y no está disponible el ROM set, pídeme que vuelva a subir `wwfwfest.zip`.
