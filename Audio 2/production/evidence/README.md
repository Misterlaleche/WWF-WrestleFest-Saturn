# Evidencia publicada

Estos archivos contienen resultados reales, hashes, comandos y metadatos. Los WAV, ROM, trazas de notas, estados de emulador con RAM, objetos y discos se conservan privadamente. No se han sustituido fallos por resultados de otro perfil.

`profile10` identifica el firmware de SHA256 `b5ecee6e3e2a0efe7c13f1c6f4cb89bbc6b18cb96db23a57203b20e4a2b06b66`, antes de corregir el reloj heredado de A53 y las envolventes. Es evidencia histórica de esta sesión, no el certificado de la revisión más reciente del código.

| Archivo | Alcance |
|---|---|
| `host-profile10.json` | API, corrupción, alias de recursos, plazos y18×600s+fade contra R01, fuente estable durante la ejecución |
| `native-command01-profile09.json` / `profile10.json` |35s reales de un único comando, con/sin observaciones privadas |
| `queue-pcm-profile-history.json` | Mediciones causales de preparación, refill, visibilidad y fallback |
| `tag7372-profile10.json` | Combate real con/sin audio, mismo código/entradas, estado y productores; conserva sus fallos y el rendimiento insuficiente |
| `sfx-host-profile10.json` | PCM completo y98comandos contra modelo independiente; no es captura Saturn |
| `reference-regeneration-profileA53.json` | Reproducción autocontenida de los oráculos históricos, no aceptación del reloj físico |
| `modulation-depth-regression.json` | Prueba independiente del error de factor4 en el puente FM |

Los hashes de fuente en cada informe son los que delimitan su validez. Una modificación posterior exige las verificaciones correspondientes. Nunca se infiere `PRODUCTION READY` de que exista un informe con `passed=true` para un subconjunto.
