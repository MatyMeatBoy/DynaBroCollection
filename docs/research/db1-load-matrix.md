# DB1: matriz nativa de carga de 85 destinos (2026-11-18)

Los 85 destinos del catálogo DB1 pasaron la calificación de carga nativa: índices 0..7 y 20..96. Incluye 95 (EMI) y 96 (END). Se verificaron 255 aserciones de identidad de destino, 85 capturas PNG y 6.000 frames completos por proceso: 510.000 frames en total. No hubo operaciones AOT ausentes, instrucciones ilegales, fallos del guard de IRQ ni errores del harness en esta matriz.

## Identidades

- Ejecutable SHA256: `c196844369fd9b48d0af2bc045abe58b883d3889342d90d02c74f83df3e71f56`.
- Inglés actual CRC32: `2740C64E`.
- ROM de entrada SHA256: `44fe845490eadcbabe54747aea3c26b7153bef48eb343904230ec873f942f026`.
- El ejecutable, DLL y activos se copiaron a un directorio privado después del aviso READY explícito. Las pruebas no usaron un ejecutable que se estuviera relinkando. No hubo cambios de fuentes, build/dev ni ROM originales.
- Tras la matriz, el original JP conserva SHA256 `b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c` y la imagen suministrada v0.1 conserva `30ba0e06ce6a0704168d191cbb4d45487406908859f5df7aacd484bd42b0e7b7`.

## Método y resultado observado

Cada índice se ejecutó con perfil aislado, idioma inglés por defecto y visor fit, sin captura de audio ni overlays de host. Hubo un máximo de cuatro procesos de juego en paralelo y un timeout de 120 segundos por prueba. `DYNA_DB1_REPLAY_TEST_INDEX` recibió el índice en decimal; el módulo diagnóstico existente obtuvo el código de la tabla efectiva y lo introdujo por la pantalla Password. Los fixtures no contienen WRITE_RAM, cambios de PC ni parches de instrucciones.

Ruta de controles: WAIT 1999, START; WAIT 2000, START; WAIT 500; tres DOWN separados por WAIT 60; A; WAIT 500. Antes de la captura se exige:

- `FF0034 = index` del destino.
- `FF002E = 00FF` y `FF0030 = ADDE + 74 * index`, donde 74 es decimal (`0x4A`).

La captura se solicita en frame 5180 después de esas tres aserciones; el proceso continúa hasta 6000, aportando otros 820 frames. Se exige además estado de replay 5 (LOADED), finalización completa del benchmark e identidades exactas de ejecutable y ROM. El código de salida y los logs confirman que ninguna aserción falló antes de las capturas.

| Grupo de índices | Casos | Resultado |
| --- | --- | --- |
| 0..7 | 8 | PASS |
| 20..39 | 20 | PASS |
| 40..59 | 20 | PASS |
| 60..79 | 20 | PASS |
| 80..96 | 17 | PASS |
| Total | 85 | PASS 85, FAIL 0 |

Se releyeron todos los JSON, logs y PNG al terminar, independientemente del resumen del runner. El manifiesto registra SHA de entrada y fixture para reanudación; el informe de calificación registra también SHA de cada PNG, JSON y log. Se inspeccionó la hoja de contacto completa de los 85 destinos. Los tiempos por proceso fueron de 17,406 a 24,500 segundos, con mediana de 19,719 segundos en esta máquina.

## Alcance

Esta evidencia califica LOAD, identidad de stage y descriptor, captura y ejecución nativa finita de los 85 destinos. No demuestra campaña completa, victoria, recuperación de todos los estados, ausencia de todo defecto de gameplay, audio ni el final natural de los escenarios especiales. Algunos especiales pueden perder naturalmente durante el tramo posterior: eso no transforma la prueba de carga en una prueba de clear. Se publica sólo este informe textual, sin imágenes extraídas ni datos privados de ROM.

Evidencia privada retenida bajo `local/db1-load-matrix/`: `manifest.json`, `qualification.json`, `fixtures/`, `index-*/attempt-1/` y `all-85-private-contact.png`.

Reproducción resumible: `python local/db1-load-matrix/run-matrix.py`. Acepta sólo el SHA de ejecutable indicado y vuelve a verificar la evidencia previa antes de omitir un PASS.
