# Componentes y fuentes

El código propio se distribuye bajo GPL-2.0-or-later, incluida la opción GPLv3. La carpeta `generador` contiene bibliotecas Qt enlazadas dinámicamente; no deben copiarse sobre la instalación de OBS.

## Qt 6.11.1

Qt Core, Gui y Widgets: Copyright The Qt Company Ltd. y colaboradores. Se incluyen los textos LGPL-3.0 y GPL-3.0 en `licenses/`. Las bibliotecas permanecen separadas y pueden sustituirse por versiones compatibles; se permite la ingeniería inversa necesaria para depurar modificaciones de estas bibliotecas conforme a LGPL.

Código fuente de la versión: https://github.com/qt/qtbase/tree/v6.11.1

Paquete y recetas usados para compilar los binarios de OBS: https://github.com/obsproject/obs-deps

El plugin utiliza las bibliotecas Qt de OBS; el generador incluye copias del paquete Qt publicado por OBS. El flujo `scripts/build-windows.ps1` identifica el paquete y construye las aplicaciones. No se han modificado las bibliotecas Qt.

## SQLite 3.50.4

El generador y sus pruebas incorporan SQLite en el ejecutable, desde la amalgamación oficial de dominio público:

https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip

SHA256: `1d3049dd0f830a025a53105fc79fd2ab9431aea99e137809d064d8ee8356b032`.

Declaración de dominio público: https://www.sqlite.org/copyright.html

## Runtime Microsoft Visual C++

Se incluyen las bibliotecas redistribuibles necesarias para el generador, tomadas de la instalación de Visual Studio del compilador de GitHub Actions. Son componentes de Microsoft distribuidos bajo sus condiciones de redistribución.

## Texto bíblico y fuentes descargadas por el usuario

La atribución de Reina-Valera 1909 está en `data/bibles/provenance.txt`. Las tipografías opcionales de Google Fonts no se incluyen en el paquete. Las Biblias y fuentes que importe el usuario conservan sus propias licencias.
