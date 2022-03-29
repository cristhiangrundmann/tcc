# Visualização de curvas e superfícies a partir de geodésicas

O objetivo desse projeto é desenvolver uma interface gráfica para visualizar curvas e superfícies. A principal ferramenta teórica que guia a implementação é a noção de medidas intrínsecas de curvatura (curvatura gaussiana), e por isso o foco desse trabalho é o *Geodesic Tracing*, uma técnica de renderização análoga ao *Ray Tracing* clássico da Computação Gráfica.
Nesta proposta, a partir de um ponto, curvas geodésicas são traçadas, e os pontos atingidos são observados, formando uma imagem. Ao mover esse ponto, curvaturas intrínsecas serão percebidas pela forma com que a imagem se distorce. Na ausência de curvatura, a imagem se transforma rigidamente. A presença de curvatura pode fazer geodésicas convergirem ou divergirem, distorcendo a imagem. Isso ocorre na astronomia, no fenômeno de lente gravitacional, onde a gravitação de uma galáxia faz raios de luz convergirem, distorcendo e até duplicando objetos na imagem.

Além de geodesic tracing, outras funcionalidades são implementadas, como a renderização 3D da superfície, traço de curvas, cálculo de comprimento e área.

A primeira etapa da aplicação é a definição dos objetos de interesse:
- Descrição textual dos objetos de interesse numa gramática especial
- Interpretação da descrição em uma estrutura de dados apropriada
- Geração de objetos não descritos porém necessários, como diversas derivadas e a primeira forma fundamental
- Compilação das fórmulas simbólicas, aproveitando as otimizações feitas pelo compilador e do fato de se ter um código de máquina específico para cada função

A segunda etapa é a visualização em si:
- Os objetos são discretizados para a renderização em um espaço 3D, curvas são segmentadas e superfícies são trianguladas
- Cálculo aproximado de comprimento e área
- As superfícies terão geodesic tracing, texturas, e
- Desenhos de curvas e regiões vetoriais
- Controle de parâmetros globais e dos objetos

A estética dos gráficos gerados e a perfórmance da interface são relevantes para o projeto, por isso o projeto será feito em *C++* e *OpenGL*.