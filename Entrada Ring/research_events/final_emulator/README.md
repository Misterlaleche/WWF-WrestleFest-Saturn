# R02: audio nativo capturado en Mednafen

La captura del BIN identificado en `validation-summary.json` reproduce la secuencia nativa con continuidad y sin saturación. Se localizaron los 19 inicios PCM originales en cada una de las tres primeras vueltas. Dentro de cada vuelta, la separación temporal coincide con los eventos de referencia con una dispersión máxima de una muestra de 44,1 kHz (22,7 µs). La repetición sigue el periodo visual independiente de 424 fotogramas.

La correlación de la onda completa con el modelo nativo SCSP supera 0,988 en las tres vueltas; la correlación de las envolventes RMS de 50 ms supera 0,999. Las siete componentes del modelo (FM 0, 1, 2, 3, 4, 7 y OKI) dan coeficientes positivos en el ajuste de diagnóstico. La grabación tiene aproximadamente 1,5 dB menos RMS que el modelo de referencia; los archivos de entrega no se han normalizado para ocultar esa diferencia.

El primer tono nativo medible aparece a 13,443333 s del MOV completo. El origen de alineación del modelo es 13,420816 s; incluye el retardo de captura/modelo y no es una medición directa del inicio de la función en SH2. La elección del recorte visual corresponde al análisis de vídeo del proyecto.

Hay silencio digital exacto en ambos canales entre las muestras 1528827 y 1529008, intervalo [34,667278912; 34,671405896) segundos. Un final en 34,669 s permite cortar antes de la cuarta vuelta sin fundido ni modificación del audio.

Esto verifica una grabación de emulador y la ejecución del esquema nativo. No demuestra todavía el resultado eléctrico en Saturn físico, ni identidad entre el timbre FM del SCSP y el YM2151 original. La prueba host escribe a resolución de muestra y no representa el coste del bus ni el instante submuestra de cada IRQ. Los WAV de esta carpeta son referencias de validación; no forman parte del contenido reproducido por Saturn.
