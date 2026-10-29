# Estado de entrega R02

Implementación nativa, integración y verificación terminadas. El candidato final es `disc/candidate/WF-WP01-R02.bin`, SHA256 `eac02ac3b84c9338fff20856e3b11d4b38421b7cdd836850acd95bd2eb9deee2`.

El firmware probado genera seis voces FM mediante operadores SCSP y reproduce dos tipos de muestras OKI originales. No usa una pista musical PCM. Las fuentes originales, R01 y repositorios permanecen intactos.

Correcciones incorporadas y comprobadas: interrupción compare-A local compatible con BIOS real/HLE; protección de KYONEX antes de modificar los bits de control de slots; lectura FRT de bytes altos/bajos con verificación de acarreo compatible con el latch físico y Yabause; reloj FRT nominal NTSC corregido a `120093750/143 Hz`.

Las comprobaciones de host, escena original, tres repeticiones y reinicios en emulador están terminadas. La captura Mednafen usa el BIN final, BIOS USA real y ningún cartucho de expansión. El vídeo conserva los tiempos nativos y usa el sonido de esa captura.

Pendiente de evidencia externa: prueba en Saturn físico. Pendiente de fidelidad, fuera de la afirmación de completitud de esta primera versión nativa: acercar más envolventes, realimentación y timbres SCSP a YM2151. No se afirma identidad de audio con el arcade.

El trabajo futuro debe continuar en una revisión nueva independiente; no sobrescribir R01, esta entrega verificada ni repositorios existentes sin autorización del usuario.
