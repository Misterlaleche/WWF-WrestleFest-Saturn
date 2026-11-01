# Compilación y límites de reproducción

Primero ejecutar `python -B verificar_paquete.py`. No necesita recursos externos.

La versión consolidada se encuentra en `experimentos/native18k-standalone`:

```text
python tools/build.py --toolchain RUTA_SH2 --compiler-prefix sh-elf --sdk RUTA_SDK --rom RUTA_SOUND_ROM --master RUTA_MASTER_ELF --out build
```

Ejecutar desde ese directorio. El script verifica PIN.json y requiere el master exacto e56b027b… y la ROM completa de 65536 bytes
SHA256 f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7. Los objetos del módulo sólo incrustan sus primeros 48 KiB.
El standalone reproduce el prototipo fab67aa1…, que también falla el gate largo. No reproduce automáticamente la última variante bee32079….

La fuente de esa última variante está conservada en `experimentos/z80-inline-tabla/generated` y `src/audio/native`.
Sus scripts históricos conservan dependencias/rutas del laboratorio: adaptar el empaquetado antes de ejecutarlos en otro equipo.
No desactivar los PIN del standalone para hacer pasar una variante distinta por el original.

El módulo SH2 se compiló con GCC 9.3.1, usando libgcc del SDK 13.2.0. El runtime completo usa GCC13/ZSTD por la compatibilidad LTO del SDK;
no recompilar todo el juego con GCC9/Os: esa alternativa empeoró el control gráfico medido.
El firmware 68000 usa m68k-elf-gcc 9.3.1, -m68000 -msoft-float -Os, con límites de linker y comprobaciones de pila.
Las órdenes exactas e identidades se encuentran en los informes conservados.

Para reproducir simulación PCM también hacen falta el backend 68000 y las ROM originales usadas por las pruebas del laboratorio.
Los scripts de emulación requieren Beetle Saturn, BIOS y medio base externos. No descargan ni incluyen esos recursos.

No hay BIN/CUE/ISO en este paquete. No se autoriza a declarar una imagen lista por el solo hecho de compilar.
