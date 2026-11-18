# Seguimiento verificado: IRQ y progreso periférico en AOT de programa completo

La ampliación de una prueba de tutorial pausado a juego activo reprodujo un bloqueo: el programa esperaba un cambio de blank del VDP dentro de una interrupción, mientras el adaptador ejecutaba el handler atómicamente, fijaba blank y suspendía el avance del raster. Aumentar el guard de instrucciones habría ocultado el síntoma sin resolver la espera. La restauración completa de registros/stack desde el host también impedía representar cambios de contexto del propio juego.

La corrección limita el cambio al stream AOT de programa completo: solicitudes pendientes hasta un límite de instrucción, prioridad/máscara explícitas, ACK sólo del nivel aceptado y excepción mediante el núcleo MC68000 existente. Handler y RTE continúan como instrucciones compiladas ordinarias, con raster/Z80 avanzando. El núcleo conserva SSP/USP, frame, trace/mask y los 44 ciclos de entrada; no se agrega de nuevo deuda de IRQ ni una restauración de CPU desde el host. Los helpers acotados y el backend de funciones siguen separados.

Lección propuesta: al migrar de llamadas nativas por función a ejecución con PC explícito, volver a validar el contrato de interrupciones y tareas, no sólo la aritmética de instrucciones. Cubrir esperas de periféricos dentro de handlers, cambios del frame de retorno, IRQ anidadas, STOP y stacks de supervisor/usuario. Una escena pausada que arranca no demuestra juego activo.

Evidencia: seis contratos CTest, 38 pruebas Python, ruta de 18.000 cuadros con y sin mezcla de audio con hashes CPU/audio iguales, controles hermanos de DB1 y replay de las 18.000 ejecuciones sin diferencias. El paquete corregido exige once rutas, incluida la transición que antes fallaba. Audio acelerado/dummy no equivale a aceptación física ni prueba de pacing.

[Cambio integrado](https://github.com/MatyMeatBoy/DynaBrothers1-2/commit/c7fbc84) y [contrato/alcance](https://github.com/MatyMeatBoy/DynaBrothers1-2/blob/work/dynabrothers/docs/research/db2-continuous-native-irq.md). No se adjuntan cartuchos, capturas de assets ni logs privados.
