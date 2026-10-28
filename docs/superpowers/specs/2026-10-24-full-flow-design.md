# Flujo completo y control en Saturn/MODE

Solicitud del usuario, 24 de octubre de 2026: corregir el bloqueo observado
al pulsar el mando en Saturn americana60Hz; la siguiente imagen destinada a
juego debe recorrer antidrogas, modo, roster, entrada/presentacion y combate
con control libre. Continuar despues las mecanicas pendientes.

## Arquitectura elegida por el implementador

Integrar owners C nativos en el runtime18 existente, usando la ROM original
y WP12/15/20/25/29 como autoridad. Reutilizar CDFS, shadows de video/paletas,
metasprites, SMPC y los contratos de memoria. Conservar modo, identidades,
registros y RNG en el enlace de frontend a combate. La semilla fisica se
observa; no se fuerza cero para esconder rutas pendientes.

Alternativas consideradas: reutilizar Test15 entero daria un flujo rapido,
pero sus controles, tiempos y corredor son aproximaciones documentadas;
mostrar placas sin owners preservaria parte del aspecto y perderia efectos,
seleccion y transiciones. Ninguna satisface el recorrido solicitado con la
autoridad del proyecto. Los assets historicos solo se aceptan con procedencia
y verificacion contra ROM, reutilizando el mapa de memoria actual.

## Estado y evidencia

- La imagen del23/10 completa una secuencia automatizada concreta; no cubre
  juego libre. Se conserva sin sobrescribir como evidencia historica.
- Video IMG_4305.mp4: a32s aparece fail() verde/cian al pulsar el mando.
- Reproduccion sobre ese BIN: A600 alcanza frame767/CPU1C150/dependencia1C2CC;
  sus nueve actores coinciden byte a byte con la captura HOST natural.
- Causa reproducida: la entrada state0 no dirige B7.bit0/bit4 aislados al
  prefijo existente. La ROM1C370/1C37E si los consume. Corregir solo ese gate,
  comparando RAM/FG/D0-D7/RNG y manteniendo guards ajenos.
- Inventario completo: validation/local-20261024/frontend-inventory.txt.
  No hay owners frontend C integrados; los workpacks auditados locales
  permiten portarlos. Los ZIP specialist ausentes no son un bloqueo general.

## Flujo y limites

BOOT conserva FBI habilitado, legal, Technos y title; START acreditado entra
al modo. La seleccion confirma con action1/2 y conserva el normalizador
frontend original; no importar START-confirm/B-back de Test15. TAG y Royal
requieren sus rutas y setups propios: no mostrar un modo como jugable mientras
su enlace no lo sea. El prematch sigue WP12, incluido skip y teardown.

No inventar pasillos, carreras o entradas por cuerdas que no demuestre la ROM.
No presentar una ruta parcial como implementacion universal ni certificar
hardware por una partida emulada. Audio no fue anadido expresamente a este
alcance; conservar el dispositivo silencioso hasta integrar su pipeline.

## Verificacion requerida

Reproducciones de pulsaciones aisladas, mantenidas y combinadas en momentos
distintos; semilla observada en hardware y registros de entrada cuando haya
otro bloqueo. Oraculos ROM de cada owner modificado; regresiones CORE y de
transporte; SH2 dentro de memoria; imagen EDC/ECC exacta; secuencia completa
en emulacion con entrada externa; despues comprobacion Saturn/MODE.

El diagnostico debe mostrar datos legibles al fallar, sin retirar el guard.
Un disco de diagnostico, si se necesita para recopilar datos fisicos, se
identifica expresamente y no sustituye la entrega del flujo completo.
