# Reproducción

Desde la raíz del repositorio extraído:

```
python3 validation/run-host.py src/runtime --cc clang --out validation/results/clang
python3 validation/run-host.py src/runtime --cc clang --sanitize --out validation/results/sanitizers
python3 validation/run-host.py src/runtime --cc gcc --out validation/results/gcc
```

El ejecutor compila las fuentes CORE una vez y enlaza/ejecuta cada target test del Makefile: 48 en este checkpoint. Requiere Python 3.9+ y compilador C11. Ejecutar GCC en Linux; en macOS /usr/bin/gcc es Clang y no cuenta como GCC independiente.

Para SH-2, extraer el archivo original `sh2eb-elf-yaul(1).zip` (SHA256 `8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d`) conservando permisos ejecutables, y ejecutar con Docker disponible:

```
sh validation/run-sh2.sh /ruta/absoluta/sh2eb-elf
```

El SDK incluido en ese ZIP contiene libyaul instalada. La imagen Linux está fijada por digest y el compilador suministrado es GCC SH-2 13.2.0. No se necesita reconstruir libyaul-develop para este enlace. El ejecutable resultante es exclusivamente un harness de enlace; no es una imagen jugable de Saturn.

Las auditorías de ROM en src/runtime/tools requieren el ZIP arcade original o el binario MC68000 reconstruido, según el argumento de cada script. No se distribuye otra ROM como autoridad. SHA1 MC68000 esperado: `a726c46f350723ea73158bc78055d17d418b14e2`.

GCC -fanalyzer se ejecutó sobre todos los .c de src/ y generated/ con `-std=c11 -Wall -Wextra -Werror -fanalyzer -O0 -Iinclude -c`. Los logs y versiones exactas están en el paquete de evidencias. Los tests pueden emplear dobles de callbacks para verificar contratos; no acreditan implementaciones externas ni gameplay completo.

## Actualización R02

El checkpoint actual tiene **48 tests HOST**. Los comandos HOST/SH-2 anteriores siguen vigentes.

Oráculos independientes y auditorías, desde la raíz extraída (Python 3.12 recomendado):

```
python3 -m venv .venv
.venv/bin/python -m pip install -r validation/requirements-rom.txt
.venv/bin/python validation/run-rom.py /ruta/main68k.bin /ruta/wwfwfest.zip --out validation/results/rom
```

El ejecutor recompila los probes nativos y compara TAG (28 casos), Royal (45), owner activo, frame activo completo (29 fronteras + ACK) y contrato de registros del render (144 casos y 18 rechazos). Ejecuta además 14 auditorías ROM y verifica que la fuente generada se reproduce exactamente. Los paquetes Python son solo herramientas de test; no se incluyen en el ELF.

El frame comprueba registros/RNG, no framebuffer. El contrato de render admite records simples y compactados sin enlaces; bloquea explícitamente las demás composiciones. Ver CURRENT-STATE-LATEST.md y docs/recovery-evidence, incluida la evidencia del frame en src/runtime/docs/recovery-evidence.

Para repetir el análisis estático con GCC Linux: `CC=gcc sh validation/run-analyzer.sh`. Se incluyen cuatro comparaciones ROM adicionales para los efectos de registros de $204B2 y el pago/refill $8B3A.
