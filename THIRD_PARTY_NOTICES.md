# Procedencia y avisos

Este documento distingue las licencias de las dependencias; no concede derechos sobre los cartuchos originales.

| Componente | Uso | Licencia / fuente |
| --- | --- | --- |
| GenesisRecomp | Runtime y herramientas | [PolyForm Noncommercial](https://github.com/mstan/segagenesisrecomp/blob/4b2247d2b33a45623a686043a48c2021744c39ab/LICENSE.md) y [dependencias](https://github.com/mstan/segagenesisrecomp/blob/4b2247d2b33a45623a686043a48c2021744c39ab/THIRD-PARTY-LICENSES.md) |
| m68k-recomp-core | ABI de operaciones AOT | Avisos del submódulo fijado por el engine |
| recomp-ui | Launcher y ajustes | [MIT](https://github.com/mstan/recomp-ui/blob/d55bf8304c455dbc5773a000c919f6ba0a26d6bb/LICENSE) |
| SDL2 | Vídeo, audio e input | [zlib](https://github.com/libsdl-org/SDL/blob/SDL2/LICENSE.txt) |
| SDL_GameControllerDB | Identificación de dispositivos | [zlib](https://github.com/mdqinc/SDL_GameControllerDB/blob/4e87e3c6c2574f49e1cdf8526b6f829b61e5ccb3/LICENSE) |
| Fuentes de recomp-ui | Interfaz | LatoLatin/Noto: OFL; OpenMoji: CC BY-SA 4.0. Avisos copiados al paquete |
| Traducción de Tybis | Base inglesa de DB1 suministrada por el usuario | [Identidad publicada](https://retroachievements.org/forums/topic/15658); el parche de entrada no se incluye en el repositorio |
| Genesis Plus GX | Referencia local independiente | No se enlaza ni se distribuye en el checkpoint |

Los paquetes copian avisos de runtime, fuentes, SDL, base de mandos y bibliotecas GCC/MinGW junto a los ejecutables. Los pins exactos se conservan en [upstream-pins.json](docs/research/upstream-pins.json) y en el manifiesto de cada paquete.
