# Toolkit de depuración de R36S

## Reproduce una escena

Guarda el binario y su SHA‑256, la versión del firmware o de la VM, el guion y la cantidad de cuadros. Para DB1, el guion `r36s-normal1-battle.input` atraviesa el arranque y entra en Normal 1 con las entradas originales. Las aserciones comprueban el índice y el descriptor del mapa. Registra al menos 7000 cuadros para incluir la batalla.

En el laboratorio Windows, la VM ARM64 completa sirve para compatibilidad de arranque, librerías y fallos de sistema. El modo Linux-user ejecuta el mismo ELF ARM64 con las bibliotecas de dArkOS sobre un kernel x86-64 virtual acelerado. Es el modo de juego interactivo más rápido en este PC. La R36S física completa la matriz con GPU Mali, pantalla, audio, mando y tiempos reales.

Si está disponible la carpeta local del laboratorio, usa `--archive` para elegir el ZIP exacto y `--qemu-dir` si QEMU no está en el PATH.

```powershell
py -3.12 tools/r36s_vm/run.py local/darkos-vm/dArkOS_RG351MP_trixie_11182026.img --archive releases/DynaBrothers-v0.5.5-r36s-arm64-alpha2.zip --preview-bench --userspace --paced --bench-frames 7200 --bench-input games/db1/scenes/r36s-normal1-battle.input
```

## Maneja el port desde el invitado

Arranca un invitado dedicado con el mando SDL virtual y un directorio nuevo para la sesión. Espera a que aparezca `ready.json`. `padctl` funciona con la ventana en segundo plano y devuelve la ruta del fotograma y de `guest.log` en JSON. Los nombres enviados son los botones impresos en la R36S.

```powershell
$sessionFolder = 'local/darkos-vm/db1-session'
py -3.12 tools/r36s_vm/run.py local/darkos-vm/dArkOS_RG351MP_trixie_11182026.img --archive releases/DynaBrothers-v0.5.5-r36s-arm64-alpha2.zip --play --userspace --virtual-pad --control-dir $sessionFolder
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

Para automatizar una prueba, agrega `--input-script ruta.input` y un límite de cuadros. Por ejemplo, `PRESS START 12` mantiene Start durante doce cuadros lógicos. Los guiones reproducen entradas del juego, mientras que la prueba del mando virtual atraviesa la API de SDL.

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

En un build que incluye las ventanas de temporización, cada registro `HOST-WINDOW` agrega 180 cuadros sin escribir a disco por cuadro. `HOST-HITCH` identifica las fases de los últimos eventos retenidos. La variante temporal para medir por separado la capacidad sin espera es `--runtime-uncapped`. Para una prueba de presentación real mantén el ritmo normal y el backend que usa el jugador.

## Lee y compara

```sh
python tools/r36s_debug.py referencia.log candidato.log --from-frame 5600
```

Usa los mismos inputs, firmware, backend y opciones en ambos registros. El analizador comprueba el tipo y longitud de las corridas y compara los hashes de estado y audio. Compara el arranque y la batalla por separado. La mediana de los bloques `R36S-LIVE` mide las presentaciones del invitado. Los percentiles de `HOST-HITCH` describen solo los eventos retenidos por el motor. Repite en frío y en caliente antes de atribuir una mejora a un cambio.

En el laboratorio, la prueba `--stress` permite aislar renderizado por software, cuotas de CPU, memoria limitada, DRM ausente, renderer inválido, presentación demorada y subidas de textura rechazadas. Usa la prueba en la R36S física para confirmar el ritmo del panel, la salida audible y el comportamiento del mando. Los logs de SDL y kernel ayudan a distinguir errores de arranque de una caída de cuadros en un nivel reproducible.

La [investigación de otros ports para RK3326](research/r36s-ports.md) reúne sus métodos de reproducción y medición.
