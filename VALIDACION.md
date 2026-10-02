# Validación de esta entrega

Fecha: 2 de octubre de 2026.

## Ejecutado

`tests/verify_data.py` terminó con código de salida 0: 66 libros, 1.189 capítulos y 31.084 entradas de versículos. Se comprobaron numeraciones continuas, ausencia de textos vacíos y etiquetas HTML, y pasajes de Génesis, Salmos, Juan y Apocalipsis.

Los 66 archivos se recuperaron mediante el conector GitHub desde BibleAquifer/ReinaValera1909. Sus hashes de contenido quedan registrados en `data/bibles/provenance.txt`. Se retiraron únicamente los números en etiquetas `sup`, las etiquetas de párrafo y los espacios HTML `nbsp`; se conserva el texto del proveedor.

Se revisaron la declaración de la fuente, la creación del panel y las funciones para guardar/restaurar contra la documentación oficial de OBS. Se detectó OBS 32.2.2 en el equipo.

## No ejecutado

No se ha compilado el C++ ni se han ejecutado `bible-tests` ni pruebas dentro de OBS. No se encontró CMake, compilador C++ ni los SDK de OBS/Qt en las ubicaciones consultadas. La revisión del código y la prueba de datos no demuestran que la DLL compile o funcione en OBS.

La siguiente etapa requiere preparar esos SDK, compilar, ejecutar `ctest` y realizar los pasos de verificación descritos en README. Esta entrega es el proyecto fuente inicial; el ZIP no es un instalador.
