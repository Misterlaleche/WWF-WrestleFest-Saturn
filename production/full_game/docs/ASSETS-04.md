# Checkpoint04 — carga común y autoridad de identidades

Continuación de Task4/5. No sustituir el estado ROM por cuatro IDs inventados.
La revisión de $5DA0/$64CC muestra que el paso siguiente a TEST03 requiere
selección de compañero, confirmación76ticks y RAM/D/A vivos. La intro exportada
termina sin resolver créditos ni transición: no inferir un salto a MODE.

1. Conectar el CDFS y transporte acotado de HW1, intactos, al único arranque.
2. Sacar los siete recursos TEST03 del ejecutable: precargarlos desde CD en
   una zona LWRAM, verificar tamaños/CRC32 contra los bytes aprobados antes
   de mostrar la escena. SHA256 de fuente/recursos sigue verificado al compilar.
3. API de carga con capacidad/alineación, fallo explícito y sin publicar
   punteros parciales. Pruebas host de límites, corrupción y fallos de transporte.
4. Vincular tabla ROM $64CC a la identidad confirmada, sin formar equipos.
   Contraste mediante ejecución68000 de la ROM; conservar auditoría de socios,
   Demolition y paletas para conectar el selector completo posteriormente.
5. Mismo disco None/1MiB/4MiB, cuatro recorridos de mando previos, cero lecturas
   CD durante selección. Disco con recurso corrupto debe fallar antes de MODE.
6. Evidencia, revisión, fuentes/disco/vídeos y checkpoint GitHub verificable.

Ruling: completar el lector compartido antes de enlazar Intro con selección;
la documentación de R04 prohíbe inferir créditos/transiciones desde su terminal.
Coste: no se añade aún ese recorrido, pero se conserva la lógica pendiente sin
una política de partida falsa. Propietario CD y LWRAM tienen un único dueño.
