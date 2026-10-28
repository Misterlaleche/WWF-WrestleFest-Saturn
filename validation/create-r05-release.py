from pathlib import Path
import hashlib
import json
import shutil
import zipfile

root = Path(__file__).resolve().parent.parent
build = root / 'validation/local-20261028/eval-r05-build/results'
release = root / 'entregas/WrestleFest-Saturn-evaluacion-R05-20261028'
archive = release.with_suffix('.zip')
expected = {
    'wrestlefest-runtime18.bin': '51354bdd232bdb9ca70b61385490f80e149bd65d3a96b7fbfe015da20b53ab75',
    'wrestlefest-runtime18.cue': '331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14',
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

for name, digest in expected.items():
    assert sha((build / name).read_bytes()) == digest, name
assert 'wrestlefest-runtime18.bin' in (build / 'wrestlefest-runtime18.cue').read_text()
assert not archive.exists()
release.mkdir()
for name in expected:
    shutil.copyfile(build / name, release / name)

readme = '''WRESTLEFEST SATURN — EVALUACIÓN R05 — 28 DE OCTUBRE DE 2026

PARA PROBAR EN TU SATURN
Descomprime el ZIP y copia el BIN y el CUE juntos, sin cambiar sus nombres, a tu unidad MODE. Selecciona el CUE. Esta imagen está preparada para la Saturn americana NTSC a 60 Hz. Conserva la versión anterior por si necesitas volver a ella.

QUÉ HA CAMBIADO DESDE LA ÚLTIMA IMAGEN QUE PROBASTE
La comparación se ha hecho con wrestlefest-runtime18-audio-isolation.bin, la imagen de aislamiento de efectos con la que pudiste volver a jugar. Los recursos del disco y el firmware de efectos son los mismos; ha cambiado el ejecutable del juego.

• Se ha corregido una transición de animación enlazada en los estados 0B/0C al llegar al marcador FE, siguiendo el comportamiento de la ROM arcade. Conviene probar agarres y sus transiciones.
• Se ha ajustado una condición de contacto para la lucha fuera del ring, y se ha incorporado el marcador y la selección de lista de dibujo de esa escena. Comprueba especialmente las cuerdas y los sprites durante los cambios de escena. No se ha confirmado todavía que todos los fallos visuales del exterior estén resueltos.
• Se reutilizan datos de sprites y se evita borrar algunas tablas completas en cada fotograma. En una repetición de TAG la consulta de metadatos pasó de 45.402 a 20.376 llamadas, manteniendo iguales los registros de sprites comparados. Esto es una medición de trabajo interno, no una medición de FPS en Saturn.
• Se han ampliado las comparaciones con la ROM arcade, incluidas ramas del selector 00 y secuencias del 07 para distintos luchadores. Es trabajo de comprobación, no movimientos nuevos añadidos desde la imagen que probaste. Quedan ramas, situaciones y presentación final por verificar: no se considera certificado ninguno de los 143 selectores completos.

LO QUE SIGUE PENDIENTE
No se han modificado las pantallas del frontend respecto a aquella imagen: los puntos de colores, la ralentización al aparecer los retratos y las demoras de entrada pueden seguir presentes. El audio musical sigue aplazado; se conserva el firmware de efectos de la imagen anterior. La prueba automatizada no comprueba si los efectos se oyen correctamente.

VALIDACIÓN REALIZADA
La imagen se ha compilado para SH-2 y sus sectores y archivos se han verificado. En el emulador Beetle Saturn llegó al título y a combates TAG y Royal. En las ventanas observadas no se registró el error fatal vigilado. En Royal se observaron pantallas negras durante varios segundos antes de que la partida continuase: anota si también sucede en tu consola. No se ha probado esta imagen en una Saturn física y no se ha medido su fluidez en ella. Las pruebas automáticas de dirección quedaron interferidas por agarres y caídas, así que no certifican los controles.

QUÉ CONVIENE OBSERVAR EN TU SATURN
1. Inicio: tiempo desde SEGA hasta antidrogas, residuos o puntos de colores, velocidad de retratos y transición al menú.
2. TAG: desplazamiento arriba, abajo, izquierda y derecha; si tu luchador se gira hacia el rival; velocidad de presentación y combate. Prueba agarres, especialmente Hogan y DiBiase con cuatro luchadores en pantalla.
3. Royal: duración de pantallas negras, velocidad y si continúa sin congelarse. Apunta aproximadamente cuánto tiempo ha pasado si se bloquea.
4. Fuera del ring: carga de sprites y posición del luchador que permanece dentro respecto a las cuerdas. Si puedes avanzar, observa la victoria TAG y el siguiente escenario.
5. Indica si se oyen efectos. La música no es un objetivo de esta imagen.

Si aparece una pantalla de diagnóstico, fotografía todo el texto y anota los luchadores, el modo y la acción inmediatamente anterior. Si no aparece texto, describe en qué escena se quedó y qué sucedía justo antes.
'''
(release / 'LEEME-PRUEBA.txt').write_text(readme, encoding='utf-8')
checks = {name: sha((release / name).read_bytes()) for name in [*expected, 'LEEME-PRUEBA.txt']}
(release / 'SHA256SUMS.txt').write_text(''.join(f'{v}  {k}\n' for k, v in checks.items()), encoding='ascii')
with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for p in sorted(release.iterdir()):
        z.write(p, p.name)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for p in release.iterdir():
        assert z.read(p.name) == p.read_bytes(), p.name
report = {'status': 'PASS', 'folder': str(release), 'zip': str(archive), 'zip_sha256': sha(archive.read_bytes()), 'files': checks}
(build.parent / 'release-validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False))
