# Resolución de revisión05

1. Demolition P2: el harness esperaba10/11 y fallaba aunque la máquina original
producía11/10 tras normalizar el primer actor sin crédito. Se ajustó sólo la
expectativa del test y se añadió demolition-p2 a ambas suites. Normal y
ASan/UBSan pasan, clock601, RNG9c0aa86a. No se alteró la lógica de selección.

2. Enlace SH-2: ld-r retenía entradas undefined sin relocaciones desde secciones
eliminadas. objcopy --strip-unneeded conserva explícitamente las tres funciones
públicas de wf_session. La comprobación final sólo encuentra memcpy/memset.
El archivo se genera de nuevo antes de reemplazar el anterior; archive-regression.py
inserta wf_actor.o obsoleto y prueba su eliminación y el SHA256 final idéntico.
Resultado de ambos pasos en sh2-build.log. 183464B, GCC13.2.0.

No hallazgos P2 pendientes de esta revisión. Las restricciones del alcance
original siguen vigentes: sin nuevo disco, renderer, IRQ de hardware ni medida
de rendimiento. El oracle de START no cubre todo el recorrido.
