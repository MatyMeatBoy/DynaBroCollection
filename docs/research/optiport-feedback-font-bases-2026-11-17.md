# Seguimiento: una misma fuente puede tener varios contratos de índices

Aplicado con OptiPort `58843a2c9122720f5db8649c0d9cfdb60dc479cf`.

Al ampliar la traducción de DB1 se reprodujeron tres contratos distintos para la misma fuente inglesa: índices directos en créditos, bytes de texto convertidos a otra base de tiles en mensajes especiales y una tercera base para el fondo desplazándose. Reutilizar ASCII o la codificación del primer renderer producía glifos incorrectos en los otros. Incluso la puntuación requería mirar los tiles cargados; el primer borrador mostró el glifo equivocado en una captura.

Lección propuesta para localización de fuentes indexadas: recuperar por renderer la transformación byte → índice → tile cargado, incluyendo blancos, puntuación y terminadores. Una fuente compartida no implica una codificación compartida. Validar el resultado en cada contexto y preservar las capacidades y tablas originales cuando se pueda evitar cambiar código del juego.

La corrección mantiene hash de origen, guards por tabla, dimensiones, checksum, BPS roundtrip y reversión al japonés desde revisiones inglesas anteriores. Pruebas públicas rechazan overflow, solapamientos, glifos inválidos y guards distintos. Las capturas de referencia y nativas muestran nombres de créditos, el mensaje secreto, las dos pantallas especiales y sus fondos legibles. Las rutas preparadas para créditos/victoria se identifican como diagnósticas; no se cuentan como campañas completadas.

Código y contrato recuperado: [bloque de créditos](https://github.com/MatyMeatBoy/DynaBrothers1-2/commit/ef13aa6) y [mensajes especiales](https://github.com/MatyMeatBoy/DynaBrothers1-2/commit/e2db7b2). Las imágenes de cartuchos y logs privados no se adjuntan.
