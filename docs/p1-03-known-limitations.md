# P1-03 AUDIO CANDIDATE — limitaciones conocidas

- No se ha ejecutado esta imagen en una Sega Saturn física.
- No había Mednafen ni otro emulador Saturn instalado en el entorno.
- La mejora subjetiva de nitidez del título y la desaparición del ruido percibido
  durante Legion of Doom necesitan confirmación en hardware.
- Las músicas largas siguen siendo capturas finitas de 600 segundos; los puntos
  de bucle exactos del driver no se cierran en P1-03.
- P1-03 no pretende cerrar todavía todos los movimientos, animaciones,
  prioridades gráficas, recorridos completos de TAG/Royal, guardado ni
  rendimiento físico enumerados por la auditoría.
- La prueba automática de campaña TAG sin pulsaciones pierde el combate, como
  corresponde a una partida sin control. La regresión de progresión utiliza
  exclusivamente pulsaciones físicas producidas por el driver de prueba y
  alcanza correctamente el combate 2 sin escribir RAM ni progresión.
