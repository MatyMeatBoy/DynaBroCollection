# Changelog

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
