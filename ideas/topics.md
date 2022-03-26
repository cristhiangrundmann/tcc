# Visualização de curvas e superfícies

O objetivo desse projeto é visualizar curvas e superfícies em uma interface gráfica.
A principal motivação é a intuição de curvaturas intrínsecas, e por isso o foco desse trabalho é a visão geodésica, uma técnica de renderização análoga ao *Ray Tracing*.
A partir de um ponto, curvas geodésicas são traçadas, e os pontos atingidos são observados, formando uma imagem. Ao mover esse ponto, curvaturas intrínsecas serão percebidas pela forma com que a imagem se distorce. Na ausência de curvatura, a imagem se transforma rigidamente. Com curvatura, a imagem se distorce de alguma forma.

Além da visão geodésica, outras funcionalidades são implementadas, como a renderização 3D da superfície, traço de curvas, cálculo de comprimento e área.

O projeto consiste nos seguintes componentes:
- Gramática de descrição de curvas e superfícies
- Geração de matemática simbólica a partir da descrição dada:
    - Função de parametrização
    - Aplicação normal
    - Primeira forma fundamental
    - Equação geodésica
    - Aproximação de geodésicas por Runge-Kutta 4 e **visão geodésica**
- Compilação da matemática simbólica gerada em *shaders*
- Controle de parâmetros globais e de superfície
- Desenho de curvas e regiões com gráficos vetoriais (plano UV)
- Cálculo aproximado de comprimentos e de áreas
- Visualizações:
    - Triangulação da superfície para desenho 3D
    - Controle da renderização 3D
    - Controle da visão geodésica
- Técnicas de perfórmance