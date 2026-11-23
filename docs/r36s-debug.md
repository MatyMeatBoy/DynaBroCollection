# Toolkit de depuración de R36S

## Reproduce una escena

Guarda el binario y su SHA‑256, la versión del firmware o de la VM, el guion y la cantidad de cuadros. Para DB1, el guion `r36s-normal1-battle.input` atraviesa el arranque y entra en Normal 1 con las entradas originales. Las aserciones comprueban el índice y el descriptor del mapa. Registra al menos 7000 cuadros para incluir la batalla.

En el laboratorio Windows, la VM ARM64 completa sirve para compatibilidad de arranque, librerías y fallos de sistema. El modo Linux-user ejecuta el mismo ELF ARM64 con las bibliotecas de dArkOS sobre un kernel x86-64 virtual acelerado. Es el modo de juego interactivo más rápido en este PC. La R36S física completa la matriz con GPU Mali, pantalla, audio, mando y tiempos reales.

Si está disponible la carpeta local del laboratorio, usa `--archive` para elegir el ZIP exacto y `--qemu-dir` si QEMU no está en el PATH. Sin `--archive`, el laboratorio toma la alfa más reciente que encuentre en `releases/`.

```powershell
py -3.12 tools/r36s_vm/run.py local/darkos-vm/dArkOS_RG351MP_trixie_11182026.img --archive releases/DynaBrothers-v0.5.5-r36s-arm64-alpha3.zip --preview-bench --userspace --paced --bench-frames 7200 --bench-input games/db1/scenes/r36s-normal1-battle.input
```

## Maneja el port desde el invitado

Arranca un invitado dedicado con el mando SDL virtual y un directorio nuevo para la sesión. Espera a que aparezca `ready.json`. `padctl` funciona con la ventana en segundo plano y devuelve la ruta del fotograma y de `guest.log` en JSON. Los nombres enviados son los botones impresos en la R36S.

```powershell
$sessionFolder = 'local/darkos-vm/db1-session'
py -3.12 tools/r36s_vm/run.py local/darkos-vm/dArkOS_RG351MP_trixie_11182026.img --archive releases/DynaBrothers-v0.5.5-r36s-arm64-alpha3.zip --play --userspace --virtual-pad --control-dir $sessionFolder
```

Desde otra terminal, consulta la pantalla antes de decidir qué pulsar. En DB1, `START` pasa la intro y una segunda pulsación abre el menú desde `PRESS START BUTTON`. Al probarlo, `DOWN` seleccionó `CONTINUE`, `UP` volvió a `NEW GAME`, `A` inició la primera batalla y `LEFT` movió la selección de la barra de herramientas. `START SELECT` salió del port y el invitado se apagó. Por eso conviene guardar las capturas de interés antes de pulsar esa combinación.

```powershell
$sessionFolder = 'local/darkos-vm/db1-session'
py -3.12 tools/r36s_vm/padctl.py --session $sessionFolder
py -3.12 tools/r36s_vm/padctl.py --session $sessionFolder START --hold 250 --settle 750
py -3.12 tools/r36s_vm/padctl.py --session $sessionFolder DOWN --settle 230
py -3.12 tools/r36s_vm/padctl.py --session $sessionFolder START SELECT --hold 600 --settle 100
```

`--hold` controla la duración de la pulsación y `--settle` la espera antes de capturar, en milisegundos. Para capturar una transición, sincroniza la pulsación con la escena y usa una espera corta. El invitado registra las pulsaciones como `[R36S-PAD]`, con `R36S_PREVIEW_SDL_START_OK` como comprobación adicional al pulsar Start por primera vez.

El mando virtual usa un mapa de eventos observado en la R36S y lo pasa a SDL. En DB1, el B inferior cambia al control de cámara, el A derecho cancela y R2 acepta. La alfa 2 también acepta con X en la batalla al usar el perfil automático. La cruceta y Start conservan sus funciones. `padctl` admite `Y`, `SELECT`, `L1`, `L2`, `R1` y `FN`. Los ejes analógicos quedan centrados y el firmware puede aplicar su propio mapa. Comprueba los eventos del mando y los atajos del sistema con la consola física. [Mapa de entradas de R36S](https://github.com/dov/r36s-programming#mapping-of-the-r36s-controls)

En juego manual, el visor acepta flechas para la cruceta, Enter para Start, F8 para capturar y Esc para salir. Con `--virtual-pad`, Z/X/C corresponden a los botones impresos A/B/X, Backspace a Select, A al botón superior y S/D a L1/R1. Con el teclado directo, Z/X/C conservan las asignaciones de Genesis. El visor también admite los botones digitales del primer mando XInput conectado a Windows.

El título muestra los FPS que presenta el juego dentro del invitado, su porcentaje respecto al objetivo indicado por el juego y los FPS que dibuja el visor. Por defecto, el puente transmite uno de cada dos fotogramas, por lo que cerca de 30 FPS del visor pueden corresponder a 100 % en el invitado. Agrega `--preview-fps 60` para enviar cada cuadro en el modo Linux-user. El visor conserva el fotograma más reciente y su tasa efectiva depende del equipo. El título se actualiza una vez por segundo y toma el último bloque de 180 presentaciones del log del invitado.

Para automatizar una prueba, agrega `--input-script ruta.input` y un límite de cuadros. `PRESS START 12` mantiene Start durante doce cuadros lógicos. Con el mando de seis botones seleccionado, el guion también admite `X`, `Y`, `Z` y `MODE`. Los guiones reproducen entradas del juego, mientras que la prueba del mando virtual atraviesa la API de SDL.

## Recoge evidencia local

En Linux, ejecuta el colector dentro de la carpeta del port. En una instalación anterior, copia `platform/r36s/collect-debug.sh` a `dynabrocollection/debug/collect.sh` y dale permiso de ejecución con `chmod +x debug/collect.sh`.

```sh
./debug/collect.sh
```

El resultado incluye el kernel y ABI, firmware, memoria, modos DRM, frecuencia y gobernador si están expuestos, temperaturas, tarjetas ALSA, capacidades de joysticks y mensajes de kernel filtrados. Copia el último log de cada port cuando existe. El directorio es local y excluye ROMs y partidas.

Para un diagnóstico de ritmo de DB1 en la consola, desde `dynabrocollection`:

```sh
profile="$(mktemp -d)"
LD_LIBRARY_PATH="$PWD/lib" GENESIS_USER_DIR="$profile" \
  SDL_VIDEO_DRIVER=kmsdrm SDL_AUDIO_DRIVER=alsa GENESIS_HOST_TIMING_DUMP=1 \
  ./DynaBrothers1AOT --no-launcher --fullscreen --widescreen off \
  --measure-runtime 7000 --input-script /ruta/r36s-normal1-battle.input "$PWD/roms/db1.md" \
  >"$profile/db1-perf.log" 2>&1
printf '%s\n' "$profile/db1-perf.log"
```

Con `GENESIS_HOST_TIMING_DUMP=1`, el port mantiene en RAM las últimas 128 ventanas completas de 180 cuadros y las vuelca al terminar la sesión. `HOST-HITCH` conserva los últimos 128 eventos. `HOST-WORST` guarda el cuadro más lento de toda la sesión, incluido el desglose por fase y los contadores de audio, aunque haya salido del historial. `HOST-PERCENTILES` resume todos los cuadros medidos, incluido el final de una ventana incompleta. Los percentiles son límites superiores con resolución de 512 µs. Un valor `-1` indica que el percentil cae en el intervalo abierto a partir de 261,632 ms. Si el proceso se interrumpe abruptamente, los datos pendientes en RAM se pierden. Otras trazas detalladas activadas por separado pueden escribir durante la captura.

La variante para medir la capacidad sin espera es `--runtime-uncapped`. Para una prueba de presentación real mantén el ritmo normal y el backend que usa el jugador. Los intervalos de la modalidad normal incluyen espera de ritmo. Para estimar el coste del diagnóstico, repite con las mismas entradas y tiempos de reloj, alternando trazas desactivadas y activadas.

En estas líneas, `deadline_wait` mide la espera explícita del limitador en la ruta estándar. La etiqueta histórica `pacer` incluye el intercambio de buffers de SDL y trabajo asociado, incluso sin tope. `present` mide el trabajo posterior a esa fase. El intervalo medido termina antes de los diagnósticos de final de cuadro.

Si activas también `GENESIS_HOST_SWAP_TIMING=1`, `HOST-SWAP` aísla el tiempo de pared y el tiempo de CPU del hilo dentro de `SDL_RenderPresent` en la ruta estándar. Una diferencia sostenida señala tiempo fuera de CPU durante la llamada. Esta medición agrega llamadas al reloj y se reserva para la calificación. El modo de presentación interpolada de alta frecuencia tiene su propia ruta y queda fuera de estos contadores.

Con presentación normal y diagnóstico activo, `deadline_wait` mide la espera del reloj y `pacer` mide el resto del tramo, incluida la llamada de presentación. En logs anteriores y en el modo de alta frecuencia la espera queda incluida en `pacer`.

Los bloques recientes incluyen `max_us` y el número de cuadros de 25 ms o más. Revisa el primer cuadro por separado para distinguir inicialización de tirones durante el juego. `HOST-SLOW-SIM` conserva en RAM los 64 cuadros más costosos cuya simulación alcanza 24 ms y los imprime al salir. Registra el PC, las instrucciones retiradas y el tiempo de CPU del hilo. Un `cpu_us=0` indica que el reloj del hilo no está disponible. `HOST-SLOW-SIM-SUMMARY` indica cuántos cuadros pasaron el umbral. El cronómetro de la prueba se detiene antes del volcado. Los ensayos con ritmo normal en la VM también registran cargas de DB1 (`DB1-LZ`) y llamadas lentas del puente gráfico (`R36S-PRESENT-HITCH`).

Cuando el tirón se repite en el mismo cuadro, `GENESIS_R36S_TRACE_RASTER_FRAME=4974` registra el tiempo dedicado al 68000, al Z80, al vídeo y a las interrupciones durante ese cuadro. Cambia el número para otro caso. El muestreo por línea puede alterar esa medición, así que compara la misma configuración antes y después.

En el laboratorio Windows puedes pasar el mismo número con `--trace-raster-frame 4974` en una prueba `--userspace --preview-bench` de más de 4974 cuadros. Busca `[R36S-RASTER]` en el log del invitado.

La marca `[R36S-68K-PEAK]` identifica la línea más costosa del 68000 dentro del cuadro elegido. Incluye el tiempo, el número de instrucciones retiradas y los PC de entrada y salida. Úsala junto con el total del 68000 para distinguir un tramo aislado de un costo que se reparte a lo largo del cuadro.

## Lee y compara

```sh
python tools/r36s_debug.py referencia.log candidato.log --from-frame 5600
```

Usa los mismos inputs, firmware, backend y opciones en ambos registros. El analizador comprueba el tipo y longitud de las corridas y compara los hashes de estado y audio. Compara el arranque y la batalla por separado. La mediana de los bloques `R36S-LIVE` mide las presentaciones del invitado. Los percentiles de `HOST-HITCH` describen solo los eventos retenidos por el motor. Repite en frío y en caliente antes de atribuir una mejora a un cambio.

En el modo `--preview-bench --userspace`, `--no-frame-stream` mantiene el renderizado en el invitado y desactiva la copia de píxeles hacia Windows. Compara con una pasada del mismo ZIP y guion que omita ese argumento. Comprueba `received_bytes` y los hashes para confirmar qué trabajo se eliminó y que ambas pasadas completaron la misma carga.

En el laboratorio, la prueba `--stress` permite aislar renderizado por software, cuotas de CPU, memoria limitada, DRM ausente, renderer inválido, presentación demorada y subidas de textura rechazadas. Usa la prueba en la R36S física para confirmar el ritmo del panel, la salida audible y el comportamiento del mando. Los logs de SDL y kernel ayudan a distinguir errores de arranque de una caída de cuadros en un nivel reproducible.

La [investigación de otros ports para RK3326](research/r36s-ports.md) reúne sus métodos de reproducción y medición.

### Prueba experimental del ejecutor 68K de DB1

Configura con `-DDYNA_DB1_REGISTER_EXPERIMENT=ON` para comparar el ejecutor de registros con la ruta habitual. La compilación clasifica los cuerpos C de ambos catálogos de DB1 con el verificador del motor. Las operaciones que necesitan el bus, las excepciones y el modo de traza siguen usando la ruta general. El ejecutor rápido verifica en vivo el opcode y sus extensiones. El interruptor viene desactivado.

En dos pares de recorridos de 6.200 cuadros hasta el combate de Normal 1, bajo QEMU Cortex-A7 con renderizado por software y sin tope, el promedio pasó de 79,83 a 86,12 FPS en el primer par y de 81,29 a 83,20 FPS al invertir el orden. El P95 global bajó en ambos pares. Las huellas de juego y audio coincidieron en todas las pasadas. La réplica también comparó las huellas de imagen de los cuadros 3.100 y 6.200, que coincidieron. El máximo del tramo de combate fue 22,45→18,99 ms en el primer par y 26,21→32,19 ms en la réplica. El promedio de combate empeoró en ambos pares. Estos datos mantienen el cambio en fase experimental.

En DB1, `DYNA_DB1_POLL_TAILS=1` habilita otro experimento. Agrupa la prueba de bits o la comparación y el salto de dos bucles de espera. Cada lectura del VDP o de RAM sigue pasando por la ruta habitual. El motor exige que coincidan los descriptores y las palabras ejecutadas y que el tramo termine antes del próximo límite de la CPU. Una interrupción pendiente, el bit de traza o una discrepancia conserva la ejecución individual.

En la primera comparación de 6.200 cuadros con el mismo binario, activar la agrupación elevó el promedio de 82,32 a 86,95 FPS y bajó el P99 de 17,92 a 16,90 ms. En los cuadros 5.220–6.119, el peor cuadro pasó de 31,89 a 19,93 ms. Al invertir el orden, la referencia sufrió un pico de 272,14 ms fuera de combate y se ejecutó más lento. La cifra de esa réplica tiene más variación ambiental. Los cuatro recorridos conservaron las huellas de estado y audio, además de las de imagen de los cuadros 3.100 y 6.200. El experimento está desactivado de forma predeterminada.
