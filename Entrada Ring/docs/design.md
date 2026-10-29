# WP01 R02 — audio SCSP nativo

## Encargo y límites

El usuario pide intentar audio nativo en la entrada TAG ya entregada. Se conserva la R01, todas las fuentes anteriores y todos los repositorios. La nueva revisión vive exclusivamente en `wp01_audio_r02/`; sus entregables llevarán R02. Debe mantenerse la escena gráfica original y su cadencia, y entregarse BIN/CUE, vídeo de emulador y fuentes para probar en Saturn NTSC física.

## Arquitectura elegida

La ROM original determina las notas, parámetros, cambios y disparos. Una captura de escrituras realizada en MAME con las mismas entradas originales proporciona el reloj de los eventos. La música 0E utiliza seis canales FM de cuatro operadores; los canales audibles 0,1,2,3,4,7 se asignan a 24 slots SCSP. Los cuatro slots siguientes corresponden a las cuatro voces OKI, aunque esta escena utiliza principalmente las voces 0 y 2. Las muestras OKI se decodifican fielmente desde ADPCM a PCM16 para que el SCSP las reproduzca individualmente.

Los operadores FM se generan en el propio SCSP con osciladores sinusoidales, modulación, realimentación y envolventes. La conversión de parámetros se basa en los manuales y modelos de los chips. No se incluye la mezcla grabada de la escena ni una grabación de cada canal musical. Un secuenciador de escrituras a registros reproduce las órdenes de la ROM con temporización inferior a un fotograma. El SH-2 dirige registros; no sintetiza ni mezcla audio muestra a muestra.

Se descartan la pista PCM mezclada (R01) y la emulación software completa de YM2151 en el camino crítico gráfico. El 68000 de sonido puede añadirse posteriormente; esta revisión utilizará el temporizador del SH-2 para evitar una nueva dependencia de compilador y mantener el trabajo medible.

## Fidelidad y validación

Los chips son distintos: las envolventes, la cuantización de tono y la modulación requieren traducción y pueden dejar diferencias audibles. R02 no se calificará como audio idéntico a la ROM por el mero hecho de sonar o de reproducir los eventos correctos. Se compararán eventos, notas, voces, niveles, duración, espectro y captura real del emulador. Se informarán las diferencias que sigan presentes.

Las pruebas incluyen reproducción, reinicio, cambio de pareja, límites de RAM de sonido, ausencia de clips fuertes o voces colgadas, integridad de la escena gráfica y coste de interrupciones. La prueba física seguirá pendiente del usuario.
