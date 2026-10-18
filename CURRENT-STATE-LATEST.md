# RUNTIME-17 — checkpoint consolidado de recuperación, 2026-10-18

**CONSOLIDADO Y PROBADO EN HOST/SH-2. CIERRE CANÓNICO PENDIENTE. NO RUNTIME-18. NO HARDWARE PASS.**

## Procedencia

Base GitHub: `gameplay-runtime17-staging`, commit `b85df2d23e4c64ecf24b5d2d41c4dc7b9000189e`, árbol `01b72cee6491312f3a55fd64efa1f49539863408`. Se recuperaron y verificaron todos sus blobs y el objeto commit exacto mediante el conector. El repositorio local es shallow en ese commit. No se ha modificado ni publicado ninguna rama remota.

Implementación acumulativa: Continuation-4 → Continuation-3 → `RUNTIME17-CONTINUATION-2-LIVE-WORKTREE-WIP.zip`, SHA256 `df130112b94d746ebc796bebf9159b23670ec73268ed6ab5698c8264dba69d6e`. El checkpoint $7A00 del relevo coincide con SHA256 `ebcd26f52a5fa5c4aa5d0f0b71dea8b667c9e5cc5a9e02b8dcd0d037739ab555`. No se tomó gameplay de WIP160/Test255.

Rama local: `recovery/runtime17-20261018`. El commit de entrega consta en el manifiesto externo del paquete.

## Cambios efectivos

- Importado el árbol acumulativo real con owners post-$7A00 y ramas A/B, ausentes del espejo GitHub anterior.
- Implementado $2017A desde desensamblado de ROM: scans, escrituras solapadas, D0–D7, actor seleccionado, activación y gates post-ACK. $206FE conserva su frontera externa de transferencia de vídeo y sus registros; falla si falta el callback.
- Corregidas omisiones demostradas de shadow de registros en $10120, $10E6A y $F4C2. Conservadas anchuras WORD y mitades altas; añadidas regresiones.
- Auditoría $262D2 acepta ruta ROM por argumento, eliminando la dependencia fija de /mnt/data.
- Añadida reproducción portable HOST y SH-2. Corregido el orden de configuración Yaul para que el mapa se genere en audit/saturn-link-build sin depender de una carpeta creada por HOST.

## Evidencia de esta ejecución

43/43 tests con Apple Clang 17 strict; 43/43 con ASan/UBSan; 43/43 con GCC Linux 16.2 strict. GCC -fanalyzer sobre todas las fuentes src/generated sin diagnósticos. Catorce auditorías de firmas/spans ROM pasan, incluida SHA1 canónica `a726c46f350723ea73158bc78055d17d418b14e2`.

Compilación limpia y enlace completo con SH-2 GCC 13.2.0: `nm -u` vacío. ELF observado SHA256 `fabedfe7d63e7a0afba98043b47b414104679b482318be8e8dce3fa270a54865`. El linker mantiene avisos RWX de los scripts Yaul suministrados. Es un harness de enlace, no un ejecutable de combate. Estos resultados prueban las comprobaciones ejecutadas, no equivalencia total con el arcade.

## Bloqueos que impiden declarar RUNTIME-17 cerrado

1. `$18C4` delega gameplay TAG `$6E34` y Royal `$9132`; no hay implementación nativa recuperada. El test A sustituye TAG por un callback de prueba exitoso. No se ha añadido sustituto a producción.
2. `$9132` alcanza selección Royal, estado persistente adicional, inputs, FG0 y helpers `$1079E/$782C/$7506/$7720/$75EE`; no puede cerrarse como una pequeña inicialización. La evidencia directa y los hashes de spans están en docs/recovery-evidence/royal-9132-boundary.txt.
3. Falta la prueba completa de composición D1–D7 de todos los hijos del dispatcher y un oracle ROM del tuple de registros/RNG de un frame activo completo. Los tests A/B comprueban recorridos con acciones restringidas.
4. Rutas de cambio de escena después del ACK permanecen explícitamente bloqueadas. Antes del ACK, WF_BLOCKED por dependencia no es una espera reanudable: no repetir frame como si fuera VBlank.

El entry Saturn conserva el harness de enlace. Emulador, imagen BIN/CUE y Saturn real con CD-R: NOT RUN. No se entrega disco jugable ni se promueve a RUNTIME-18.

La documentación histórica conservada en docs/recovery-evidence describe su momento original; este estado y los logs de recuperación prevalecen al interpretar qué se probó ahora.
