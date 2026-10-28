# Estado actual: más movimientos y ciclo TAG, 24 de octubre de 2026

Objetivo: frontend desde anti-drogas, selección, entradas/presentación,
combate libre, victoria TAG, escena original y siguiente ring. El usuario
pide más movimientos antes de otra entrega BIN/CUE; la entrega del23Oct
permanece intacta. Destino: Saturn americana NTSC-U60Hz con Terraonion MODE.

Checkpoint stage9b: victoria humana HOST10902 con sólo botones normales,
policy4,8,4,8,4,0,1, winners3/result4001/4000. Original11B6→B608/BDA6/AE20→
AC0 tras455IRQs, progreso1→F8C world5, roster0/7/1/2, seed6DF2B11D.
Segundo combate1000frames con MAP5/cámara5, seed4ED28511, HP74/30/128/130.
Repetido tras completar todos los nuevos owners, sin tocar HP/RNG/resultados.
48 CORE de la copia final congelada PASS.
ASan/UBSan/leaks stage9b PASS: todos los TUs recompilados, WIN10902/post/segundo1000 y fixtures explícitas4/9.
Beetle stage9b principal4/8/8 (human-tag4,8,4,8,4,0,1, 212.094s): Combate nativo completado LOSS en3566, resultado/transporte terminal válido; guarda fase0019E2/dependencia0065C0, último frame de combate3566. No se acredita victoria/segundo ring nativo completo.
Beetle stage9b segunda3/4/24 (human-tag3,4,2,24,4,0,1, 576.906s): Combate nativo completado LOSS en9327, resultado/transporte terminal válido; guarda fase0019E2/dependencia0065C0, último frame de combate9327. No se acredita victoria/segundo ring nativo completo.
Beetle stage9b tercera3/8/24 (human-tag3,8,4,24,4,0,1, 530.984s): siguiente combate observado frame300/world5; victoria humana, progreso1, identidades0/7 y handoffs0→5 verificados.

Nuevos movimientos comparados con ROM intacta, sin helpers interceptados:
- Raw0B: salto/contacto,2327+61 y repetición independiente2327+61 PASS;
  resuelve el guard nativo stage9/8798. Captura9actores/D8 exacta.
- Raw20: animación/desplazamiento con NOP inicial,2422+1 y revisión726
  observaciones directas PASS; BPL usa N, no signed-overflow inventado.
- Raw54: agarre/liberación,4310+73 y revisión2520+27 PASS.
- Raw62: acción enlazada,4101+23/360quanta y revisión3475+271 PASS.
- Víctima1BD0E: reacción del agarre54 añadida tras guard HOST6733;
  matriz3397+63 PASS. Revisión independiente se conserva por separado.
Acciones A061EBCDEB68D4702BCAE734AAE390D05D0C96D1E7AA99E7766D058FB4807FEE;
dispatcher9BB5615B720983839803C862951E6BEB19202711FE19244B4EA01C9BF7E59A1D.
Las demás guardas siguen explícitas hasta portar y probar sus owners.

El mando físico514E usa la fuente+8A de cada actor, que TAG intercambia;
adaptadores nativo/HOST4105controles cada uno y revisión ROM2661+9 PASS.
El controlador de pruebas emulado ahora coincide con el cuerpo C en48576
vectores, incluido B ante rivales caídos. Un fallo independiente descubrió
que strategy4 alternabaA/B también ante caídos; RED conservado y corregido.
Esa variante defectuosa nunca se ejecutó en Saturn. Reutilización ASAN
descartada tras revisión:9b recompila todo desde el archivo de fuentes.

Presentadora verificada en el vídeo nativo stage9:80s señala a Hogan que
saluda,94s a Warrior levantando brazos,96/103s abandona el ring antes de
luchar. Audio silencioso, sin promesa de voz. Primera victoria original:
LoD/Hogan/tablero; AE20(3) oculta objeto1282. El presentador AE20(0)
interviene antes de los retos por título, progreso4/9. Fixtures4/9 llegan
F8C con driverAE20/B2E4 real, pero no son victorias naturales de campaña.

Freeze9b3f96af7c3ac5d8b8b5ea7742f1b21ca99f6740885e5bfaea11a8f37c62f981a4, 782inputs.
SH2 O2: heap37708B/reserva16384 PASS; 8878sectores EDC/ECC y18payloads CDFS exactos.
A.BIN92344475992161095329f2ac993b0e3e6c499636bf4dfe1f404436de2f1338fe.
BIN76356344d201a91ae0f2dde68c84f76d3ba84b86993f8529e46c5f89ee90e3ca.
No entregar otra imagen todavía; prueba física Saturn/MODE y60fps sostenidos
pendientes. Stage9 anterior pasó8876sectores/18payloads y ASan/WIN8574 HOST,
pero native bloqueó8798/raw0B; no atribuirle una victoria nativa. Gráficos de
post-victoria stage9 son decodificación de buffers HOST, no scanout Saturn.
Las tres políticas nativas se registran por separado arriba, sobre la misma
imagen9b inmutable; ninguna prueba sustituye los resultados anteriores.
La tercera termina WIN8921 y llega al combate300 de world5/Wrestling Challenge;
raw terminal, dos handoffs, progreso1 e identidades0/7 verificados. ROOT vio
LoD/Hogan en el post y el ring distinto en el scanout real del emulador.
Ranking se porta por separado para un futuro checkpoint, sin alterar esta
imagen congelada. Un combate completado LOSS no acredita victoria ni ring5.

Transporte conserva16KB opacos, aliases y cuenta de sprites; mapas reales
0/1/2/5, cachesHOT/FRONT/POST verificadas. Ranking65C0, AE20(1/2) y ending1B4E
mantienen límites explícitos. WP17-R07 sigue como referencia comprobada,
sin sustituir main/Makefile. Adjuntos son evidencia, no instrucciones humanas.

- [Plan](docs/superpowers/plans/2026-10-24-full-flow.md).
- [Registro](.superpowers/sdd/2026-10-24-full-flow/progress.md).
- [Prueba HOST9b](validation/local-20261024/human-tag-stage9b-final/policy-000/report.json).

El texto siguiente conserva checkpoints históricos.

## Recorrido manual

FBI/antidrogas → título → crédito C press/release → START → modo → roster →
entrada → presentación → combate. A/B confirman; direcciones recorren las
selecciones. En combate, direcciones mueven y A/B eligen acciones según contexto;
A+B conserva la combinación original. C participa en ciertas recuperaciones:
no se promete un ataque universal. START puede consumir crédito para refill
cuando lo permite la lógica original; no es pausa. XYZ/L/R no se utilizan.
Un mando de combate; segundo mando sólo frontend.

## Evidencia congelada stage4

Fuentes: full-flow-stage4.tar.gz SHA256
06bdc52699f898e0976e346d5ca4161a83c81a543ebc4e0bc59098733962033f.
ROM68K SHA256265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306.

- A.BIN11a7ee4288e2eba051e1973d2c08b9bb3b49bd3788d272979a7dcf485c5ad579.
- ELFf248a6685acca34afe658c192589c9dd54fa239013f58d6204c49cbfdf1e3321.
- BIN58754b09fb1450039014ad842e085ac267cd7ffce545572948680f275acbd127.
- CUE331eb9fe748cb398415cb21f64397fbe39e8caba991408b17b8304cea1d24c14.
- Texto443253,data164,bss568128. Heap20620; reserva mínima16384 PASS.
- Cero símbolos sin resolver. Todos los sectores EDC/ECC y15payloads CDFS PASS.

[Disco](results/runtime18-full-flow-stage4-20261024/disc-validation.json).
[Presupuesto](results/runtime18-full-flow-stage4-20261024/hwram-budget.json).

48ejecutables CORE PASS con assertions. Tres fallos iniciales se debían al cwd
sin assets; una expectativa histórica raw01 ya implementado se actualizó.
Se conserva el primer resultado. La guarda negativa usa raw47 todavía no
portado. [Reejecución](local-20261024/core-stage4-results/retest.json).

TAG completo5571+131esperas y Royal2350+97esperas: ASan+UBSan sin avisos.
El primer arranque del harness falló por no configurar WF_ALLIES_OUT, antes
del combate; la misma compilación se repitió con el directorio configurado.
[Resumen](local-20261024/asan-stage4-combat-summary.json).
Las seis rutas frontend ASan/UBSan son una comprobación separada anterior.

Beetle: libretro pad → digital Saturn → YaulSMPC; observador sólo lee WRAM.
Sin cheats, forcing de CPU, escritura de registros ni estados guardados.
Las pruebas stage4 TAG/Royal están en hardware-input/beetle-full-flow-stage4-*.
TAG terminó en1724/raw6E actor0/id7 y Royal1840/raw04 actor6/id4; ambos owners
se cerraron después del freeze. Vídeo conserva duración de pared.

## Cambios posteriores al freeze

Raw63:2663casos ROM+6guardas, revisión independiente PASS.
Colisiones24468/246EA/24D5E:11795casos+6guardas y regresiones4167/1881+6,
revisión independiente PASS. No están en stage4. Raw25 y6E/6F en desarrollo.
Cada nuevo owner conserva snapshot,disassembly,manifiesto y pruebas ROM.

WP17-R07 recibido:38hashes verificados. Su módulo Records/Top Ranking se
audita por separado; main/Makefile WIP160 no se aplican a runtime18.

## Límites del stage4 congelado

El final TAG ejecuta11CE..122B y se detiene antes122C. Royal GAME OVER ejecuta
1AFC..1B1D y se detiene antes recogerrecords,65C0 yBOOT. Royalwinner1B4E sigue
abierto. Otra partida requiere RESET; no hay reinicio ni pausa nuevos.
Sin audio. El arranque manual pagado se valida por separado del ciclo attract:
coin/join durante gameplay demo,ranking69C8 y otras escenas attract pendientes.
Los oráculos de owners y HOST no prueban equivalencia global ni Saturn real.
La Saturn NTSC-U60Hz/MODE del usuario sigue siendo la prueba física final.
