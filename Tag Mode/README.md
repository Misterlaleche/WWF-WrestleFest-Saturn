# Tag Mode — WWF WrestleFest → Sega Saturn

Área independiente para preservar el desarrollo del combate TAG de **Hulk Hogan, Ultimate Warrior, Ted DiBiase y Mr. Perfect**.

**Autoridad:** `wwfwfest.zip` (ROM arcade) > evidencias ROM-EXACT auditadas > contratos Yaul/Saturn > código de prueba. No inventar movimientos, IA, tiempos, animaciones ni audio.

## Estado real de lo recuperado (30/10/2026)

| Versión | Qué se conserva | Qué NO está demostrado |
| --- | --- | --- |
| WP1979 | Código fuente y pruebas de referencia. Esta carpeta contiene una **copia Git independiente** de `native/` existente en `Combate real en Saturn`. | No equivale a WP2015 R6. |
| WP2015 R3 — checkpoint 06 | Archivo fuente completo de 370 ficheros de código/build/test, respaldado en Library y con SHA-256; aquí también hay código SH-2 original de R3 y un restaurador. | R3 es WIP; no fue validado como R6 ni en Saturn físico. |
| WP2015 R5 | BIN/CUE de prueba conservado fuera de GitHub; enlace y SHA-256 en el índice. | No se recuperaron fuentes R5 completas. |
| WP2015 R6 | Sólo mediciones, enlaces históricos no persistentes y registro de cambios. | **No se ha recuperado el ZIP de fuentes, BIN/CUE ni el vídeo**. No declarar R6 archivado. |

**Fuentes R3 recuperadas (370 archivos):** [descargar copia persistente en Library](https://chatgpt.com/library?file_id=libfile_629884379af08191ba24cdd9ce709c7a&account_id=personal). SHA-256: `17f13f0607c3facbec50942fdb838c97863d83245ff64566d00f5d3cdf20eff6`.

**Checkpoint R3 original (incluye fuentes y recursos externos):** [WP2015 checkpoint 06](https://chatgpt.com/library?file_id=libfile_5e4571b2c0d48191aa08dff0dc5a875b&account_id=personal). SHA-256: `321c23f6aebb576edb33effa8f71da3d61e45570405e97fc628b5de4f9aab405`.

La carpeta `WP1979-base/native` es una copia de objetos Git existentes y no modifica los originales. `WP2015-R3/codigo-comprobado/` contiene módulos auténticos recuperados byte por byte del checkpoint. Para restaurar el resto de fuentes R3, usa `tools/restore_wp2015_checkpoint.py` y el checkpoint externo. **Este README no debe interpretarse como una copia completa de R3 almacenada en GitHub**.

## Política de checkpoints desde ahora

1. Cada nuevo hito deberá subir primero **todas sus fuentes de producción, scripts y pruebas, con hashes**, bajo `Tag Mode/checkpoints/<identificador>/`. Mantener los commits inmutables (añadir otro identificador para una nueva revisión).
2. Confirmar que el commit existe en GitHub y que la reconstrucción desde el commit más los recursos externos produce el mismo ejecutable, **antes** de dar por entregado un BIN/CUE.
3. Conservar ZIP fuente completo y BIN/CUE en almacenamiento persistente, con hashes y enlaces. Los links temporales de ejecución **no cuentan como backup**.
4. Los recursos protegidos derivados de la ROM, imágenes de disco y PCM de ROM no se insertan en GitHub: registrar hashes, procedencia y recuperación local.
5. Registrar resultados de Mednafen por separado de los resultados en Sega Saturn real. No calificar un checkpoint WIP como HARDWARE-PASS.
6. No modificar `Intro`, `Entrada Ring`, `Escena Presentadora`, `Combate real en Saturn`, `modo Royal` ni código maestro fuera de `Tag Mode/` para mantener esta línea independiente.

No hay sincronización automática en segundo plano: el guardado en GitHub requiere un commit real en cada checkpoint.

Comienza por [INDICE-CHECKPOINTS.md](INDICE-CHECKPOINTS.md) y [INTEGRACION-CODEX.md](INTEGRACION-CODEX.md).
