# Colaborar

Empieza por [README](README.md), [STATUS](STATUS.md) y las notas del checkpoint. Mantén los cambios de lógica, traducción y presentación separados y revisables.

- No subas ROMs, recursos extraídos, código generado desde cartuchos, saves ni logs privados.
- Identifica el dump/edición con hashes; no aceptes un parche sólo porque aplique sin error.
- Los cambios de traducción necesitan contexto, límites de glifos/layout y comprobación visual de la escena.
- Conserva guards vivos, contratos de bus, excepciones y ciclos al optimizar AOT. Compara cargas iguales y reporta la dispersión.
- Usa fixtures sintéticos públicos para los contratos. La prueba privada de DB1 se omite si falta el dump; el CI público no certifica una partida.
- Ejecuta las pruebas Python y los CTests pertinentes. Para una entrega, prueba los binarios empaquetados con perfiles aislados y sin DLLs del entorno de desarrollo.

Para reportar fallos, incluye versión/build y una reproducción mínima. Revisa cualquier archivo antes de compartirlo; no envíes automáticamente una carpeta de diagnóstico completa.
