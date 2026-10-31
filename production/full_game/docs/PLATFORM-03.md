# Plataforma común — checkpoint 03

Continúa Task 4 de PLAN.md; autoridad SPEC.md. Autonomía y publicación ya
autorizadas. El mismo ejecutable debe arrancar None/1MiB/4MiB. Este hito no
equivale a la beta ni añade reglas de juego a TEST03.

1. Prueba de expansión: después de dram_cart_init del SDK, aceptar sólo ID4MiB,
   tamaño4MiB y base0x22400000; verificar extremos de los ocho bancos de512KiB
   con patrones distintos y complementarios, restaurando los valores sondeados.
   Cualquier alias/fallo conserva perfil interno. Prueba host con bus simulado,
   bancos espejados/fallo de datos/restauración. Prueba real Mednafen tres perfiles.
2. Propietario único SMPC/VBlank y reloj racional. ISR cuenta campos; sólo main
   modifica el reloj. Carga excluida mediante frontera explícita. La deuda no
   se descarta durante la escena. Un paso visible por salida presentada.
3. Separar TEST03 en estado de selección y renderer. Conservar funciones de
   dibujo, capas y orden de parches. Sustituir bucles/espera privada por pasos
   del maestro. Reducir BSS mediante reutilización de almacenamiento entre MODE
   y ROSTER. No inferir identidad de luchador a partir del slot visual.
4. Compilar BIN/CUE universal y comprobar detección, reloj, navegación y
   confirmación mediante mando emulado y observación sólo lectura. Conservar
   vídeo, configuración, ejecutable y hashes; publicar checkpoint verificable.

Ruling: TEST03 usa una tabla de11 columnas pero entrega direcciones de4 bits.
El adaptador rechaza de forma explícita códigos>10 antes del acceso; su
comportamiento arcade para combinaciones físicamente contradictorias sigue OPEN.
No se inventa una normalización ni se lee fuera del array.

No se transfieren aún Intro/Entrada/Presentadora ni combate; FRT, CD compartido,
presupuesto dinámico de combate, propietario RNG y progresión siguen Task4/5.
