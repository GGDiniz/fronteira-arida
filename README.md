# Fronteira Árida

Protótipo 3D de sobrevivência em blocos, feito em C com Raylib. A câmera é em primeira pessoa: explore o Oeste, recolha materiais mirando nos blocos, trabalhe na serraria e construa seu próprio abrigo colocando blocos no mundo.

## Controles

- **WASD**: andar
- **Mouse**: olhar ao redor (o cursor fica capturado durante o jogo)
- **Espaço**: pular
- **Shift esquerdo**: correr
- **Botão esquerdo**: minerar o bloco no centro da mira
- **Botão direito**: colocar o bloco selecionado ao lado do bloco mirado
- **1–5 / roda do mouse**: selecionar material da barra rápida
- **E**: trabalhar perto da serraria e receber dinheiro
- **C**: transformar um tronco em quatro tábuas na serraria
- **F**: comprar comida
- **G**: comprar água
- **R**: descansar e recuperar vigor
- **Enter**: recomeçar depois de morrer

## Rodar no Windows

1. Instale o MinGW/WinLibs, caso ainda não tenha:

```bat
winget install --id BrechtSanders.WinLibs.MCF.UCRT -e
```

2. Baixe o pacote Raylib 6.0 para MinGW e extraia-o conforme o caminho esperado pelo `build.bat`, ou execute o próprio script para baixar e compilar:

```bat
build.bat
run.bat
```

Se o terminal bloquear os arquivos `.bat`, execute os comandos pelo CMD ou use o comando de compilação abaixo, ajustando os caminhos ao local das suas dependências:

```bat
gcc src\main.c -I deps\raylib-win64\raylib-6.0_win64_mingw-w64\include -L deps\raylib-win64\raylib-6.0_win64_mingw-w64\lib -o build\fronteira-arida.exe -lraylib -lopengl32 -lgdi32 -lwinmm -lm
copy deps\raylib-win64\raylib-6.0_win64_mingw-w64\lib\raylib.dll build\
```

## Sistemas do protótipo

- Mundo voxel 3D com terreno gerado, árvores, água, região arenosa e serraria
- Movimento em primeira pessoa, pulo, gravidade e colisão com blocos
- Mineração e colocação de blocos com alcance limitado
- Inventário rápido com terra, pedra, troncos, tábuas e areia
- Trabalho e fabricação de tábuas na serraria
- Compra de comida e água usando dinheiro
- Fome, sede, vigor e ciclo de dia
