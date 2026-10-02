# Biblia para OBS — versión 0.4.0

Plugin nativo en C++ y Qt 6. Registra una fuente llamada **Biblia** y un panel acoplable con pestañas **Biblia**, **Apariencia**, **Fondos**, **Temas** y **Listas**. No utiliza navegador, HTML ni servicios de consulta en línea.

## Novedades 0.4.0

- Franja de color opcional, independiente del fondo y de la posición inferior/centrada. Tamaño de letra de 12 a 240 píxeles y escala de 0,5 a 2.
- Consulta separada, sugerencias locales de libros, Biblia, Listas, Temas y Configuración; Apariencia y Fondos mantienen sus pestañas propias.
- Listas por arrastre, clic para preparar y botón para proyectar; vaciado con confirmación y añadido del capítulo completo.
- Temas al seleccionarlos, copias con Guardar como tema, edición, renombrado y borrado de temas personales; Ctrl+S guarda el tema personal en su editor.
- Atajos editables que funcionan cuando el panel tiene foco; limpiar el campo desactiva el atajo. Restauración de valores y ayudas opcionales.
- Cero líneas desactiva la división. Letras a/b/c opcionales, limpieza de notas numéricas y saltos internos, y restablecimiento sin borrar Biblias.

Las sugerencias de Google Fonts son una lista pequeña incluida sin conexión. Rachni/Acri son variantes nativas de color inspiradas en la guía. La implementación utiliza estilos Qt, no CSS de navegador. No incorpora licencias comerciales, cuentas ni destinos web. Quedan pendientes los iconos personalizados y un indicador independiente de versículo proyectado; la selección actual identifica el pasaje preparado.

## Funciones implementadas en el código

- Selección de versión y búsqueda por libro, capítulo, versículo o rango: `Juan 3:16`, `Juan 3:16-18`, `Salmos 23`.
- Búsqueda sin distinguir mayúsculas ni acentos, con nombres completos o prefijos únicos de libros y rangos abiertos (`Heb 11:39-`).
- Lista del capítulo con el versículo seleccionado resaltado. Selección de rangos contiguos con Ctrl/Shift, doble clic para proyectar, botones de navegación de versículo y capítulo.
- Botón **Proyectar**, botón **Ocultar pasaje** y modo opcional **Proyectar al seleccionar / navegar**.
- Franja inferior inspirada en la referencia visual del usuario: referencia a la izquierda, versión a la derecha y texto debajo, con color y opacidad configurables. También conserva la proyección centrada.
- Tipografía del sistema, tamaño, color, negrita y cursiva.
- Cuatro fondos incluidos: Azul profundo, Amanecer, Bosque y Púrpura; imágenes propias, videos locales y fondo transparente.
- Videos en bucle, sin sonido, utilizando la fuente multimedia nativa de OBS. El video cubre el área de proyección conservando su proporción y recortando los bordes que sobresalgan.
- Fuente de 1920 × 1080. Divide automáticamente pasajes largos en diapositivas con un máximo configurable de líneas; botones anterior/siguiente y contador. La división conserva el texto y usa el motor Unicode de Qt.
- Altura de recuadro ajustada a cada diapositiva, a la más larga o al espacio disponible.
- Cuatro temas, temas personales guardados y editor de hojas de estilo nativas de Qt.
- Transición opcional de disolución de 250 ms.
- Listas locales con referencias y versiones; añadir, proyectar, quitar y ordenar entradas.
- Importación local de fuentes TTF/OTF y descarga opcional de Google Fonts con caché sin conexión.
- Aplicación separada **biblia-generador.exe** para convertir Biblias SQLite sin conexión de OpenLP a JSON.
- Todas las instancias de la fuente Biblia reciben el mismo pasaje. No hace falta vincularlas manualmente.
- Conserva el pasaje y la apariencia por colección de escenas de OBS.
- Importación de versiones locales en JSON. Valida el formato antes de guardar y mantiene las versiones en la configuración de OBS.

## Estado real

La DLL Windows x64 se compila con GitHub Actions contra OBS 32.2.2 y Qt 6.11.1. Consulta `VALIDACION.md` para distinguir pruebas automatizadas y pruebas manuales de cada versión. El paquete contiene plugin, datos y generador; la instalación es manual.

Incluye los 66 libros de Reina-Valera 1909: 1.189 capítulos y 31.084 entradas de versículos según los archivos del proveedor BibleAquifer. Se conserva su numeración y texto, sin modernizar la ortografía. La procedencia y los hashes de los 66 archivos originales están en `data/bibles/provenance.txt`. También puedes importar otras versiones en el formato siguiente.

## Compilar en Windows x64

Se necesitan Visual Studio 2022 o 2026 con desarrollo C++, CMake 3.28 o posterior, Qt 6 (MSVC x64) y los paquetes de desarrollo `libobs` y `obs-frontend-api` compatibles con la instalación de OBS. El flujo `.github/workflows/windows.yml` prepara el SDK y ejecuta las pruebas. El OBS instalado normalmente no incluye estos paquetes. Los binarios Qt deben coincidir con los que usa OBS; no copiar otra versión de Qt encima de OBS.

Configurar `CMAKE_PREFIX_PATH` con las rutas de los paquetes de desarrollo, por ejemplo:

```powershell
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/SDK/OBS;C:/SDK/Qt/lib/cmake"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix dist
```

Las rutas del ejemplo son marcadores que deben sustituirse. Se usan las interfaces oficiales documentadas por [OBS](https://docs.obsproject.com/plugins) y la estructura de enlace de su [plantilla de plugins](https://github.com/obsproject/obs-plugintemplate).

## Instalación manual, después de compilar

Cierra OBS. El árbol `dist` contiene `obs-plugins/64bit/obs-biblia.dll` y `data/obs-plugins/obs-biblia/`. Copia ambas carpetas conservando esa estructura a la carpeta de instalación de OBS, con los permisos que Windows solicite. No copiar los archivos `.cpp` a OBS.

Abre OBS, habilita el panel **Biblia** en el menú **Paneles** y añade **Biblia** desde el botón `+` de Fuentes. Arrastra el título del panel al lado derecho de OBS para acoplarlo como en la referencia visual. Busca un pasaje y pulsa **Proyectar**. El capítulo aparece en la lista; puedes preparar un versículo con un clic y proyectarlo con el botón o con doble clic. Activa **Proyectar al seleccionar / navegar** si quieres cambiar la salida al seleccionar o usar las flechas. Por defecto, buscar y navegar solo prepara la selección.

En **Apariencia** puedes escoger **Franja inferior** o **Texto centrado**, tipografía, color y opacidad de franja. Los cambios de apariencia actualizan lo que está al aire. Activa **Dividir automáticamente en diapositivas**, configura el máximo de líneas y utiliza los botones de diapositiva en Biblia. El ajuste de altura afecta al recuadro detrás del texto; las imágenes y los videos conservan su cobertura de pantalla. Si desactivas la división, el tamaño de letra se reduce para intentar encajar el pasaje.

En **Fondos**, pulsa **Escoger video…** para cargar MP4, MOV, MKV, WEBM, AVI o M4V desde tu equipo. OBS lo reproduce automáticamente en bucle y sin sonido. No se sube el video a Internet. La miniatura del panel indica el archivo seleccionado; el movimiento se ve en la fuente y en la vista previa de OBS. La compatibilidad de los códecs depende del módulo multimedia de OBS; los errores de reproducción se muestran en el panel.

El fondo **Transparente** permite poner la franja sobre otras fuentes de la escena. **Ocultar pasaje** retira tanto el texto como el fondo de esta fuente.

## Temas, fuentes y listas

En **Temas** selecciona un tema y pulsa **Aplicar tema**, o guarda la apariencia actual con un nombre. El editor usa [hojas de estilo de Qt](https://doc.qt.io/qt-6/stylesheet-reference.html), con sintaxis compatible con CSS para sus widgets: no implementa todo CSS de navegador, HTML, JavaScript ni animaciones CSS. Usa **Apariencia** para controlar el diseño, la altura y las transiciones. Ejemplo:

```css
QLabel#verse { color: #ffffff; font-family: Georgia; }
QLabel#reference { color: #ffd67a; }
QLabel#version { color: #d7d2e8; }
QWidget#frame { border: 2px solid #9575cd; border-radius: 14px; }
```

El texto es plano. Tamaños extremos, bordes y rellenos personalizados pueden reducir el espacio útil: comprueba la salida al aplicar estilos. La paginación contempla el tamaño y la familia tipográfica resueltos por Qt. Los temas y listas se almacenan con escritura atómica en `library/library.json` dentro de la configuración del módulo; las fuentes importadas o descargadas se guardan en `fonts/` y se cargan sin Internet al iniciar.

**Google Fonts** solo conecta al pulsar su botón de descarga; escribe el nombre de una familia (por ejemplo `Lora`). Solicita una fuente regular desde la [API de Google Fonts](https://developers.google.com/fonts/docs/getting_started) y la guarda localmente. Si el servicio devuelve un formato incompatible con Qt, el panel muestra el error y permite importar un TTF/OTF. La descarga necesita Internet; consultar Biblias, proyectar, usar temas, listas y fuentes ya guardadas no lo necesita.

En **Listas**, crea una lista, prepara un pasaje en Biblia y pulsa **Añadir el pasaje preparado**. Puedes ordenar o quitar entradas y proyectarlas con doble clic. Cada entrada conserva versión y referencia; si falta esa Biblia, el panel solicita importarla. Límites: 100 listas y 5000 entradas por lista, 100 temas personales.

## Generador separado para OpenLP

Abre `generador/biblia-generador.exe` junto a las DLL y subcarpetas incluidas. Selecciona una Biblia SQLite local de OpenLP (`.sqlite`, `.sqlite3` o `.db`), completa identificador, nombre y licencia del texto, y genera un JSON. Después, en OBS pulsa **Importar versión JSON…**. El generador abre SQLite en modo de solo lectura; no modifica la Biblia original ni utiliza Internet. Admite el esquema [oficial de OpenLP](https://gitlab.com/openlp/openlp/-/blob/master/openlp/plugins/bibles/lib/db.py), con tablas `book` y `verse`. No convierte Biblias que dependen de consultas web ni otros formatos de archivos de OpenLP.

## Formato de versiones

```json
{
  "id": "mi-version",
  "name": "Nombre de la versión",
  "license": "Licencia o autorización de distribución del texto",
  "books": {
    "Juan": {
      "3": {
        "16": "Texto del versículo",
        "17": "Texto del siguiente versículo"
      }
    }
  }
}
```

Capítulos y versículos son claves numéricas positivas en forma de cadenas. El archivo debe ser UTF-8, con un máximo de 32 MB. Se requieren nombres completos de libros; los alias como `Jn` no están implementados. Una versión parcial no permite proyectar capítulos incompletos.

Las imágenes y los videos se guardan como rutas: si los mueves, vuelve a seleccionarlos. Las versiones importadas permanecen en la carpeta de configuración de OBS. La ocultación deja la fuente transparente, incluido el fondo.

## Comprobaciones para otras instalaciones

1. Compilar y ejecutar las pruebas C++.
2. Cargar en OBS 32.2.2 y confirmar que aparecen el panel y la fuente.
3. Proyectar un versículo, un rango y un capítulo; comprobar acentos y libros numerados.
4. Agregar dos fuentes Biblia; confirmar que las dos reciben el mismo pasaje.
5. Cambiar apariencia, probar una imagen propia y ocultar/restaurar un pasaje.
6. Guardar, reiniciar OBS y cambiar de colección de escenas; confirmar la recuperación del estado.
7. Importar versiones válidas e inválidas, comprobar mensajes y cerrar OBS sin errores.

## Atribuciones

El texto Reina-Valera 1909 es de dominio público. [BibleAquifer](https://github.com/BibleAquifer/ReinaValera1909) publica esa edición con declaración de dominio público/CC0. Los fondos son diseños generados mediante código incluido en este proyecto. El código del plugin se distribuye bajo GPL-2.0-or-later, compatible con OBS; ver LICENSE.
