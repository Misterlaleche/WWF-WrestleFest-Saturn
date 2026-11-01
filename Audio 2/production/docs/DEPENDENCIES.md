# Dependencias, licencias e identidades

## Separación de runtime y herramientas

El ejecutable Saturn no enlaza Nuked-OPM ni Mednafen. Usa código del proyecto, el núcleo Z80 y Yaul, con helpers aritméticos compilados para el68000 básico. Las referencias de audio y análisis espectral se ejecutan en el host. Los datos originales y BIOS los proporciona localmente el propietario.

| Dependencia | Versión/identidad | Licencia y uso |
|---|---|---|
| Núcleo Z80 de Nicolas Allemand | Copia histórica identificada por SHA256: `z80.c` = `d82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698` | MIT; texto en `src/reference_driver/LICENSE`. Se conservan la copia de referencia y las variantes optimizadas. No se inventa un commit upstream no recuperado. |
| Yaul/SDK suministrado | Archivo `13-sh2eb-elf-yaul-1-.zip`, SHA256 `8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d` | Yaul MIT; el SDK incluye licencias independientes de GCC/binutils y utilidades. El ZIP identifica la versión exacta usada. |
| GCC SH-2 | 13.2.0; hash del binario en `tools/environment-manifest.json` | GPLv3; runtime con excepciones correspondientes. |
| Binutils SH-2 | 2.40 | GPLv3; herramienta de compilación. |
| GCC68000 |13.3.0, paquete Ubuntu fijado | GPLv3; el enlace básico usa el helper explícito siguiente. |
| Binutils68000 |2.42, paquete Ubuntu fijado | GPLv3; herramienta de compilación. |
| `lb1sf68.S` | GCC13.3.0, SHA256 `851f082a547137b70dfe686764c9312aed489df0ff00d60e2dd6c6fe64d5ba00` | GPLv3 o posterior con GCC Runtime Library Exception3.1; ambos textos incluidos. `-m68000`, sin depender de un libgcc accidentalmente construido para68020. |
| Nuked-OPM | Commit `f209e6ed3712032b641d53ce8fb24824eae6adc3` | LGPL2.1 o posterior, según los encabezados de los archivos fijados; sólo herramientas del host. El generador conserva sus avisos originales. |
| Mednafen |1.32.1; archivo fuente SHA256 `de7eb94ab66212ae7758376524368a8ab208234b33796625ca630547dbc83832` | GPL2 o posterior y licencias de los componentes incluidas por upstream; sólo ejecución/validación externa. |
| Python |3.12.14 en el entorno de ensayo | PSF; scripts de generación y captura. |
| NumPy |2.3.5 en el análisis realizado | BSD3; métricas, nunca genera la salida Saturn. |
| SciPy |1.17.0 en el análisis realizado | BSD3; espectros y correlaciones, nunca genera la salida Saturn. |
| Xvfb/X11 y utilidades Linux |127 paquetes con nombre, versión, URL y SHA256 fijados | Licencias por paquete; textos y hashes en `tools/environment/licenses/packages/licenses.json` y sus archivos `copyright`. El wrapper sólo reubica la búsqueda de xkbcomp, no modifica Mednafen. |

Las versiones de NumPy/SciPy y sus módulos se registran en cada comparación; diferencias de entorno deben conservar una nueva identidad de análisis. Las señales WAV no se normalizan ni se reajustan para alcanzar una métrica.

Los archivos completos fijados de Nuked tienen SHA256:

- `opm.c`: `983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188`.
- `opm.h`: `186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39`.

Las identidades de archivos ejecutables del entorno y de las dos reconstrucciones de Mednafen están en `tools/environment-manifest.json` y `tools/environment/verification.json`. La reconstrucción en otra ruta puede producir otro hash de ELF por metadatos de compilación; se registra ese resultado en vez de afirmar reproducibilidad binaria no demostrada.

El instalador de entorno comprueba los hashes antes de utilizar cada descarga. `tools/environment/manifests/packages.json` y `sources.json` contienen el inventario verificable. No se redistribuyen aquí ROM, BIOS, SDK binario del propietario ni capturas protegidas.
