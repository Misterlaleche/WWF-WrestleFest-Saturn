# Assets reproducibles D03/E01

El generador conserva los sprites de 4 KiB y comprime las escenas con
`zopfli==0.4.3`, 15 iteraciones, `blocksplitting=1`, `blocksplittinglast=0` y
`blocksplittingmax=15`. El pack medido contiene las siete escenas canónicas,
102 bloques lógicos y 88 streams únicos. Los bloques idénticos siguen
compartiendo el mismo stream en orden de primera aparición.

| Contrato | Valor |
| --- | --- |
| `SCENES.WFP` | 250985 bytes |
| SHA-256 de escenas | `478e0b8be09aa69d65abb31ea849eae60c46e47fe285766dceaa302ad8e1dc2d` |
| Reserva de escenas | 253952 bytes; margen 2967 bytes |
| Caché con frontend | 593920 bytes; 145 bloques de 4 KiB |
| Caché sin frontend | 610304 bytes; 149 bloques de 4 KiB |
| Prefijos de música en cartucho | 114688 bytes con los sprites canónicos actuales |
| `SPRITES.WFP`, sin cambios | 3108052 bytes |

`asset-lock.json` identifica los assets derivados E01: 34 archivos y
632644966 bytes. Sólo cambia la entrada de `SCENES.WFP` respecto a
`docs/asset-lock-b12.json`, que conserva exactamente el lock anterior
(632662337 bytes; escenas de 268356 bytes). No se debe verificar un disco
anterior con el lock E01 ni reducir la reserva del runtime conservando el
pack antiguo. Esta identidad de assets no declara aceptado el rendimiento
del binario combinado.

## Regeneración desde un disco anterior

Ejecuta desde la raíz del proyecto. La carpeta de salida es privada para
mantener intacto un `cd/` que esté usando otra prueba:

```sh
python3 -m pip install -r requirements-assets.txt
python3 tools/extract_disc_assets.py /ruta/a/disco-b12.bin \
  --lock docs/asset-lock-b12.json --output-dir build/e01-assets/cd
python3 tools/build_assets.py --source-dir source_assets \
  --output-dir build/e01-assets/cd \
  --sprite-block-bytes 4096 --sprite-compressor zopfli \
  --scene-compressor zopfli --manifest build/e01-assets/manifest.json
python3 tools/extract_disc_assets.py --create-lock-from build/e01-assets/cd \
  --lock build/e01-assets/derived-lock.json
cmp asset-lock.json build/e01-assets/derived-lock.json
```

La importación recupera también WAV, SFX, `ROYINIT.BIN` y textos; el generador
gráfico sólo sustituye los packs y recursos gráficos canónicos. Los archivos
se reemplazan mediante renombrado, conservando los hardlinks de otras
variantes. Publica estos assets junto con el runtime que reserva 253952 bytes
para escenas. El manifest registra compresor, opciones y capacidades.

Para importar un disco E01 ya generado usa el lock actual:

```sh
python3 tools/extract_disc_assets.py /ruta/a/disco-e01.bin \
  --lock asset-lock.json --output-dir build/e01-restored/cd
```

## Restauración histórica sin regenerar

```sh
python3 tools/extract_disc_assets.py /ruta/a/disco-b12.bin \
  --lock docs/asset-lock-b12.json --output-dir build/b12-restored/cd
```

Este último directorio conserva el pack original. `--scene-compressor zlib`
está disponible como control, pero las escenas canónicas producen 268356
bytes y el generador las rechaza antes de escribir archivos porque exceden
la nueva reserva. Para reproducir un binario histórico utiliza su runtime
y los assets restaurados con su lock.

## Comprobaciones

```sh
python3 tests/test_scene_pack_builder.py
python3 tests/test_assets.py --source-dir source_assets \
  --sprite-pack cd/SPRITES.WFP --sprite-block-bytes 4096 \
  --build-dir build/e01-assets/native-tests --sanitize
```

La primera prueba compara el hash medido, repite la compresión, decodifica
las siete escenas, comprueba todos los alias y rechaza el pack zlib antiguo.
También verifica los defaults y las capacidades del manifest. Su prueba de
CLI reutiliza el pack de sprites existente; la función de compresión de
sprites permanece sin cambios. La segunda ejecuta el decoder C real con
los sprites existentes y las escenas recién generadas bajo sanitizadores.
