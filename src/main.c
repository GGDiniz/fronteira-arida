#include "raylib.h"
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 650

typedef struct {
    char nome[50];
    float dinheiro;
    int fome;
    int sede;
    int energia;
    int madeira;
    int nivelAbrigo;
} Jogador;

static void resetJogo(Jogador *j) {
    strcpy(j->nome, "Arthur (Forasteiro)");
    j->dinheiro = 2.00f;
    j->fome = 100;
    j->sede = 100;
    j->energia = 100;
    j->madeira = 0;
    j->nivelAbrigo = 0;
}

static void trabalharSerraria(Jogador *j) {
    if (j->energia >= 25) {
        j->dinheiro += 3.50f;
        j->energia -= 30;
        j->fome -= 20;
        j->sede -= 25;
        if (j->fome < 0) j->fome = 0;
        if (j->sede < 0) j->sede = 0;
    }
}

static void cortarLenha(Jogador *j) {
    if (j->energia >= 20) {
        j->madeira += 2;
        j->energia -= 20;
        j->sede -= 20;
        if (j->sede < 0) j->sede = 0;
    }
}

static void comprarComida(Jogador *j) {
    if (j->dinheiro >= 1.50f) {
        j->dinheiro -= 1.50f;
        j->fome = (j->fome + 40 > 100) ? 100 : j->fome + 40;
    }
}

static void comprarAgua(Jogador *j) {
    if (j->dinheiro >= 1.00f) {
        j->dinheiro -= 1.00f;
        j->sede = (j->sede + 50 > 100) ? 100 : j->sede + 50;
    }
}

static void construirAbrigo(Jogador *j) {
    if (j->nivelAbrigo == 0 && j->madeira >= 4) {
        j->madeira -= 4;
        j->nivelAbrigo = 1;
    } else if (j->nivelAbrigo == 1 && j->madeira >= 10) {
        j->madeira -= 10;
        j->nivelAbrigo = 2;
    }
}

static void descansar(Jogador *j) {
    if (j->nivelAbrigo == 0) {
        j->energia = 100;
        j->fome -= 15;
        j->sede -= 15;
    } else {
        j->energia = 100;
        j->fome -= (j->nivelAbrigo == 2) ? 5 : 10;
        j->sede -= (j->nivelAbrigo == 2) ? 5 : 10;
    }

    if (j->fome < 0) j->fome = 0;
    if (j->sede < 0) j->sede = 0;
}

static void consumirTurno(Jogador *j, int dia, float clima) {
    int desgaste = 4;
    if (dia % 2 == 0) {
        desgaste += 1;
    }

    if (clima > 0.75f) {
        desgaste += 2;
    }

    j->fome = (j->fome - (4 + desgaste / 2) < 0) ? 0 : j->fome - (4 + desgaste / 2);
    j->sede = (j->sede - (5 + desgaste) < 0) ? 0 : j->sede - (5 + desgaste);
    j->energia = (j->energia - (2 + desgaste / 3) < 0) ? 0 : j->energia - (2 + desgaste / 3);
}

static const char *descreverAbrigo(int nivel) {
    if (nivel == 0) return "Nenhum (Ao relento)";
    if (nivel == 1) return "Acampamento de Lona";
    return "Cabana de Toras Rustica";
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Fronteira Arida - Velho Oeste (Prototipo Raylib)");
    SetTargetFPS(60);

    Jogador jogador;
    resetJogo(&jogador);

    int dia = 1;
    float clima = 0.35f;
    float acumuladorTempo = 0.0f;
    bool gameOver = false;

    Color corFundo = {245, 235, 215, 255};
    Color corPainel = {210, 180, 140, 255};
    Color corTextoEscuro = {60, 42, 20, 255};
    Color corBotao = {139, 69, 19, 255};

    Rectangle btnSerraria = {50, 420, 360, 45};
    Rectangle btnLenha = {50, 475, 360, 45};
    Rectangle btnComida = {490, 420, 360, 45};
    Rectangle btnAgua = {490, 475, 360, 45};
    Rectangle btnAbrigo = {50, 530, 360, 45};
    Rectangle btnDescanso = {490, 530, 360, 45};

    while (!WindowShouldClose()) {
        Vector2 posicaoMouse = GetMousePosition();
        bool clicou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (!gameOver) {
            acumuladorTempo += GetFrameTime();
            if (acumuladorTempo >= 1.80f) {
                clima += 0.05f;
                if (clima > 1.0f) clima = 0.0f;
                consumirTurno(&jogador, dia, clima);
                dia += 1;
                acumuladorTempo = 0.0f;
            }

            if (clicou) {
                if (CheckCollisionPointRec(posicaoMouse, btnSerraria)) trabalharSerraria(&jogador);
                if (CheckCollisionPointRec(posicaoMouse, btnLenha)) cortarLenha(&jogador);
                if (CheckCollisionPointRec(posicaoMouse, btnComida)) comprarComida(&jogador);
                if (CheckCollisionPointRec(posicaoMouse, btnAgua)) comprarAgua(&jogador);
                if (CheckCollisionPointRec(posicaoMouse, btnAbrigo)) construirAbrigo(&jogador);
                if (CheckCollisionPointRec(posicaoMouse, btnDescanso)) descansar(&jogador);
            }
        }

        gameOver = (jogador.fome <= 0 || jogador.sede <= 0 || jogador.energia <= 0);

        BeginDrawing();
        ClearBackground(corFundo);

        if (!gameOver) {
            DrawText("FRONTEIRA ARIDA - SIMULADOR DE SOBREVIVENCIA", 50, 30, 22, corTextoEscuro);
            DrawText("Versao Grafica em C (Raylib)", 50, 55, 14, DARKGRAY);

            DrawRectangle(50, 90, 800, 160, corPainel);
            DrawRectangleLines(50, 90, 800, 160, corTextoEscuro);

            char buffer[128];
            sprintf(buffer, "Forasteiro: %s | Dia: %d | Clima: %.0f%%", jogador.nome, dia, clima * 100.0f);
            DrawText(buffer, 70, 110, 18, corTextoEscuro);

            sprintf(buffer, "Dinheiro: $%.2f   |   Toras de Madeira: %d", jogador.dinheiro, jogador.madeira);
            DrawText(buffer, 70, 140, 16, corTextoEscuro);

            sprintf(buffer, "Abrigo: %s", descreverAbrigo(jogador.nivelAbrigo));
            DrawText(buffer, 70, 165, 16, corTextoEscuro);

            DrawText("Fome:", 70, 200, 14, corTextoEscuro);
            DrawRectangle(130, 200, 200, 16, LIGHTGRAY);
            DrawRectangle(130, 200, (int)(2.0f * jogador.fome), 16, RED);

            DrawText("Sede:", 350, 200, 14, corTextoEscuro);
            DrawRectangle(400, 200, 200, 16, LIGHTGRAY);
            DrawRectangle(400, 200, (int)(2.0f * jogador.sede), 16, BLUE);

            DrawText("Energia:", 620, 200, 14, corTextoEscuro);
            DrawRectangle(690, 200, 140, 16, LIGHTGRAY);
            DrawRectangle(690, 200, (int)(1.4f * jogador.energia), 16, GREEN);

            DrawText("PAINEL DE ACOES (CLIQUE NOS BOTOES)", 50, 380, 16, corTextoEscuro);

            DrawRectangleRec(btnSerraria, corBotao);
            DrawText("1. Trabalhar na Serraria (Ganha $3.50)", btnSerraria.x + 15, btnSerraria.y + 14, 14, WHITE);

            DrawRectangleRec(btnLenha, corBotao);
            DrawText("2. Cortar Lenha na Floresta (+2 Toras)", btnLenha.x + 15, btnLenha.y + 14, 14, WHITE);

            DrawRectangleRec(btnComida, corBotao);
            DrawText("3. Comprar Comida no Saloon (-$1.50)", btnComida.x + 15, btnComida.y + 14, 14, WHITE);

            DrawRectangleRec(btnAgua, corBotao);
            DrawText("4. Comprar Agua Fresca (-$1.00)", btnAgua.x + 15, btnAgua.y + 14, 14, WHITE);

            DrawRectangleRec(btnAbrigo, corBotao);
            DrawText("5. Construir / Melhorar Abrigo (Usa Toras)", btnAbrigo.x + 15, btnAbrigo.y + 14, 14, WHITE);

            DrawRectangleRec(btnDescanso, corBotao);
            DrawText("6. Descansar no Abrigo (Recupera Energia)", btnDescanso.x + 15, btnDescanso.y + 14, 14, WHITE);
        } else {
            DrawText("VOCE MORREU NA FRONTEIRA...", SCREEN_WIDTH / 2 - 180, SCREEN_HEIGHT / 2 - 40, 22, RED);
            DrawText("A fome ou a sede extrema cobraram seu preco nas terras secas.", SCREEN_WIDTH / 2 - 240, SCREEN_HEIGHT / 2, 14, DARKGRAY);
            DrawText("Aperte ENTER para tentar novamente.", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 + 40, 14, DARKGRAY);

            if (IsKeyPressed(KEY_ENTER)) {
                resetJogo(&jogador);
                dia = 1;
                clima = 0.35f;
                acumuladorTempo = 0.0f;
                gameOver = false;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
