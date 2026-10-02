# Validación de OBS Biblia 0.4.0

Fecha: 2 de octubre de 2026.

Código compilado: `bf9786c609c9b180d313ec09593668e56d8c6637`.

[Compilación y pruebas en GitHub Actions](https://github.com/FULLY4425/Biblia-IASD-1.0/actions/runs/37076205782).

DLL Windows x64 compilada contra OBS 32.2.2 y Qt 6.11.1.

| Prueba | Resultado | Duración |
|---|---|---|
| references-and-lookup | Aprobada | 0.09 s |
| complete-chapter-fit | Aprobada | 1.40 s |
| slides-css-and-openlp | Aprobada | 2.62 s |
| real-obs-source-registration | Aprobada | 0.56 s |

Se verificaron prefijos únicos de libros, rechazo de abreviaturas ambiguas, rangos abiertos, búsquedas y navegación; ajuste de capítulos y tamaños pequeños; paginación Unicode que conserva caracteres, limpieza opcional de notas y saltos, sufijos después de z, estilos y conversión OpenLP. La prueba con el runtime real de OBS registra la fuente y crea el panel nativo con siete pestañas usando una configuración temporal aislada. Comprueba la persistencia del tamaño y escala, y el renderizado con franja activada/desactivada en posición inferior y centrada. También comprueba los cinco controles para borrar atajos.

La Biblia RVR1909 pasó la validación de 66 libros, 1189 capítulos y 31084 versículos.

## Alcance

La versión 0.4.0 no se instaló ni se probó manualmente en el OBS abierto del usuario. La prueba automatizada del panel utiliza los mismos archivos fuente y el runtime de OBS, pero no sustituye la comprobación de interacción, video, descargas de fuentes, listas, transiciones y apariencia dentro de la aplicación. No se reemplazó ninguna DLL en uso.

Se mantiene el control nativo en OBS. La personalización usa estilos Qt; no implementa todo CSS de navegador. Las sugerencias de Google Fonts son una lista pequeña incluida. El paquete es de instalación manual y contiene también el generador OpenLP.

## Integridad del paquete

- DLL: `443303ECECF8CC166B6AA678F4BBA34216E0BD3F4A81DF676DEFEF2F67C03FAF`
- Generador: `23890D1E4C83CB0EE1D8D84A2BE263975346E8D05F318522D4A7ACC5B9A45AFB`
- ZIP original de Actions: `890D2EBA70D14BED3E1AECEEA0E577506A4CA45A4F71336D525A7E8EBAC5ABF2`

El ZIP entregado añade este informe y la documentación actualizada; los binarios conservan sus bytes originales. El archivo SHA256 adjunto contiene su hash final.
