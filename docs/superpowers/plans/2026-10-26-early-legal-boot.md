# Mostrar antes la pantalla antidroga

## Decisión

La pantalla antidroga debe aparecer antes de cargar audio y recursos del
combate. Se conserva la secuencia arcade de las tres placas y la puerta de
visibilidad que evita residuos. Alternativas evaluadas: una placa estática
duplicada adelantaría más el primer píxel, pero duplicaría gráficos y lógica;
acelerar solamente el audio ya redujo su lectura y aún deja unos 330 VBlanks
negros. Reordenar el arranque existente es la opción elegida.

## Trabajo

### 1. Dividir el arranque normal en placa legal y recursos diferidos

**Archivos:** `src/runtime/src/wf_saturn_entry.c`,
`src/runtime/src/wf_saturn_frontend.inc`.

- Mostrar el primer mapa legal con vídeo, fuente y CDFS inicializados.
- Omitir `wf_r18_scsp_service()` hasta que termine el arranque de SCSP.
- Después de `legal_first_visible`, cargar SCSP y SFX usando
  `WF_SATURN_LWRAM_SCENE`, mientras el mapa legal permanece visible.
- En el arranque normal no cargar ni enlazar `R18RING.BIN` previamente:
  `frontend_world_bind` ya carga el pack seleccionado cuando lo necesita.
- Mantener la carga inicial de audio y ring del modo de prueba directa.

**Comprobación:** la emulación debe mostrar la placa legal antes de la carga
de SCSP; `legal_first_visible` debe bajar respecto de 834 VBlanks. Las
transiciones antidroga → copyright → Technos y la demo deben continuar sin
excepción ni restos del ring.

### 2. Verificar disco y estabilidad

**Archivos:** compilación interna en `validation/local-20261026/`.

- Compilar de cero, verificar presupuesto de RAM, pilas y sectores del disco.
- Emular un ciclo completo de frontend y al menos 500 fotogramas de demo.
- Registrar los tiempos de primera placa y del final de la carga de audio.

La BIN/CUE entregada `WrestleFest-Saturn-frontend-r01-20261026` se conserva
intacta para la prueba física del punto 2 del frontend.
