# DB2 Special · Traducción para Mega Drive y Genesis

El generador produce un BPS independiente desde el original de Sega Channel Special, CRC32 `BA3554BC`. Lleva a la consola las 548 entradas fijas, 100 cadenas de glifos de ROM, 205 diálogos de script, 24 campos de las biografías y los cuatro rótulos gráficos del menú principal. Verifica la revisión de origen, calcula el checksum de Mega Drive y comprueba que el BPS reproduce la ROM de salida byte por byte.

```text
python tools/translate_db2_console.py <original.md> --assembler <vasmm68k_mot>
```

La ROM de consola amplía el renderizador original en 68000 y cambia un banco de teselas del menú. Usa las fuentes y paletas del juego. Para los campos estrechos de las biografías, usa caracteres de cinco píxeles y conserva la asignación original de VRAM. El ejecutable del port mantiene sus capas de texto y genera su propia identidad de ROM. La opción de consola admite NTSC doméstico y NTSC de exportación al conservar el rechazo original de hardware de 50 Hz.

La prueba en Genesis Plus GX forzó NTSC de Japón y Estados Unidos. En los dos modos se recorrió el título, se entró a Normal 2, se pausó y se abrió la configuración de seis botones con MODE. Los encabezados centrados `1P` y `2P` también se verificaron en el ejecutable de Windows. En Estados Unidos se comprobó el menú Versus, el texto de la secuencia de presentación y la biografía de Stego. Para repetir el menú de seis botones, usa `games/db2special/scenes/player-normal2.input` seguido de Start, una espera y MODE.

Queda pendiente una prueba en Mega Drive y Genesis físicas, así como una revisión visual de todas las rutas. El BPS de entradas fijas se puede generar por separado con `tools/translate_db2.py --native-only`.
