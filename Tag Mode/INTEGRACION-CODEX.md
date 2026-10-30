# Handoff para Codex — TAG Mode

Lee `Tag Mode/README.md`, `INDICE-CHECKPOINTS.md`, el README principal y la política de propietarios del repositorio. El dueño del combate permanece aislado: **no importar esta carpeta en el master por copiar ficheros o concatenar BIN/CUE**.

## Reanudación

1. Clonar el repositorio privado y trabajar en una carpeta nueva o rama propia. Mantener `Tag Mode/` independiente.
2. Usar las fuentes WP1979 en `WP1979-base/native/` como referencia persistida. Su origen es el bloque ya archivado `Combate real en Saturn`, no R6.
3. Descargar desde Library `wp2015-checkpoint-06.zip` y comprobar SHA-256 `321c23f6aebb576edb33effa8f71da3d61e45570405e97fc628b5de4f9aab405`.
4. Ejecutar `python3 "Tag Mode/tools/restore_wp2015_checkpoint.py" /ruta/wp2015-checkpoint-06.zip /tmp/wp2015-r3-native`. No escribir sobre repositorios existentes. Alternativamente descargar el archivo de fuentes R3 de Library, cuyo SHA está en el índice.
5. Continuar únicamente desde código efectivamente recuperado. Reconstruir y volver a medir toda corrección de R4/R5/R6 que no exista en fuentes persistentes, sin atribuir los resultados históricos a una compilación nueva.
6. Para cada checkpoint nuevo, subir fuentes, tests, scripts y manifiestos a GitHub **y verificar el HEAD publicado**; guardar BIN/CUE, vídeo y recursos binarios protegidos externamente con hash.

## Requisitos de rendimiento y fidelidad

- ROM arcade `wwfwfest.zip` es la autoridad. SH-2 para lógica; VDP1 para sprites; VDP2 para fondo/HUD; SCSP para audio nativo.
- Cuatro luchadores: Hogan, Warrior, DiBiase y Perfect, TAG. La selección, IA, estados, animaciones y audio no deben sustituirse por aproximaciones.
- Mantener estados de los luchadores equivalentes al arcade. Medir también ventanas lentas, picos de render, parpadeos, pérdidas de audio y comportamiento con mando.
- MAME/ROM para referencia de estados; Mednafen para las medidas de Saturn emulada; la validación física sólo existe cuando el propietario prueba el disco en la consola.
- Si falta una rutina ROM-exact, dejarla OPEN en lugar de inventar datos.

## Protección de trabajos anteriores

Los cambios quedan restringidos a `Tag Mode/`; nada de reescribir `main`, GAMEPLAY canónico, `modo Royal`, `Entrada Ring`, `Intro`, `Escena Presentadora`, WP1979 original o WP24.
