# Graficador de automata celular

Es una implementacion de un automata celular. Un ejemplo de estos es el 'Juego de la Vida', pero existen de muchos otros.

Un automata celular esta dado por una cuadricula con celdas que estan prendidas o apagadas, en cada paso cada una de ellas nace, muere o se queda en su estado.

Se va leer la descripcion de un automata celular desde un archivo 'Run Length Encoded' o RLE que su primer linea tendra la siguiente pinta:

x = n, y = m, rule = abc

donde n y m son el ancho y alto de la grilla respectivamente. rule tendra el criterio del comportamiento de las celdas en cada paso rule = B12/S3 significa
que una celda nace si tiene 1 o 2 celdas vecinas vivas y sobrevive si tiene exactamente 3 vecinos.
La proxima linea indica el estado inicial de las celdas y sera una sucesion de <numero><tag>, siendo los posibles valores de <tag>:b,o o $.
b significa celda muerta, o celda viva y $ fin de linea. <numero> no es necesario si es 1. No es necesario especificar las celdas muertas hasta fin de linea.

La carpeta patrones contiene mas de mil automatas para probar el programa. Fueron descargados de [ConwayLife](http://www.conwaylife.com/).

Una mejora del programa seria poder graficar el automata sobre una grilla de tamaño mayor a la que viene definida.

Para correr el programa basta con:
* Compilar con: g++ automata-celular.cpp -o automata-celular
* Correrlo con: ./automata-celular ARCHIVORLE [TIEMPO]
