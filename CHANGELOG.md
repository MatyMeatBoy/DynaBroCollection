# Changelog

## 0.1.5 � checkpoint candidate

- DB1: c�mara coherente, introducci�n de nivel y submen�s extendidos, hazards continuos, fondos y logo del t�tulo integrados.
- Launcher verde �nico con settings en la misma ventana, DPI y navegaci�n por mando; descubrimiento de ROMs y parches, selecci�n expl�cita respetada.
- Replay de contrase�a nativo verificado; guardado/Extras completo y traducciones restantes siguen pendientes. DB2 contin�a pausado.

## 0.1.3 — checkpoint de prueba

- DB1: mundo ampliado a 16:9, HUD a la derecha y render independiente del buffer de 36 actores; considera todos los IDs válidos visibles y conserva las capas de cursor/selección.
- Original con presentación 4:3; Enhanced con lienzo 398×224 y escala entera por defecto. Adaptive acotado para otros aspectos de ventana.
- DB1: 76 metas locales basadas en su catálogo público y badges oficiales locked/color. DB2 Special: 93 etapas y seis hitos propios, con badges originales del segundo juego.
- Lista pausada de logros, popup con badge, cola FIFO y persistencia atómica por juego. Diagnósticos, scripts y replays excluidos del progreso.
- Test portable de DB1 con escenas, comparación de estado/audio Original/Enhanced y pruebas sintéticas de actores y prioridades.
- Continúan pendientes las traducciones completas, DB2 ampliado, campañas completas y hardware físico.

## 0.1.2 — en desarrollo

- Corregido el bloqueo de DB2 al despausar: interrupciones continuas en el stream AOT, con excepción de 44 ciclos y retorno/contexto del programa original.

- DB1: 18 nombres de créditos y mensaje secreto del final difícil en inglés.
- DB1: mensajes de completar práctica/modo alienígena y fondos de texto corregidos.
- Idiomas: se conserva la recuperación de japonés/inglés desde los dumps cualificados y las imágenes inglesas v0.1/v0.2.
- Versión compartida entre CMake, launcher y empaquetado; entregas históricas conservadas por versión/revisión.
- El gate de paquete añade las rutas preparadas de práctica, alienígenas y créditos. No certifica completar esos modos normalmente.

Traducciones completas, widescreen, modos Original/Enhanced y hardware físico siguen pendientes.

## 0.1.1 — checkpoint

### Añadido

- Launcher con dos tarjetas y ajustes compartidos por ambos juegos.
- Selección reversible japonés/inglés, con inglés activado por defecto.
- Perfiles M30 y overrides por dispositivo; fallback configurable para joysticks SDL.
- Caché de descriptores AOT con guards de instrucciones conservados.
- Empaquetado portable con DLLs, procedencia incrustada y hashes de la entrega.

### Corregido

- Seis etiquetas de mapas/rendición de DB1 que usaban índices japoneses sobre una fuente inglesa.
- Contrato de interrupción H-int, código RAM de DB2 y selección de región japonesa.
- Falsos positivos de los probes cuando no se guardaban las capturas solicitadas.
- Restauración de repeticiones que exigía mandos conectados y cambiaba sus preferencias.
- Validación de paquetes basada sólo en headers de build, en lugar del ejecutable copiado.

### Alcance

Es una entrega de desarrollo. Traducciones completas, widescreen ampliado, modos Original/Enhanced, aspecto de píxel final, campañas y hardware físico siguen pendientes. Detalles y mediciones en [notas del checkpoint](docs/releases/v0.1.1-checkpoint.md).


## 0.1.4 test checkpoint — 2026-11-18

Timer A audio correction; integer-fill canvas and DB1 scene/weather fixes; automatic SDL controller source and M30 bindings; Start+Mode menu with analog navigation; same-window full settings; themed main hub and external launch flags; Extras guide and atomic native-code progress store/replay. DB2 guarded renderer integrated, further DB2 development paused by user. See docs/releases/v0.1.4-checkpoint.md for qualification and limits.
