/*
* Kernel.c
*/

int le_tecla_simples(void);
void espera_tecla(void);

void delay(unsigned int count) {
    volatile unsigned int i;
    for(i = 0; i < count * 1000; i++) {
    }
}

void tela_limpa(void) {
    char *vidptr = (char*)0xb8000;
    unsigned int j = 0;
    
    while(j < 80 * 25 * 2) {
        vidptr[j] = ' ';
        vidptr[j+1] = 0x04;
        j = j + 2;
    }
}

// Variaveis do jogo
static int jumpHeight = 0;
static int gameSpeed = 3; // velocidade do jogo, quanto mais alto, mais lento
static int obstaclePosition = 0;
static int score = 0;
static int gameRunning = 0;
static unsigned int rng_state = 2463534242u;
static int obst_tipo = 2; // 0=cacto, 1=cacto pequeno, 2=passaro
static int gap_cols = 0;  // espaco entre obstaculos
static int moveEvery = 3; // frames por movimento
static int lastSpeedIncrease = 0; // controla quando foi a ultima aceleracao
static int vel_do_pulo = 0;      // faz subir e cair
static int gravidade_mais = 1;   // cai mais devagar
static int segura_topo = 0;      // fica um pouco no topo
static int quica_uma_vez = 0;    // não sei direito mas muda algo

// Dimensoes e posicao do dino + pulo
#define DINO_X 2
#define DINO_WIDTH 8
#define DINO_HEIGHT 8
#define MAX_JUMP 8


static int rndu(void) { // Gera números "aleatórios"
    rng_state = rng_state * 1103515245u + 12345u;
    return (int)((rng_state >> 16) & 0x7FFF);
}


static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
 
 
// printa string na posicao que eu quiser
void print_at_position(int x, int y, const char* str, char color) {
    char *vidptr = (char*)0xb8000;
    unsigned int pos = (y * 80 + x) * 2;
    unsigned int i = 0;
    
    while (str[i] != '\0' && pos < 80 * 25 * 2) {
        vidptr[pos] = str[i];
        vidptr[pos + 1] = color;
        pos += 2;
        i++;
    }
}

// teclas
int le_tecla_simples(void) {
    if (inb(0x64) & 0x01) {
        unsigned char sc = inb(0x60);
        if ((sc & 0x80) == 0) return sc;
    }
    return 0;
}

// espera tecla
void espera_tecla(void) {
    while (1) {
        if (le_tecla_simples()) break;
    }
}

// info
void tela_jogo(void) {
    tela_limpa();
    print_at_position(15, 2, "Pressione ESPACO para pular, ESC para parar", 0x0F);
    print_at_position(62, 2, "PONTOS: ", 0x0F);
    
    // chao
    char *vidptr = (char*)0xb8000;
    unsigned int pos = (24 * 80) * 2;
    for (int x = 0; x < 80; x++) {
        vidptr[pos] = '=';
        vidptr[pos + 1] = 0x0E;
        pos += 2;
    }
}

// dino
void dino(int jumpType) {
    static int animationState = 1;
    static int animTick = 0; // desacelera
    static const int SPRITE_H = DINO_HEIGHT;
    static const char* CLEAR_LINE = "            ";
    static const char* FRAME_A[8] = {
        " ***#  ",
        "*******",
        "***^^^ ",
        " ***** ",
        " ****##",
        " ****  ",
        "/ #### ",
        " *#    "
    };
    static const char* FRAME_B[8] = {
        " ***#  ",
        "*******",
        "***^^^ ",
        " ***** ",
        " ****##",
        " ****  ",
        "/ #### ",
        "  * *  "
    };
    static const char* FRAME_C[8] = {
        " ***#  ",
        "*******",
        "***^^^ ",
        " ***** ",
        " ****##",
        " ****  ",
        "/ #### ",
        "  *  * "
    };
    static int prev_baseY = -1;
    
    // limpa os frames "dirty rectangle" para sair o rastro do dino pulando
    int baseY = (24 - (SPRITE_H - 1)) - jumpHeight;
    
    int clear_inicio = baseY - 1;
    int clear_fim = baseY + SPRITE_H;
    if (prev_baseY >= 0) {
        if (prev_baseY - 1 < clear_inicio) clear_inicio = prev_baseY - 1;
        if (prev_baseY + SPRITE_H > clear_fim) clear_fim = prev_baseY + SPRITE_H;
    }
    if (clear_inicio < 0) clear_inicio = 0;
    if (clear_fim > 24) clear_fim = 24;
    for (int cy = clear_inicio; cy <= clear_fim; cy++) {
        print_at_position(DINO_X, cy, CLEAR_LINE, 0x00);
    }
    if (clear_inicio <= 24 && clear_fim >= 24) {
        print_at_position(DINO_X, 24, "============", 0x0E);
    }
    
    // troca de frame mais lenta
    animTick++;
    if (animTick >= 4 && jumpType == 0) { // só quando correndo
        animTick = 0;
        animationState++;
        if (animationState > 3) animationState = 1;
    }

    // escolhe frame
    const char** frame = FRAME_A;
    if (jumpType == 1 || jumpType == 2) {
        frame = FRAME_A;
    } else if (animationState == 1) {
        frame = FRAME_A;
    } else if (animationState == 2) {
        frame = FRAME_B;
    } else {
        frame = FRAME_C;
    }
    
    for (int i = 0; i < SPRITE_H; i++) {
        int ry = baseY + i;
        if (ry >= 0 && ry < 25) {
            print_at_position(DINO_X, ry, frame[i], 0x02);
        }
    }
    
    prev_baseY = baseY;
}

// cacto
void cacto(void) {
    // pula frames
    static int frameSkip = 0;
    frameSkip++;
    
    // se estamos no gap, nao desenha nada, so espera passos de movimento
    if (gap_cols > 0) {
        if (frameSkip >= moveEvery) {
            gap_cols--;
            frameSkip = 0;
        }
        return;
    }
    
    // limpa trilha no chao
    for (int i = 18; i <= 23; i++) {
        print_at_position(79 - obstaclePosition, i, "           ", 0x00);
        print_at_position(78 - obstaclePosition, i, "           ", 0x00);
        print_at_position(77 - obstaclePosition, i, "           ", 0x00);
    }
    // limpa trilha no alto (para passaro)
    int birdY = (24 - (DINO_HEIGHT - 1)) - (MAX_JUMP + 4);
    if (birdY < 1) birdY = 1;
    for (int i = birdY; i <= birdY + 2; i++) {
        if (i >= 0 && i < 25) {
            print_at_position(79 - obstaclePosition, i, "           ", 0x00);
            print_at_position(78 - obstaclePosition, i, "           ", 0x00);
            print_at_position(77 - obstaclePosition, i, "           ", 0x00);
        }
    }
    
    // desenha obstaculo atual
    if (obstaclePosition < 75) {
        int x = 75 - obstaclePosition;
        if (obst_tipo == 0) {
            // cacto grande
            print_at_position(x, 20, "#_ *** _#", 0x0A);
            print_at_position(x, 21, "** *** **", 0x0A);
            print_at_position(x, 22, "*********", 0x0A);
            print_at_position(x, 23, "   ***   ", 0x0A);
        } else if (obst_tipo == 1) {
            // cacto pequeno
            print_at_position(x, 22, " ***", 0x0A);
            print_at_position(x, 23, " ***", 0x0A);
        } else {
            // passaro (linha aprox max pulo)
            int y = birdY;
            print_at_position(x, y,   " \\_/ ", 0x07);
            print_at_position(x, y+1, " /_\\ ", 0x07);
        }
    }
    
    // move conforme moveEvery
    if (frameSkip >= moveEvery) {
        obstaclePosition++;
        frameSkip = 0;
        
        if (obstaclePosition > 80) {
            obstaclePosition = 0;
            score++;
            
            // placar
            char scoreStr[10];
            int temp = score;
            int digits = 0;
            
            if (temp == 0) {
                scoreStr[0] = '0';
                scoreStr[1] = '\0';
            } else {
                char tempStr[10];
                while (temp > 0) {
                    tempStr[digits] = '0' + (temp % 10);
                    temp /= 10;
                    digits++;
                }
                for (int i = 0; i < digits; i++) {
                    scoreStr[i] = tempStr[digits - 1 - i];
                }
                scoreStr[digits] = '\0';
            }
            
            print_at_position(69, 2, "     ", 0x0F);
            print_at_position(69, 2, scoreStr, 0x0F);
            
            // sistema de aceleracao controlada com alfa adequado
            int intervaloPorVelocidade = 5 + (score / 10); // intervalo cresce com score
            if (score - lastSpeedIncrease >= intervaloPorVelocidade) {
                if (gameSpeed > 1) {
                    gameSpeed--;
                    lastSpeedIncrease = score;
                } else if (moveEvery > 1) {
                    moveEvery--;
                    lastSpeedIncrease = score;
                }
            }
            
            // prepara proximo obstaculo com gap minimo 14
            gap_cols = 14 + (rndu() % 12);
            // escolhe tipo
            if (score >= 15 && (rndu() % 3) == 0) {
                obst_tipo = 2; // passaro
            } else {
                obst_tipo = (rndu() % 2); // 0 ou 1
            }
        }
    }
}

// teclado
int check_keyboard(void) {
    // le teclado
    if (inb(0x64) & 0x01) {
        unsigned char sc = inb(0x60);
        if (sc & 0x80) {
            return 0;
        }
        if (sc == 0x39) return ' ';
        if (sc == 0x01) return 27;
    }
    return 0;
}

// loop
void dino_game(void) {
    int input;
    int isJumping = 0;
    int frameCounter = 0;

    // zera pulo
    jumpHeight = 0;
    
    // debug
    print_at_position(30, 5, "Jogo iniciado!", 0x0C);
    delay(1000);
    
    tela_jogo();
    
    while (1) {
        frameCounter++;
        
        // pulo simples
        input = check_keyboard();
        if (input == ' ' && !isJumping && jumpHeight == 0) {
            isJumping = 1;
            vel_do_pulo = 16; // sobe mais
            segura_topo = 0;  // segura so no pico
        } else if (input == 27) {
            print_at_position(28, 10, "Saindo do jogo...", 0x0C);
            delay(1000);
            return;
        }

        if (isJumping) {
            // topo curto no pico
            int vel_antes = vel_do_pulo;
            vel_do_pulo -= gravidade_mais;
            if (vel_antes > 0 && vel_do_pulo <= 0 && segura_topo == 0) {
                segura_topo = 2; // segura 2 frames
                vel_do_pulo = 0;
            }
            if (segura_topo > 0) {
                segura_topo--;
                vel_do_pulo = 0;
            }

            // move
            if (vel_do_pulo > 0) {
                jumpHeight += 2;
            } else if (vel_do_pulo < 0) {
                static int acum = 0; // meio passo
                acum ^= 1;
                if (acum) jumpHeight -= 1;
            }
            if (jumpHeight < 0) jumpHeight = 0;
            if (jumpHeight > MAX_JUMP + 12) jumpHeight = MAX_JUMP + 12;

            // quica
            if (vel_do_pulo <= 0 && jumpHeight == 0) {
                if (quica_uma_vez == 0) {
                    vel_do_pulo = 2; // quique mínimo
                    quica_uma_vez = 1;
                } else {
                    isJumping = 0;
                    quica_uma_vez = 0;
                    vel_do_pulo = 0;
                }
            }
        }
        
        // desenha
        if (isJumping) {
            dino(vel_do_pulo > 0 ? 1 : 2);
        } else {
            dino(0);
        }
        
        // cacto
        cacto();
        
        // tempo
        delay(gameSpeed * 1000);
        
        // colisao
        int dinoLeft = DINO_X;
        int dinoRight = DINO_X + DINO_WIDTH - 1;
        int dinoTop = (24 - (DINO_HEIGHT - 1)) - jumpHeight;
        int dinoBottom = dinoTop + (DINO_HEIGHT - 1);
        int ox = 75 - obstaclePosition;
        int oLeft = ox + 2, oRight = ox + 2, oTop = 20, oBottom = 22;
        if (obst_tipo == 0) { // cacto grande - caixa um pouco maior
            oLeft = ox + 2; oRight = ox + 6; oTop = 20; oBottom = 23;
        } else if (obst_tipo == 1) { // cacto pequeno
            oLeft = ox + 1; oRight = ox + 2; oTop = 22; oBottom = 23;
        } else { // passaro
            int by = (24 - (DINO_HEIGHT - 1)) - (MAX_JUMP + 4); if (by < 1) by = 1;
            oLeft = ox; oRight = ox + 4; oTop = by; oBottom = by + 1;
        }
        int overlapX = (dinoLeft <= oRight) && (dinoRight >= oLeft);
        int overlapY = (dinoTop <= oBottom) && (dinoBottom >= oTop);
        if (overlapX && overlapY) {
            print_at_position(25, 10, "********************************", 0x0C);
            print_at_position(25, 11, "*           Perdeu!            *", 0x0C);
            print_at_position(25, 12, "*  o dino morreu brutalmente   *", 0x0C);
            print_at_position(25, 13, "*       Pontuacao:             *", 0x0C);
            print_at_position(25, 14, "********************************", 0x0C);
            
            // placar final
            char scoreStr[10];
            int temp = score;
            int digits = 0;
            if (temp == 0) {
                print_at_position(44, 13, "0", 0x0E);
            } else {
                char tempStr[10];
                while (temp > 0) {
                    tempStr[digits] = '0' + (temp % 10);
                    temp /= 10;
                    digits++;
                }
                for (int i = 0; i < digits; i++) {
                    scoreStr[i] = tempStr[digits - 1 - i];
                }
                scoreStr[digits] = '\0';
                print_at_position(44, 13, scoreStr, 0x0E);
            };

            print_at_position(32, 16, "    Pressione   ", 0x0c);
            print_at_position(32, 17, " qualquer tecla ", 0x0C);
            
            espera_tecla();
            
            score = 0;
            obstaclePosition = 0;
            jumpHeight = 0;
            vel_do_pulo = 0;
            gameSpeed = 3; // reset para valor inicial
            moveEvery = 3; // reset para valor inicial
            lastSpeedIncrease = 0; // reset contador de aceleracao
            isJumping = 0;
            
            tela_jogo();
        }
    }
}

void ascii_art(void) {
    print_at_position(15, 1, "       .                 .           ", 0x0C);
    print_at_position(15, 2, "     dP    Dino Run porra  9b        ", 0x0C);
    print_at_position(15, 3, "     dX     jogue agora!     Xb      ", 0x0C);
    print_at_position(15, 4, "    __                         __    ", 0x0C);
    print_at_position(15, 5, "  dXXXXbo.                 .odXXXXb d", 0x0C);
    print_at_position(15, 6, " XXXXXXXXOo.           .oOXXXXXXXXVXX", 0x0C);
    print_at_position(15, 7, " XX'~   ~'OOO8b   d8OOO'~   ~'XXXXXXX", 0x0C);
    print_at_position(15, 8, " 9XX'          '98v8P'          'XXP'", 0x0C);
    print_at_position(15, 9, "  9X.          .db|db.          .XP", 0x0C);
    print_at_position(15, 10, "    )b.  .dbo.dP''v''9b.odb.  .dX(", 0x0C);
    print_at_position(15, 11, ",dXXXXXXXXXXXb     dXXXXXXXXXXXb.", 0x0C);
    print_at_position(15, 12, "dXXXXXXXXXXXP'   .   '9XXXXXXXXXXXb", 0x0C);
    print_at_position(15, 13, "dXXXXXXXXXXXXb   d|b   dXXXXXXXXXXXXb", 0x0C);
    print_at_position(15, 14, "9XXb'   'XXXXXb.dX|Xb.dXXXXX'   'dXXP", 0x0C);
    print_at_position(15, 15, "''      9XXXXXX(   )XXXXXXP      ''", 0x0C);
    print_at_position(15, 16, "        XXXX X.'v'.X XXXX", 0x0C);
    print_at_position(15, 17, "        XP^X''b   d''X^XX", 0x0C);
    print_at_position(15, 18, "        X. 9  ''   '  P )X", 0x0C);
    print_at_position(15, 19, "        'b  '       '  d'", 0x0C);
    print_at_position(15, 20, "        '             '", 0x0C);

    print_at_position(7, 22, "Pressione qualquer tecla para iniciar essa bomba!", 0x0B);
}

void tela_inicio(void) {
    tela_limpa();
    
    ascii_art();

    espera_tecla();
    tela_limpa();
}

void kmain(void) {

    tela_inicio();
    
    delay(100);
    
    tela_limpa();
     
    dino_game();
    
    return;
}