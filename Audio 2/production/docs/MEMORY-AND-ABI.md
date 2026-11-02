# Contrato de memoria y ABI

## Sound RAM — dirección local68000

| Intervalo, fin excluido | Propietario y tamaño |
|---|---|
| `00000..00400` | Vectores,1024bytes |
| `00400..03000` | Código/datos bajos; el linker impide cruzar los rings |
| `03000..0B008` | Cuatro rings PCM: cada uno4096 muestras16bit y un sentinel,8194bytes |
| `0B010..0B018` | Reloj DSP y constantes |
| `0B020..0B040` | Cabecera de transporte, generación y contadores históricos |
| `0B100..0B900` |256 registros de8bytes SH-2→68000 |
| `0C008..0C01C` | Interfaz de control histórica reservada |
| `0C040..0C060` | Mailbox de32bytes, ABI3 |
| `0C100..10000` | Código alto, tablas y BSS; límite estricto de linker |
| `10000..12400` |36 ciclos de seno matemático de128 muestras16bit,9216bytes |
| `12800..14000` | Pila68000,6144bytes |
| `14000..7F986` | SFX.DAT generado,440710bytes |
| `7F986..80000` |1658bytes libres; no se convierten en requisito de expansión |

Los huecos no asignados no autorizan solapar otro subsistema. La API rechaza el perfil histórico de onda en`0x70000`, que colisionaría con este pack. El ELF68000 debe tener exactamente tres PT_LOAD de este perfil y EM_68000/flags correctos; validación completa antes de cualquier copia.

La mezcla usa32 slots FM (8 canales×4 operadores), y cuatro voces PCM en el DSP. El programa DSP generado ocupa120 de128 pasos y12 operaciones RAM. No hay cuatro slots PCM adicionales. Los coeficientes/modelo de conversión no hacen al SCSP idéntico al YM2151.

## SH-2 y VDP1

| Perfil | Contrato |
|---|---|
| Diagnóstico estático | Código/recursos embebidos en HWRAM y compartido musical en`0x25C7F000`. Medir su propio mapa; no importarlo como master del juego. |
| Módulo ABI3 histórico | Loader de3 PT_LOAD y ruta histórica de copia/relocación; pertenece a los hashes de su master medido. |
| Loader ABI4 histórico | Exactamente4 PT_LOAD; incorpora una ventana caliente propiedad del master, revalida antes de escribir y no acepta el módulo ABI3. |
| Integración nueva | Debe enlazar y validar master+módulo conjuntamente; no basta cambiar el campo ABI. Toda reserva nueva debe quedar fuera del heap y de ambas pilas. |

El perfil ABI4 de TAG delimita programa VDP1`0x05C5C000..0x05C7C000`, BSS`0x05C7C000..0x05C7F000`, compartido`0x05C7F000..0x05C80000` (alias no cacheado`0x25C7F000`) y exactamente18KiB calientes en HWRAM. El renderer debe demostrar que no usa esos intervalos. No son RAM libre genérica.

Royal HW1 usa explícitamente otro perfil: programa frío en VDP2 `0x05E26000..0x05E3B000`, BSS `0x05E3B000..0x05E3E000` y compartido `0x05E3E000..0x05E3F000`. En la variante sin cartucho, los 18 KiB de código/estado caliente residen en la cola auditada de VDP1 `0x05C78020..0x05C7C820`; su pila permanece en HWRAM. La variante con cartucho reserva el bloque caliente en HWRAM. Esos contratos se seleccionan al compilar master, loader y módulo juntos; el mismo número ABI4 no permite intercambiarlos. Las direcciones, reservas del renderer, mapas y resultados negativos están en [ROYAL-INTEGRATION-GATE.md](ROYAL-INTEGRATION-GATE.md).

El estado caliente es3220bytes en el ABI SH-2. El código debe enlazarse en el mismo `workspace` que posee el master, incluso si VMA=LMA. La corrección de prepare comprueba esa igualdad antes de copiar/borrar. La ruta de arranque purga la cache del master y entra al slave mediante código no cacheado antes de habilitar su cache.

El perfil del juego necesita una pila slave independiente de8KiB y un límite de heap explícito. El ejemplo aislado usa los stacks del SDK y no certifica por sí mismo las reservas del master. No copiar esos símbolos sobre la asignación de memoria de un runtime existente.

## Presupuestos que se verifican y los que faltan

Se comprueban por compilación: segmentos Sound RAM, tamaño exacto de onda/vector, estado caliente, límite18432, tipos de ELF y símbolos sin resolver. Se conservan mapas, `.su`, hashes y comandos de compilación.

Se miden en ejecución: descartes individuales y por lote, rechazos de transporte, underruns32bit, contador de retrasos, máximos de preparación/refill/lanzamiento y estado de ambos procesadores. La instrumentación privada tiene coste observable y se contrasta con un perfil sin ella.

No se certifica un porcentaje de CPU ni un ancho de banda sin medida del mismo combate con/sin audio. El audio reserva un slave; la fidelidad no justifica ralentizar el juego. DMA no se habilita en el camino de muestras: las copias de arranque son explícitas, y no hay un beneficio DMA medido que autorice añadir contención. La cadencia de lógica objetivo(~57,44/s) se comprueba separadamente del refresco de vídeo.

El análisis del cambio de banco OKI ha identificado un resolvedor de direcciones y datos faltantes que requeriría 704 bytes adicionales de recurso. Un esquema de checkpoints estudiado ocuparía al menos 528 bytes, sin incluir su control ni garantizar plazo. Es una propuesta medida de almacenamiento, **no una reserva ni una implementación aprobada**. El motor actual todavía no cambia correctamente el banco de una voz activa. Véase [ACTIVE-OKI-BANK-AUDIT.md](ACTIVE-OKI-BANK-AUDIT.md).
