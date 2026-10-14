# Fronteira Árida

Protótipo de jogo em C utilizando Raylib com foco em sobrevivência no Velho Oeste, inspirado no documento de referência do projeto.

## Objetivo

- Sobreviver ao calor, à sede e à fome
- Ganhar dinheiro trabalhando na serraria
- Coletar madeira e construir abrigo
- Evoluir da condição de forasteiro para moradia estável

## Como rodar no Windows

1. Instale o MinGW/WinLibs (se ainda nao tiver):

```bat
winget install --id BrechtSanders.WinLibs.MCF.UCRT -e
```

2. Abra um novo terminal e baixe o Raylib oficial do GitHub:

```bat
mkdir deps
curl -L -o deps\raylib-6.0_win64_mingw-w64.zip https://github.com/raysan5/raylib/releases/download/6.0/raylib-6.0_win64_mingw-w64.zip
tar -xf deps\raylib-6.0_win64_mingw-w64.zip -C deps
```

3. Compile o jogo manualmente:

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

Se o Windows bloquear o `.bat`, use os comandos acima diretamente no CMD.

## Mecânicas implementadas

- Sistema de fome, sede e energia
- Gerenciamento de dinheiro e madeira
- Construção de abrigo em fases
- Botões de ação para serraria, coleta, compra de mantimentos, descanso e construção
- Condição de game over por exaustão, fome ou sede
- Ciclo de tempo e clima simples
- Visual low-poly com céu, terreno, árvores, abrigo e personagem em cena
