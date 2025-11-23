#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Sala {
    char *nome;
    struct Sala *esq;
    struct Sala *dir;
} Sala;

/**
 * criarSala(nome)
 * Cria dinamicamente uma sala (nó da árvore) copiando o nome.
 * Retorna ponteiro para a Sala criada ou NULL se falhar.
 */
Sala *criarSala(const char *nome) {
    Sala *s = (Sala *) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro: falha ao alocar memória para Sala.\n");
        return NULL;
    }
    size_t len = strlen(nome) + 1;
    s->nome = (char *) malloc(len);
    if (!s->nome) {
        fprintf(stderr, "Erro: falha ao alocar memória para nome da sala.\n");
        free(s);
        return NULL;
    }
    memcpy(s->nome, nome, len);
    s->esq = s->dir = NULL;
    return s;
}

/**
 * liberarSalas(raiz)
 * Libera recursivamente a memória usada pela árvore.
 */
void liberarSalas(Sala *raiz) {
    if (!raiz) return;
    liberarSalas(raiz->esq);
    liberarSalas(raiz->dir);
    free(raiz->nome);
    free(raiz);
}

/**
 * explorarSalas(inicio)
 * Permite a navegação interativa pela mansão a partir da sala 'inicio'.
 * O jogador escolhe 'e' (esquerda), 'd' (direita) ou 's' (sair).
 * Exibe as salas visitadas durante a exploração.
 */
void explorarSalas(Sala *inicio) {
    if (!inicio) {
        printf("Mapa vazio. Nada a explorar.\n");
        return;
    }

    // vetor simples para guardar o percurso (apenas ponteiros para nomes).
    const char **visitadas = NULL;
    size_t cap = 0, n = 0;

    Sala *atual = inicio;
    char buf[64];

    while (1) {
        // registrar visita
        if (n + 1 > cap) {
            size_t nova = (cap == 0) ? 4 : cap * 2;
            const char **tmp = realloc(visitadas, nova * sizeof(const char *));
            if (!tmp) {
                fprintf(stderr, "Erro: falha ao alocar memória para histórico de visitas.\n");
                break;
            }
            visitadas = tmp;
            cap = nova;
        }
        visitadas[n++] = atual->nome;

        // exibe sala atual
        printf("\nVocê está na sala: %s\n", atual->nome);

        // checar se é folha
        int temEsq = (atual->esq != NULL);
        int temDir = (atual->dir != NULL);
        if (!temEsq && !temDir) {
            printf("Esta sala não possui caminhos. Fim da exploração.\n");
            break;
        }

        // mostrar opções disponíveis
        printf("Escolha um caminho:\n");
        if (temEsq) printf("  (e) esquerda\n");
        if (temDir) printf("  (d) direita\n");
        printf("  (s) sair da exploração\n");
        printf("Opção: ");

        // ler entrada
        if (!fgets(buf, sizeof(buf), stdin)) {
            // EOF ou erro
            printf("\nEntrada interrompida. Saindo.\n");
            break;
        }

        // pegar primeiro caractere não-espaco
        char escolha = '\0';
        for (size_t i = 0; buf[i] != '\0'; ++i) {
            if (!isspace((unsigned char)buf[i])) {
                escolha = tolower((unsigned char)buf[i]);
                break;
            }
        }

        if (escolha == 's') {
            printf("Você escolheu sair da exploração.\n");
            break;
        } else if (escolha == 'e') {
            if (temEsq) {
                atual = atual->esq;
                continue;
            } else {
                printf("Caminho à esquerda não disponível. Escolha outra opção.\n");
                continue;
            }
        } else if (escolha == 'd') {
            if (temDir) {
                atual = atual->dir;
                continue;
            } else {
                printf("Caminho à direita não disponível. Escolha outra opção.\n");
                continue;
            }
        } else {
            printf("Opção inválida. Use 'e', 'd' ou 's'.\n");
            continue;
        }
    }

    // Mostrar lista completa de salas visitadas
    if (n > 0) {
        printf("\nSalas visitadas (em ordem de visita):\n");
        for (size_t i = 0; i < n; ++i) {
            printf("  %zu. %s\n", i + 1, visitadas[i]);
        }
    } else {
        printf("\nNenhuma sala visitada.\n");
    }

    free(visitadas);
}

int main(void) {
    // Montagem manual do mapa (árvore binária)
    // Hall de entrada é a raiz
    Sala *hall = criarSala("Hall de entrada");

    // nível 1
    Sala *salaEstar = criarSala("Sala de estar");
    Sala *corredor = criarSala("Corredor");

    hall->esq = salaEstar;
    hall->dir = corredor;

    // nível 2 - salaEstar
    Sala *cozinha = criarSala("Cozinha");
    Sala *biblioteca = criarSala("Biblioteca");
    salaEstar->esq = cozinha;
    salaEstar->dir = biblioteca;

    // nível 2 - corredor
    Sala *quarto = criarSala("Quarto");
    Sala *jardim = criarSala("Jardim");
    corredor->esq = quarto;
    corredor->dir = jardim;

    // folhas podem ter NULLs (já estão assim). Ex. cozinha, biblioteca, quarto, jardim são folhas.

    printf("=== Detective Quest: Exploração da Mansão ===\n");
    printf("Iniciando a exploração a partir do Hall de entrada.\n");
    printf("Navegue com: 'e' = esquerda, 'd' = direita, 's' = sair.\n");

    explorarSalas(hall);

    // liberar memória
    liberarSalas(hall);

    printf("\nExploração encerrada. Obrigado por jogar (teste de console).\n");
    return 0;
}
