# Visualização de curvas e superfícies

O objetivo desse projeto é visualizar curvas e superfícies em uma interface gráfica.
A principal motivação é a intuição de curvaturas intrínsecas, e por isso o foco desse trabalho é a visão geodésica, uma técnica de renderização análoga ao *Ray Tracing*.
A partir de um ponto, curvas geodésicas são traçadas, e os pontos atingidos são observados, formando uma imagem. Ao mover esse ponto, curvaturas intrínsecas serão percebidas pela forma com que a imagem se distorce. Na ausência de curvatura, a imagem se transforma rigidamente. Com curvatura, a imagem se distorce de alguma forma.

Além da visão geodésica, outras funcionalidades são implementadas, como a renderização 3D da superfície, traço de curvas, cálculo de comprimento e área.

A primeira etapa da aplicação é a definição dos objetos de interesse:
- Descrição textual dos objetos de interesse numa gramática especial
- Interpretação da descrição em uma estrutura de dados apropriada
- Geração de objetos não descritos porém necessários, como diversas derivadas e a primeira forma fundamental
- Compilação das fórmulas simbólicas em *shaders* na GPU, aproveitando as otimizações feitas pelo compilador e do fato de se ter um código de máquina específico para cada função

A segunda etapa é a visualização em si:
- Os objetos são discretizados para a renderização em um espaço 3D, curvas são segmentadas e superfícies são trianguladas
- Cálculo aproximado de comprimento e área
- As superfícies terão visão geodésica, texturas, e
- Desenhos de curvas e regiões vetoriais
- Controle de parâmetros globais e dos objetos