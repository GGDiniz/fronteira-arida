# Fronteira Árida

Versão atual: sandbox de sobrevivência com gameplay inspirado em Minecraft, focado em exploração, coleta, trabalho e construção da vida do protagonista no Velho Oeste.

## Objetivo

- Explorar o mapa em busca de recursos
- Minerar terra, pedra, madeira e materiais de construção
- Trabalhar na serraria para ganhar dinheiro
- Construir abrigo e evoluir a base do personagem
- Sobreviver com fome, sede e energia

## Como rodar no Windows

1. Instale o MinGW/WinLibs (se ainda não tiver):

```bat
winget install --id BrechtSanders.WinLibs.MCF.UCRT -e
```

2. Baixe o Raylib oficial do GitHub:

```bat
mkdir deps
curl -L -o deps\raylib-6.0_win64_mingw-w64.zip https://github.com/raysan5/raylib/releases/download/6.0/raylib-6.0_win64_mingw-w64.zip
tar -xf deps\raylib-6.0_win64_mingw-w64.zip -C deps
```

3. Compile o projeto:

```bat
set PATH=C:\Users\SEU_USUARIO\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;%PATH%
mkdir build
gcc src\main.c -I deps\raylib-6.0_win64_mingw-w64\include -L deps\raylib-6.0_win64_mingw-w64\lib -o build\fronteira-arida.exe -lraylib -lopengl32 -lgdi32 -lwinmm
copy deps\raylib-6.0_win64_mingw-w64\lib\raylib.dll build\
```

4. Execute:

```bat
build\fronteira-arida.exe
```

## Controles

- WASD: movimentação
- Mouse esquerdo: minerar bloco
- Mouse direito: colocar bloco
- 1 a 5: trocar item no hotbar
- E: trabalhar na serraria
- F: comprar comida
- R: descansar
- B: construir abrigo
- Enter: reiniciar ao morrer

## Mecânicas implementadas

- Sistema de sobrevivência com fome, sede e energia
- Mundo em blocos com terreno, água, árvores e pedras
- Coleta de recursos diretamente pelo mundo
- Hotbar com materiais para construir
- Sistema de trabalho para ganhar dinheiro
- Construção de abrigo para evoluir a sobrevivência
- Ciclo de dia
- Visual em blocos com estética low-poly e estilo sandbox
