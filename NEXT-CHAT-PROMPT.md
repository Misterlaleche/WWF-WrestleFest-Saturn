# Prompt recomendado para continuar en un nuevo chat

> Quiero continuar el desarrollo del port nativo **WWF WrestleFest arcade -> Sega Saturn NTSC-U** desde el último estado guardado.
>
> Repositorio privado: `Misterlaleche/WWF-WrestleFest-Saturn-Private`.
>
> **Primero lee `HANDOFF.md`, `WIP20-CHECKPOINT.md` y `NEXT-CHAT-PROMPT.md` de la rama `test255-lod-handoff`. No continúes desde `main`, WIP9 ni ramas LOD/HWFIX temporales.**
>
> Después materializa desde Library:
> `/WWF-Saturn/WWF-Saturn-Test255-WIP20-source.zip`
> SHA256 esperado:
> `5b09082b5d58707931ca8c7ca0a31ef4400485a2f6f75b547dc9626373e4bb81`
>
> La línea canónica sigue siendo **Test254 / V0.269**. **Test255 / V0.270 sigue siendo WIP.**
>
> WIP20 deriva de WIP19 y conserva Doomsday/Hawk/Animal. También integra la conducta de estabilidad HWFIX7 validada en Saturn real: los full-pack residentes `RING14.PAK`/`OBJ15.PAK` permanecen desactivados y sus familias usan solo fallback visual **PROVISIONAL** limitado.
>
> El diagnóstico de hardware 309/339/342/347/348/324 está cerrado. No vuelvas a intentar arenas `malloc()` grandes ni buffers `.bss` completos para RING14/OBJ15.
>
> Test275 ya demuestra un lector WFC2 streaming: RING14 usa 53012 bytes de metadata + bloques de hasta 3013 bytes; OBJ15 usa 32320 bytes + bloques de hasta 2741 bytes; ambos decodifican tile-for-tile idénticos al loader completo. **Todavía no está conectado al CD/runtime Saturn.**
>
> Continúa directamente con:
> 1. implementar y host-testear lectura por sectores CD para bloques WFC2, incluyendo offsets no alineados a 2048;
> 2. activar primero streaming de `OBJ15.PAK` manteniendo fallback HWFIX7 como rollback;
> 3. activar después `RING14.PAK`;
> 4. pedir validación en Saturn real antes de retirar cada fallback;
> 5. después volver al moveset individual de Hawk, Animal, sus interacciones contra los otros diez y cierre completo del roster.
>
> El trigger Doomsday Device ya está cerrado ROM-EXACT: Animal raw `$89` -> `+$26` -> Hawk 8 ticks -> `+$86` -> `$12C22` -> profile `$17` -> `+$92` -> `$12C0A` -> tercer actor por `victim +$26`. **No vuelvas a investigar ese puente.**
>
> Mantén siempre **ROM-EXACT**, **PROVISIONAL** y **REGISTERED BUT NOT IMPLEMENTED**.
>
> Guarda cada checkpoint importante en GitHub y `/WWF-Saturn/`.
>
> **No generes BIN/CUE salvo que yo lo pida. Cuando lo pida, encárgate tú de source, commits, GitHub Actions, corrección, descarga y entrega directa.**
>
> Si hace falta análisis byte-level adicional y no está disponible el ROM set, pídeme `wwfwfest.zip`; nunca lo guardes en GitHub/Library.