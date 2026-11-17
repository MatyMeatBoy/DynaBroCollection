# Changelog

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
