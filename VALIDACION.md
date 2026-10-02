# Validación de OBS Biblia 0.3.0

Fecha: 2 de octubre de 2026.

## Compilación final

Código compilado: `c4e5f099bc07b3f60f10e5841ca95057757e58be`.

Ejecución: https://github.com/FULLY4425/Biblia-IASD-1.0/actions/runs/37074016603

DLL Windows x64, OBS 32.2.2, Qt 6.11.1. SHA256 de la DLL: `4857ED5AB86589819711E57C5119A8DAF712A5E2DE80091DCB4C705D5A7AAC65`.

El ZIP de Actions tiene SHA256 `ad69fbbe06527fefbc84dbfb06dd364c4861428db3c3c0fbb2a0a0f7ede593e9`. El ZIP entregado se vuelve a empaquetar con este informe; la DLL y el generador conservan los bytes de la compilación final. Su hash aparece en el archivo SHA256 adjunto.

## Pruebas automatizadas aprobadas

| Prueba | Resultado | Duración |
|---|---|---|
| references-and-lookup | Aprobada | 0,09 s |
| complete-chapter-fit | Aprobada | 1,49 s |
| slides-css-and-openlp | Aprobada | 3,83 s |
| real-obs-source-registration | Aprobada | 0,07 s |

También pasó la validación de los 66 libros, 1189 capítulos y 31084 entradas de versículos de Reina-Valera 1909.

La prueba de extensiones comprueba límites de líneas, conservación del texto, caracteres hebreos, chinos y emoji, estilos de color y tamaño, respeto de la tipografía frente al estilo del programa anfitrión, conversión del esquema SQLite de OpenLP, consulta del JSON resultante, archivo original sin cambios y rechazo de licencia vacía, registros duplicados y archivo inexistente. La prueba de integración inicia el motor real de OBS y comprueba que registra el tipo de fuente Biblia.

## Comprobaciones manuales de esta ampliación

- El generador abrió como aplicación independiente con las bibliotecas incluidas, sin una instalación adicional de Qt.
- Se convirtió desde su interfaz un SQLite de prueba con las tablas OpenLP `book` y `verse`. Se guardó el JSON y se verificaron versión, licencia y los tres versículos sintéticos.
- La interfaz de OBS mostró las cinco pestañas: Biblia, Apariencia, Fondos, Temas y Listas.
- En una compilación intermedia, la fuente Biblia cargó y dibujó el fondo, la referencia y la franja en OBS. El tamaño del texto era incorrecto por la hoja de estilos global de OBS; se corrigió después y se añadió una regresión automatizada.
- La respuesta HTTPS de Google Fonts para Lora se comprobó desde una solicitud independiente: devuelve un TTF. Esto no verifica el botón de descarga dentro de OBS.

## Incidencias corregidas durante la validación

El SDK de OBS no incluye Qt SQL: el generador se cambió a SQLite incorporado en su ejecutable. La fuente compuesta necesitaba `audio_render` aunque el video se proyecte sin sonido; se añadió la función y la prueba de registro. El dibujado personalizado necesitaba activar el efecto gráfico de OBS; se corrigió tras observar el mensaje de falta de efecto. La tipografía de los versículos necesitaba reglas propias de tamaño para prevalecer sobre el estilo de OBS. Se incluyen los proveedores TLS de Qt en los datos del plugin para la descarga opcional de fuentes.

## Alcance pendiente

El usuario detuvo Computer Use con la tecla física Escape. Se dejó de controlar OBS. La DLL final se descargó y empaquetó después de las pruebas automáticas; no se reemplazó la DLL mientras OBS estaba abierto.

Quedan pendientes la comprobación visual del tamaño de letra y la paginación de la DLL final, la reproducción y repetición del video, la descarga de Google Fonts desde el panel, los temas personales, la persistencia y ordenación de listas, las transiciones, las tres políticas de altura y el recorrido completo de importar una Biblia generada y proyectarla. Las funciones están implementadas y compilan, pero no se presentan como verificadas manualmente.

La personalización admite hojas de estilo de Qt, no todo CSS de navegador. El generador admite Biblias SQLite locales de OpenLP con el esquema documentado; no convierte Biblias de consulta web. Solo se incluye Reina-Valera 1909; otras versiones deben importarse. El paquete es de instalación manual.

## Historial: validación de la versión 0.1

Los resultados siguientes pertenecen a la versión anterior, no sustituyen la comprobación de las funciones nuevas.

# Validación en Windows y OBS

Fecha: 2 de octubre de 2026.

## Ejecutado

`tests/verify_data.py` terminó con código de salida 0: 66 libros, 1.189 capítulos y 31.084 entradas de versículos. Se comprobaron numeraciones continuas, ausencia de textos vacíos y etiquetas HTML, y pasajes de Génesis, Salmos, Juan y Apocalipsis.

Los 66 archivos se recuperaron mediante el conector GitHub desde BibleAquifer/ReinaValera1909. Sus hashes de contenido quedan registrados en `data/bibles/provenance.txt`. Se retiraron únicamente los números en etiquetas `sup`, las etiquetas de párrafo y los espacios HTML `nbsp`; se conserva el texto del proveedor.

Se revisaron la declaración de la fuente, la creación del panel y las funciones para guardar/restaurar contra la documentación oficial de OBS. Se detectó OBS 32.2.2 en el equipo.

## Compilación verificada

GitHub Actions compiló la DLL Windows x64 con Visual Studio 2026 contra OBS 32.2.2 y Qt 6.11.1. Pasaron las pruebas C++ `references-and-lookup` y `complete-chapter-fit`, además de la validación completa de datos.

Código compilado: `7beabfd67cc9c9b63646c8f90883924c60d6b8d5`.
Ejecución: https://github.com/FULLY4425/Biblia-IASD-1.0/actions/runs/37066540373
SHA256 de la DLL: `7DFA66FC57F92B1452A994F428C0914A21F1CC79DCBA92BA83FF7D3F4F7E269B`.

## Pruebas realizadas en OBS 32.2.2

Se utilizó una copia portable con configuración separada. La instalación habitual del usuario no se modificó.

- La DLL cargó y registró la fuente **Biblia** y su panel.
- La fuente recibió automáticamente el pasaje del panel.
- Se proyectaron Génesis 2:1, Juan 3:16 y Juan 3:16–18, con acentos y referencia de la versión.
- Editar la referencia conservó la salida hasta pulsar **Proyectar**.
- Juan 99:1 mostró un error y conservó el pasaje anterior.
- **Ocultar pasaje** retiró texto y fondo; **Proyectar** los restauró.
- Se comprobaron los fondos **Bosque** y **Azul profundo**, una imagen PNG propia y el cambio a Georgia en vivo.
- OBS cerró normalmente. Al reiniciarlo con la DLL corregida recuperó Juan 3:16, Georgia y la imagen propia.
- Génesis 2 completo se proyectó sin recortarse con la DLL corregida.
- Salmos 119 completo se rechazó por exceder el espacio disponible y conservó el pasaje anterior.

## Fallo corregido

La primera DLL cortaba los capítulos completos porque medía el texto en el rectángulo de dibujo. Se cambió la medición para incluir el pasaje entero sin recorte antes de ajustar la letra. Se agregó una prueba de regresión para un versículo, un capítulo y un texto demasiado extenso.

La primera ejecución gráfica con Qt `offscreen` quedó detenida y se canceló. Con el motor Qt de Windows y un límite de 60 segundos, la prueba terminó correctamente en 1,43 segundos. La verificación visual posterior confirmó la corrección en OBS.

Los capítulos largos que caben necesitan letra pequeña. Para lectura desde lejos conviene proyectarlos por rangos.

## Alcance pendiente

No se han probado otras versiones de OBS, otras GPU, transmisión, grabación, dos fuentes simultáneas, cambio de colección ni importación de versiones externas. El registro de cierre de OBS informa una asignación sin liberar, sin atribución al plugin; no se ha realizado una auditoría de memoria. El paquete es de instalación manual, no un instalador automático.
