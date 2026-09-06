# Prompt recomendado para continuar en un nuevo chat

Pega este texto como primer mensaje del nuevo chat:

> Quiero continuar el desarrollo del port nativo **WWF WrestleFest arcade -> Sega Saturn NTSC-U** desde el último estado guardado.
>
> Repositorio privado:
> `Misterlaleche/WWF-WrestleFest-Saturn-Private`
>
> **Primero lee `HANDOFF.md` y `WIP16-CHECKPOINT.md` de la rama `test255-lod-handoff`. No continúes desde main ni desde ramas LOD temporales.**
>
> Después abre/materializa desde Library:
> `/WWF-Saturn/WWF-Saturn-Test255-WIP16-source.zip`
> SHA256 esperado:
> `dac151299786211086d10b42ba0f2df4156f2efd137792934d457c58ae9be374`
>
> La línea canónica sigue siendo Test254/V0.269 hasta que yo valide Test255 en Saturn real. Test255/V0.270 sigue siendo WIP.
>
> El último punto técnico cerrado es el trigger ROM-EXACT del Doomsday Device: Animal mantiene al rival con raw `$89`; Hawk espera 8 ticks, consulta a su partner mediante `+$86`, detecta partner state 5/raw `$89` y entra en `$12C22`. La cadena de colisión `$17`, link `+$92`, tercer actor por victim `+$26`, descriptor `$12C0A`, daño y estados está descrita en `HANDOFF.md`.
>
> **Continúa directamente con la integración real del Doomsday Device en el runtime Saturn** usando esa secuencia exacta. No vuelvas a investigar el puente de activación como si estuviera pendiente y no uses un trigger sintético simplificado.
>
> Mantén siempre las etiquetas:
> - ROM-EXACT
> - PROVISIONAL
> - REGISTERED BUT NOT IMPLEMENTED
>
> Después del Doomsday, sigue con los movimientos individuales restantes de Hawk/Animal y cierre del roster completo.
>
> Guarda cada checkpoint importante en GitHub y en `/WWF-Saturn/` de Library.
>
> **No generes BIN/CUE salvo que yo lo pida. Cuando lo pida, encárgate tú de preparar el source, GitHub Actions, corregir fallos, descargar el artefacto y entregarme directamente el BIN/CUE; no me pidas que ejecute workflows manualmente.**
>
> Si necesitas evidencia byte-level de la ROM y no tienes el ROM set en el nuevo chat, pídeme que vuelva a subir `wwfwfest.zip` antes de inferir nada.

## Archivo que probablemente debe volver a subir el usuario

El único archivo externo que puede ser necesario para seguir haciendo reverse engineering ROM-exact es el ROM set original del usuario:

`wwfwfest.zip`

No debe almacenarse en GitHub ni redistribuirse. El resto del estado fuente necesario está en el ZIP WIP16 de Library y en la documentación de la rama `test255-lod-handoff`.
