# Biblia para OBS — versión inicial 0.1.0

Plugin nativo en C++ y Qt 6. Registra una fuente llamada **Biblia** y un panel acoplable con pestañas **Biblia** y **Apariencia**. No utiliza navegador, HTML ni servicios de consulta en línea.

## Funciones implementadas en el código

- Selección de versión y búsqueda por libro, capítulo, versículo o rango: `Juan 3:16`, `Juan 3:16-18`, `Salmos 23`.
- Búsqueda sin distinguir mayúsculas ni acentos, con nombres completos de libros.
- Vista del texto encontrado, botón **Proyectar** y botón **Ocultar pasaje**.
- Tipografía del sistema, tamaño, color, negrita y cursiva.
- Cuatro fondos generados dentro del plugin: Azul profundo, Amanecer, Bosque y Púrpura; selección de imágenes propias y oscurecimiento.
- Fuente de 1920 × 1080 con ajuste automático de letra. Rechaza pasajes demasiado extensos para evitar cortar el texto.
- Todas las instancias de la fuente Biblia reciben el mismo pasaje. No hace falta vincularlas manualmente.
- Conserva el pasaje y la apariencia por colección de escenas de OBS.
- Importación de versiones locales en JSON. Valida el formato antes de guardar y mantiene las versiones en la configuración de OBS.

## Estado real

Este proyecto contiene código fuente, datos y pruebas. **Todavía no es un instalador ni una DLL compilada y no se ha probado cargándolo en OBS.** En este equipo se detectó OBS 32.2.2, pero no se encontró un entorno de compilación C++ con CMake y los paquetes de desarrollo de OBS/Qt.

Incluye los 66 libros de Reina-Valera 1909: 1.189 capítulos y 31.084 entradas de versículos según los archivos del proveedor BibleAquifer. Se conserva su numeración y texto, sin modernizar la ortografía. La procedencia y los hashes de los 66 archivos originales están en `data/bibles/provenance.txt`. También puedes importar otras versiones en el formato siguiente.

## Compilar en Windows x64

Se necesitan Visual Studio 2022 con desarrollo C++, CMake 3.28 o posterior, Qt 6 (MSVC x64) y los paquetes de desarrollo `libobs` y `obs-frontend-api` compatibles con la instalación de OBS. El OBS instalado normalmente no incluye estos paquetes. Los binarios Qt deben coincidir con los que usa OBS; no copiar otra versión de Qt encima de OBS.

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

Abre OBS, habilita el panel **Biblia** en el menú de paneles y añade **Biblia** desde el botón `+` de Fuentes. Busca un pasaje y pulsa **Proyectar**. Cambiar la referencia o buscar otro texto no cambia la salida hasta pulsar Proyectar; los cambios de apariencia sí actualizan el pasaje que está al aire.

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

Las imágenes propias se guardan como rutas: si las mueves, vuelve a seleccionarlas. Las versiones importadas permanecen en la carpeta de configuración de OBS. La ocultación deja la fuente transparente, incluido el fondo.

## Verificación pendiente en OBS

1. Compilar y ejecutar las pruebas C++.
2. Cargar en OBS 32.2.2 y confirmar que aparecen el panel y la fuente.
3. Proyectar un versículo, un rango y un capítulo; comprobar acentos y libros numerados.
4. Agregar dos fuentes Biblia; confirmar que las dos reciben el mismo pasaje.
5. Cambiar apariencia, probar una imagen propia y ocultar/restaurar un pasaje.
6. Guardar, reiniciar OBS y cambiar de colección de escenas; confirmar la recuperación del estado.
7. Importar versiones válidas e inválidas, comprobar mensajes y cerrar OBS sin errores.

## Atribuciones

El texto Reina-Valera 1909 es de dominio público. [BibleAquifer](https://github.com/BibleAquifer/ReinaValera1909) publica esa edición con declaración de dominio público/CC0. Los fondos son diseños generados mediante código incluido en este proyecto. El código del plugin se distribuye bajo GPL-2.0-or-later, compatible con OBS; ver LICENSE.
