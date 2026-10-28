# Continuar WrestleFest Saturn en otro PC — 28 de octubre de 2026

Esta rama es una copia del desarrollo local posterior a `805be2a` y a la
imagen de evaluación R05. Lee primero `CURRENT-STATE-LATEST.md` y
`validation/local-20261026/ACTION-AND-CACHE-CHECKPOINT.md`. Conserva la barra
estricta en **0/143 selectores plenamente certificados**. Hay cambios de
colisión y diagnóstico posteriores a R05, probados frente a la ROM y en HOST;
no hay prueba en Saturn real ni BIN/CUE más reciente para esos cambios.

## Contenido

- Código actual de frontend, combate, utilidades y audio; la música sigue
  aplazada. Se incluyen pruebas, capturas acotadas de RAM, manifiestos, el
  inventario y un conjunto seleccionado de informes y trazas.
- La historia parte del `805be2a` y conserva los commits de desarrollo
  anteriores; esta rama no modifica la rama principal existente.
- No se incluyen las ROM, la BIOS, SDK, imágenes BIN/CUE, builds compiladas ni
  los millones de capturas intermedias. Algunos informes históricos citan
  ficheros locales de validación que no se copiaron; no deben tomarse como si
  todos sus artefactos estuviesen disponibles aquí.
- El archivo acotado `validation/local-20261028/action-source-r05/` contiene
  únicamente el fuente histórico necesario para comprobar el inventario. El
  archivo local completo de compilación R05 incluía ROM y recursos, por lo que
  no se ha subido.

## Preparar el entorno

1. Clonar el repositorio privado y seleccionar la rama
   `runtime18/avances-20261028`.
2. Proporcionar por separado `wwfwfest.zip` y, para ejecutar pruebas que lo
   requieren, el programa 68000 extraído como
   `src/runtime/audit/runtime18-cd/ROM68K.BIN`. Su SHA-1 debe ser
   `a726c46f350723ea73158bc78055d17d418b14e2`. La carpeta `audit/` se ignora
   en Git; no se debe añadir la ROM al repositorio.
3. Instalar Python, `machine68k` y `capstone` compatibles y un compilador C
   para las pruebas HOST. El inventario se puede comprobar con
   `python validation/action-selector-inventory.py` después de aportar el
   `ROM68K.BIN`. Las huellas de estas pruebas dependen de los bytes originales:
   la rama conserva sus finales de línea mediante `.gitattributes`.
4. Para compilar y ejecutar en Saturn hacen falta, además, el SDK Yaul/SH-2 y
   los recursos del juego generados desde los archivos autorizados; para un
   emulador hace falta una BIOS apropiada. `validation/build-runtime18.sh`
   documenta los ficheros que exige y valida varios por SHA-256. No se ha
   probado una reconstrucción completa a partir de este clon limpio: puede
   ser necesario aportar o regenerar recursos adicionales.

## Límites y siguientes tareas

Priorizar estabilidad, controles, rendimiento del frontend y combate, y las
rutinas/animaciones frente a la ROM. Mantener desactivada la música hasta
que se retome su integración. Los últimos bloques probados y el SHA-256 del
ELF interno se detallan en `CURRENT-STATE-LATEST.md`. El espacio medido para
heap fue 18.700 bytes, con reserva mínima de 16.384; verificarlo de nuevo al
introducir cambios. No interpretar pruebas controladas ni HOST como pruebas
de hardware o como porcentaje de juego terminado.
