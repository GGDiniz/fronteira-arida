# Fronteira Árida

Protótipo de jogo em C utilizando Raylib com foco em sobrevivência no Velho Oeste, inspirado no documento de referência do projeto.

## Objetivo

- Sobreviver ao calor, à sede e à fome
- Ganhar dinheiro trabalhando na serraria
- Coletar madeira e construir abrigo
- Evoluir da condição de forasteiro para moradia estável

## Como compilar

1. Baixe o pacote oficial do Raylib para Windows MinGW a partir do GitHub Releases.
2. Extraia em uma pasta local, por exemplo `C:/raylib`.
3. Ajuste o caminho abaixo para a pasta `include` e `lib` do Raylib.
4. Execute:

```bat
gcc src\main.c -I C:\raylib\include -L C:\raylib\lib -lraylib -lopengl32 -lgdi32 -lwinmm -o build\fronteira-arida.exe
copy C:\raylib\lib\raylib.dll build\
```

## Como executar

Basta abrir o arquivo `build\fronteira-arida.exe`.

## Mecânicas implementadas

- Sistema de fome, sede e energia
- Gerenciamento de dinheiro e madeira
- Construção de abrigo em fases
- Botões de ação para serraria, coleta, compra de mantimentos, descanso e construção
- Condição de game over por exaustão, fome ou sede
- Ciclo de tempo e clima simples
