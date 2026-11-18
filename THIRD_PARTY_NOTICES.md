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
| RetroAchievements / Tybis | Metadatos y badges públicos de DB1 | [Catálogo original](https://retroachievements.org/game/5362); URLs y SHA256 en `assets/achievements/db1-provenance.json`. No se atribuye al proyecto la autoría de estos recursos ni se concede licencia sobre ellos |
| Badges propios de DB2 | Arte vectorial original y variantes PNG | Licencia y fuentes visuales en `assets/achievements/db2/LICENSE.txt` y `provenance.json`; no se reutilizan píxeles de DB1 ni se incluyen gráficos extraídos del cartucho |
| stb_image | Carga PNG de insignias | Implementación fijada dentro de recomp-ui; licencia MIT/dominio público al final de su cabecera, incluida con el paquete |
| ReKHCom | Referencia de diseño para progreso offline, badges y avisos | [CC0, revisión 6c1951273fc2ccf79369cead1ba207e7297b931d](https://github.com/MatyMeatBoy/ReKHCom/tree/6c1951273fc2ccf79369cead1ba207e7297b931d). La implementación local de este proyecto no incorpora sus condiciones de logros ni sincronización |

Los paquetes copian avisos de runtime, fuentes, SDL, base de mandos y bibliotecas GCC/MinGW junto a los ejecutables. Los pins exactos se conservan en [upstream-pins.json](docs/research/upstream-pins.json) y en el manifiesto de cada paquete.
