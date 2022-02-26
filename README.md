# Trabalho de conclusão de curso
O tema geral é **Aplicação em Visualização de Curvas e Superfícies**.
O usuário controla uma interface gráfica, definindo e visualizando curvas e superfícies.

## Teoria
A superfície será visualizada em 3D usando a sua parametrização, mas o restante (como cálculos de área, comprimento, e geodésicas) será feito a partir da primeira forma fundamental, pois as noções intrínsicas serão mais exploradas.

### Superfícies
- As superfícies são parametrizadas com $s: [A,B] \times [C,D] \rightarrow \mathbb{R}^3$.
O domínio da parametrização é chamado de **plano UV** (apesar de ser um retângulo).
- A parametrização da superfície é dada por uma expressão fechada, usando um conjunto limitado de funções e operações, como as funções trigonométricas, e constantes, como $e$ e $\pi$.
- As funções, assim como funções auxiliares (derivadas, primeira forma fundamental), são tratadas simbolicamente.
- A visualização da superfície em 3D é feita por uma triangulação do plano UV, podendo não ser uniforme (por exemplo, mais densa em regiões de alta curvatura).

### Curvas
- As curvas vão sempre ser sobre uma superfície.
- As curvas mais importantes vão ser as geodésicas.
- As geodésicas serão tratadas numericamente, usando métodos melhores que o de Euler.
- Um ponto sobre a superfície (mas definido no plano UV) poderá ser movido ao longo de geodésicas, e sua orientação poderá ser girada de forma locamente uniforme.
- Curvas no plano UV poderão ser definidas parametricamente.

### Medidas
- Sobre uma superfície, deverá ser possível medir áreas e comprimentos.

### Visão Geodésica
- Chama-se de **câmera polar** um ponto e um vetor de direção sobre o plano UV, onde o vetor tem comprimento 1, medido no ponto.
- A partir de uma câmera polar, define-se uma aplicação do **plano polar** para o plano UV: dado $(\theta, r)$, gira-se a orientação do ponto em um ângulo $\theta$ e mova-se geodesicamente para frente, numa distância de $r$ unidades.
- Dada uma câmera polar e uma textura sobre o plano UV, podemos vê-la de forma intrínseca no plano polar.
- Ao se mover, podemos observar as curvaturas intrínsecas da superfície, pois a textura não se comporta rigidamente nessas regiões.
- O cone é uma superfície de interesse, pois a geometria é praticamente Euclidiana.

## Prática
A implementação da aplicação será feita em C/C++, utilizando diversas ferramentas & APIs multi-plataforma e open-source.

### Ferramentas e APIs
- OpenGL (core-profile): renderização eficiente em 3D e 2D, utilizando a GPU. Inclusive, as funções simbólicas serão compiladas e executas na GPU, possivelmente em paralelo, maximizando a performance da visão geodésica, inclusive.
- GLFW3 + GLAD + GLM: criação de iterface gráficas(GLFW), inicialização do OpenGL e suas extensões(GLAD), matemática vetorial(GLM) e mecanismos de entrada e saída. Facilita bastante o uso do OpenGL.
- Pango + Cairo: gráficos vetoriais (cairo) e textos (pango), em 2D. Perfeitos para a criação de boas texturas com textos, curvas suaves, etc.

### Implementação
- Controles: elementos como a câmera polar e velocidades serão manipuladas por mouse e teclado.
- Texturas: será possível desenhar sobre o plano UV e sobre o plano polar.
- A definição da superfície e seu plano UV será dada por uma expressão textual, seguindo uma gramática formal.
- O cálculo de objetos auxiliares, como derivadas, primeira forma fundamental, e equação geodésica será feito simbolicamente. Algumas otimizações serão feitas nas expressões.
- As funções simbólicas geradas serão compiladas e rodadas na GPU, nas partes programáveis da pipe-line do OpenGL, os chamados  **shaders**. Várias otimizações matemáticas serão realizadas pelo compilador.
- A derivação simbólica e a geração de expressões auxiliares será de implementação própria.
- A gramática das expressões será mais flexível que as encontradas em linguagens de programação.
- A equação geodésica, assim como problemas de integrais, serão resolvidos numericamente, utilizando algum método numérico melhor que o de Euler, possivelmente o Runge-Kutta 4.

## Possibilidades
Funcionalidades mais difíceis de tratar, mas interessantes.

- Plano UV com regiões mais gerais, como circular.
- Renderização das expressões em notação matemática.
- Otimização/simplificação em matemática simbólica (por exemplo, $cos^2a+sin^2a=1$).
- As expressões definidas simbolicamente poderão ter parâmetros controláveis na interface. Alterar esses parâmetros não será muito rápido, uma vez que a triangulação no plano UV mudará de imagem no espaço 3D. A não uniformidade da triangulação é outro problema.

# Possíveis Referências

- Livro-site tutorial de OpenGL: <https://learnopengl.com/>
- Especificação OpenGL 4.6 (core-profile): <https://www.khronos.org/registry/OpenGL/specs/gl/glspec46.core.pdf>
- Gramáticas livre de contexto, compiladores, etc.
- Livros-texto de curvas e superfícies.