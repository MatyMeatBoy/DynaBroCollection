# Dyna Brothers 1 + 2 — auditoría inicial y propuesta

Fecha: 2026-11-17. Esta es la fotografía inicial anterior a la implementación; consultar STATUS.md para el estado actual. No hay traducción completa verificada.

## Evidencia local

Originales conservados en D:/01Claude/Dynabrothers. Espacio de trabajo: D:/01Claude/Dynabrothers1+2.

| Entrada | Tamaño | CRC32 | Resultado |
|---|---:|---|---|
| Dyna Brothers (Japan).md | 1048576 | 360C1B20 | Checksum MD correcto: 0CF5 |
| Dyna Brothers 2 Special (Japan) (Virtual Console).md | 2621440 | BA3554BC | Checksum MD correcto: 1438; declara J6 |
| Dyna Brothers (English patch v0.1).bps | 7636 | 2144DF1C | Fuente coincide; CRC interno del parche correcto |
| Dyna Brothers 2 (Japan) [English Translation] (2).ips | 176434 | AAFDDA38 | 158 registros; escribe hasta 0x400000; no contiene identidad de fuente |

Se decodificó el BPS del primer juego únicamente en memoria: salida 1048576 bytes, CRC32 C1E5C0DA coincidente con el CRC de destino; 9829 bytes distintos. Esto prueba integridad y aplicación binaria, no arranque ni calidad de traducción. No se guardaron ROMs modificadas.

El IPS debe investigarse antes de usarlo en Special: el nombre alude a la edición normal y el formato no permite certificar compatibilidad. Su expansión a 4 MiB no demuestra por sí sola incompatibilidad. Falta identificar autor, versión, ROM base y cambios de código/tablas.

## Requisitos recibidos

1. Prioridad: hacer funcionar traducciones y completar texto faltante en ambos, especialmente DB1. Idioma final pendiente de respuesta; entradas existentes en inglés.
2. Traducción independiente del modo, activada por defecto y desactivable. Mantener fuente japonesa inmutable.
3. Recompilación estática; identificar toda interpretación de respaldo y cobertura sin llamarlas recompilación completa.
4. Original: preservar reglas, ritmo, contenido y presentación 4:3. Confirmar raster y relación de píxel reales.
5. Mejorado: enfoque 16:9, mayor escala entera posible y extensión lateral del mundo; no estirar el cuadro. Ajustable a 1080p, 1440p, 4K y otras ventanas.
6. Revisar generación de terreno, cámara, HUD, clipping y descarte de sprites para mostrar personajes fuera del cuadro original sin alterar simulación.
7. Mandos de 3 y 6 botones; 6 como predeterminado. Confirmar protocolo y acciones de cada juego, sin inventar acciones extra en DB1.
8. Launcher y menú dentro del juego comparten ajustes persistentes, validación, controles y estado efectivo. Elegir modo debe conservar partidas y preferencias de idioma.

Escala entera uniforme y corrección exacta a 4:3 pueden diferir con píxeles no cuadrados: documentar ambas y priorizar la presentación 4:3 pedida para Original. No afirmar que toda resolución llena el panel mediante escala entera exacta.

## Enfoques para decidir

A. Recomendado: establecer primero las ROMs traducidas verificadas como referencia, completar extracción/reinserción y después integrar un runtime de recompilación fijado a commit. Reduce confusión entre errores del parche y del port. No exige decompilación completa previa.

B. Recuperar primero un desensamblado/decompilado específico y arrancar el port después. Facilita cambios profundos cuando existe una base fiable, pero retrasa el objetivo prioritario de traducción; todavía no se localizó una base específica verificable.

No se propone sustituir el objetivo por un launcher de emulador.

## Investigación y límites

Candidato: https://github.com/mstan/segagenesisrecomp (rama master, consultado 2026-11-17). Traduce 68000 a C y ofrece runtime compartido; documenta fallback interpretado. Evaluación de licencia, commit, cobertura por juego, audio/Z80 y proyectos donantes pendiente. Mega Drive y Genesis son nombres regionales de la misma plataforma.

No se encontró todavía un desensamblado/decompilado específico confirmado de estos dos juegos; la búsqueda inicial no prueba que no exista. Buscar y contrastar fuentes primarias, versiones y licencias antes de reutilizar código.

OptiPort consultado: MatyMeatBoy/OptiPort, commit 9673006504bf6712b320da70965bc3f65f2dd41a. Se sincronizaron SKILL.md y módulos de decompilación/localización conservando respaldos. launcher-experience.md existe en la instalación local pero no en ese commit remoto: se leyó como complemento local preservado, no como referencia upstream actual.

## Aceptación pendiente

- Arranque, menús, tutoriales, etapas, finales y textos gráficos inventariados por ID/contexto; ninguna afirmación de traducción completa sin inventario y revisión.
- Parche correcto/rechazo de fuente incorrecta, retorno a japonés y conservación de originales.
- Comparación de entradas y estados entre referencia y recompilado, incluyendo timings/audio y partidas prolongadas.
- Widescreen: pruebas de personajes y terreno a ambos lados, transiciones, cursor y HUD; demostrar que no cambia alcance de IA ni reglas.
- Mandos, reasignación, conexión/desconexión y apertura/cierre del menú sin entradas filtradas.
- Launcher empaquetado fuera del directorio de desarrollo y con ajustes compartidos.

Siguiente decisión: idioma final y enfoque A/B. Luego diseño técnico detallado y plan de implementación con evidencias por hito.
