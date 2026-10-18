# Continuación desde este checkpoint

No repetir inventario ni reconstruir desde $7A00. Usar el código src/runtime incluido y la rama de recuperación. Leer CURRENT-STATE-LATEST.md y validation/README.md; comprobar SHA256SUMS del paquete y el patch contra b85df2d.

Los bloqueos $2017A y compilación SH-2 están resueltos. No volver a declararlos ausentes. Las correcciones de $10120/$10E6A/$F4C2 también están integradas y probadas.

Siguiente trabajo concreto: implementar los joins nativos $6E34/$9132 y sus estados/hijos desde ROM y autoridades, sin reemplazar por los dobles de los tests. Consultar royal-9132-boundary.txt antes de diseñar el estado faltante. Después auditar composición completa D1–D7 de los hijos del dispatcher y construir un test de frame activo con valores esperados ROM, conservando el RNG persistente. Los callbacks de presentación y los de gameplay son fronteras distintas.

Reejecutar 43 tests y los nuevos que se añadan con Clang, GCC, ASan/UBSan, analyzer, auditorías ROM y full ELF. Hacer revisión independiente y extracción limpia. Solo entonces valorar cierre canónico de RUNTIME-17. RUNTIME-18 será integración de combate/render real; el destino de pruebas es emulador y Saturn con CD-R, no asumir MODE ni hardware PASS.
