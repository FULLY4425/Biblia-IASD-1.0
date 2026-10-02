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
