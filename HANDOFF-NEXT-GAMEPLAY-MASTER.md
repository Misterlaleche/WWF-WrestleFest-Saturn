# Relevo RUNTIME-18 actual

Continuar en `runtime18/direct-combat`; leer CURRENT-STATE-LATEST.md y
validation/RUNTIME18-DISC.md. Ya hay arranque directo, scroll VDP2 y precarga
de sprites. Medir el coste restante de CPU/presentación y resolver $1F7F0 y
$1C3FC con evidencia ROM hasta completar un combate. Mantener frontend y
audio fuera de prioridad. La prueba física CD-R sigue pendiente.

## Relevo histórico del checkpoint RUNTIME-17 R02

Las restricciones de inicio de RUNTIME-18 del texto histórico siguiente ya
fueron superadas por autorización expresa del usuario.

Usar este árbol consolidado; no reiniciar la búsqueda de $2017A/$6E34/$9132 ni volver a importarlo desde una base anterior. Los tres owners están implementados y comprobados. Leer CURRENT-STATE-LATEST.md y validation/README.md, verificar SHA256SUMS y restaurar la historia Git incluida si se necesita.

El cierre documentado es el de los pendientes solicitados de RUNTIME-17. Se ha comparado un frame activo completo y su ACK, con alcance explícito. No convertir esa evidencia en una afirmación de ROM-EXACT universal, framebuffer exacto o hardware PASS.

No continuar automáticamente a RUNTIME-18. Cuando el usuario lo autorice, la siguiente integración deberá producir un combate auténtico visible en Saturn, con arranque directo al ring si es necesario, sin priorizar frontend ni audio. Los destinos de validación son emulador Saturn y Saturn real con CD-R. El ELF incluido ahora es solo un harness de enlace y no debe grabarse como juego.
