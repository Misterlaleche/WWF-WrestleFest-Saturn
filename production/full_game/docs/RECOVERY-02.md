# Recuperación y reconstrucción verificadas — checkpoint 02

Los seis paquetes suministrados por el usuario se conservan externos a Git.
Los cinco primeros coinciden con los SHA256 fijados por los handoffs originales.
TEST03 tiene SHA25665daa21e89f505d6ff23efc80389145e5e32b4ada6bc8fb98df9f43417d1861a;
sus30 entradas de manifiesto coinciden tras resolver el prefijo absoluto de su
empaquetado histórico. El BIN de referencia coincide con el hardware-pass c9e81bb6….

Se preservan las370 fuentes/configuraciones/herramientas R3 en vendor/tag_r3,
exactas al prefijo wp2015/work/native del ZIP. SOURCE.json registra cada hash.
Esto recupera R3 y no R6. Las fuentes TEST03 se guardan en vendor/frontend_test03,
sin incluir assets ni binarios protegidos. Los originales del repositorio siguen
sin modificación.

## Compilación real

SDK suministrado: sh2eb-elf GCC13.2.0, SH-2 big-endian. Linux amd64, Python3,
make y xorrisofs. Cada build se ejecutó en una copia nueva y sin red, con el SDK
montado solo lectura. El wrapper tools/rebuild_references.sh reproduce los comandos
con WF_REFERENCES_ROOT y YAUL_INSTALL_ROOT explícitos.

| Bloque | Resultado del ejecutable reconstruido |
| --- | --- |
| IntroR04 | SHA25632aa415f… idéntico al aprobado |
| EntradaR02 | SHA256fc7d9fa0… idéntico al A.BIN del paquete |
| PresentadoraR02 | SHA2561a3566c4… idéntico al aprobado |
| MODE+ROSTER TEST03 | SHA256c7f7a677… idéntico al A.BIN del paquete |
| TAGR3 | Build nuevo WP2015_AUDIO=0, heap acotado y guardas IRQ/MAC conservadas |
| RoyalHW1 base/4MiB | Dos builds nuevos WP1980_AUDIO=0; checks HWRAM/pilas PASS |

Se generaron siete BIN/CUE de referencia MODE1/2352 (las cuatro escenas/frontend,
TAG y las dos variantes Royal). Todos sus sectores EDC/ECC se comprobaron y A.BIN
coincide con el ejecutable enlazado. Siguen siendo referencias independientes.
El disco universal del juego integrado todavía no existe.

## Ejecución de referencia

Royal sin audio pasa Mednafen1.32.1, caché full, None y4MiB, durante180 campos
de combate por perfil, con176 dibujos completados y cero fatal_owner/dependency.
La versión4MiB detecta ID0x5C y4194304B. Se conserva MOV interno con timestamps
originales. Esta prueba breve no acredita rendimiento de un recorrido completo.

Los13 probes Royal se ejecutaron: los cinco RED fallaron por el comportamiento
original esperado y los ocho GREEN pasaron, incluido el tramo68000 de referencia,
la denegación de lecturas y el reemplazo de cachés en resultados. No hubo RED por
error de compilación.

Las tasas de los checkpoints históricos no se atribuyen a esta integración.
Las pasadas largas nuevas se documentan en PERFORMANCE-02.md. La entrada por
mando aún debe tener su propia evidencia en el runtime integrado.

## Integración pendiente

El core compartido actual enlaza lógica Entrada/Presentadora y reloj, pero aún
faltan renderer/CD/SMPC únicos, frontend completo, detección/cart en un solo programa,
los motores y todas las rutas de progresión. Los paquetes ya están disponibles;
no se mantiene el antiguo bloqueo de recursos. Las implementaciones aprobadas
conservan su identidad como autoridad de integración.
