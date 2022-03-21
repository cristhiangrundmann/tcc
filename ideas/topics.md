# Visualização de superfícies

O objetivo desse projeto é visualizar superfícies em uma interface gráfica.
A principal motivação é a intuição de curvaturas intrínsecas, e por isso o foco desse trabalho é a visão geodésica, uma técnica de renderização análoga ao *Ray Tracing*.
A partir de um ponto, curvas geodésicas são traçadas, e os pontos atingidos são observados. Ao mover esse ponto, curvaturas intrínsecas serão percebidas pela forma com que a imagem se distorce. Na ausência de curvatura, a imagem se transforma rigidamente. Com curvatura, a imagem se distorce e pode até ter objetos duplicados.

Além da visão geodésica, outras funcionalidades são implementadas, como a renderização 3D da superfície, traço de curvas e cálculo de comprimento e área.