# DynaBroCollection 0.5.5 para R36S

Segunda compilación de prueba para R36S con Linux AArch64. El port usa la ABI de Ubuntu 20.04, SDL2-compat y SDL3. Para mostrar el juego usa KMSDRM/EGL y para sonido usa ALSA del firmware. La alfa 2 reduce el trabajo de seguimiento de progreso durante la simulación e incluye herramientas de diagnóstico en `dynabrocollection/debug/`.

## Instalación manual

1. Extrae el contenido del ZIP directamente en la carpeta `ports` que tu EmulationStation ya usa. Los dos scripts deben quedar junto a la carpeta `dynabrocollection`.
2. Pon tus dumps japoneses compatibles en `dynabrocollection/roms/` con los nombres `db1.md` para Dyna Brothers y `db2.md` para Dyna Brothers 2 Special. Puedes instalar uno solo. No renombres un ZIP como `.md`.
3. Refresca la lista de juegos y abre el título desde **Ports**. Pulsa **Start + Select** para salir al frontend.

Los guardados, el progreso de DB1 y la configuración están en `dynabrocollection/user/`. Allí queda también el último registro de cada juego. Para cambiar el idioma a japonés, guarda `english=0` en `user/dynabrothers-language.ini` y reinicia el juego. El port arranca a pantalla completa con la proporción base del juego. Los menús de escritorio que requieren OpenGL se omiten. El menú Extras propio de DB1 continúa disponible desde su título.

En DB1 con el mando integrado y perfil automático, B cambia al control de cámara, A cancela y X o R2 aceptan en la batalla. Las configuraciones personalizadas de mando conservan sus asignaciones.

## Compilación

Genera los mismos módulos que usa el build de escritorio con `python tools/build.py` y los dumps locales admitidos. Prepara un sysroot AArch64 Focal con las bibliotecas de desarrollo de GCC 10 y usa Clang, LLD, CMake y Ninja. Especifica el sysroot con `DYNA_R36S_SYSROOT`. Usa SDL3 con KMSDRM, EGL/GLES y ALSA, y construye sdl2-compat contra esa instalación.

```sh
cmake -S . -B build/r36s/game -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=platform/r36s/aarch64-focal.cmake \
  -DCMAKE_BUILD_TYPE=Release -DDYNA_LAUNCHER_UI=OFF -DDYNA_R36S=ON \
  -DCMAKE_C_FLAGS_RELEASE='-O2 -DNDEBUG' -DCMAKE_CXX_FLAGS_RELEASE='-O2 -DNDEBUG' \
  -DSDL2_DIR="$SDL2_PREFIX/lib/cmake/SDL2" \
  -DDYNA_SDL2_RUNTIME="$SDL2_PREFIX/lib/libSDL2-2.0.so.0" \
  -DDYNA_SDL3_RUNTIME="$SDL3_PREFIX/lib/libSDL3.so.0" \
  -DDYNA_SDL_RUNTIME_LICENSES="$SDL2_LICENSE;$SDL3_LICENSE"
cmake --build build/r36s/game --target DynaBroCollection
cmake --install build/r36s/game --prefix build/r36s/stage --strip
python tools/package_r36s.py build/r36s/stage releases/DynaBrothers-v0.5.5-r36s-arm64-alpha2.zip \
  --screenshot /path/to/screenshot-640x480.png
```

Consulta las guías de [entorno de compilación](https://portmaster.games/contribute/porting/build-environments/) y [empaquetado](https://portmaster.games/contribute/porting/packaging/) de PortMaster. La prueba en QEMU permite verificar el ejecutable AArch64 y las rutas de arranque con drivers SDL simulados. En una R36S física quedan por comprobar el panel, el mando, el dispositivo de audio y los tiempos de cuadro.

La [guía de depuración](../../docs/r36s-debug.md) documenta los controles, la reproducción de problemas y la recopilación de logs.
