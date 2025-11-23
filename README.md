#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* --------------------------
   Estruturas de dados
   -------------------------- */

/* Nó da árvore da mansão (cômodo) */
typedef struct Sala {
    char *nome;         /* nome da sala */
    char *pista;        /* pista opcional (NULL se não houver) */
    struct Sala *esq;   /* ponteiro para cômodo à esquerda */
    struct Sala *dir;   /* ponteiro para cômodo à direita */
} Sala;

/* Nó da BST que guarda pistas coletadas */
typedef struct PistaNode {
    char *pista;
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* --------------------------
   Helpers para strings
   -------------------------- */

/* strdup pode não existir em todos os compiladores padrão C;
   criamos uma versão simples e segura aqui */
static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    return copy;
}

/* Função para limpar espaço e ler uma linha do stdin (remover newline) */
static void ler_linha(char *buf, size_t tam) {
    if (!fgets(buf, (int)tam, stdin)) {
        buf[0] = '\0';
        return;
    }
    /* remover newline final, se existir */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
}

/* --------------------------
   Funções para salas (mansão)
   -------------------------- */

/**
 * criarSala(nome, pista)
 * Cria dinamicamente um cômodo com nome e pista (pista pode ser NULL).
 * Retorna ponteiro para Sala ou NULL em caso de erro.
 */
Sala *criarSala(const char *nome, const char *pista) {
    Sala *s = malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro: falha na alocação de Sala.\n");
        return NULL;
    }
    s->nome = my_strdup(nome ? nome : "");   /* copia o nome */
    s->pista = (pista && pista[0] != '\0') ? my_strdup(pista) : NULL;
    s->esq = s->dir = NULL;
    return s;
}

/**
 * liberarSalas(raiz)
 * Libera recursivamente memória usada pela árvore de salas.
 */
void liberarSalas(Sala *raiz) {
    if (!raiz) return;
    liberarSalas(raiz->esq);
    liberarSalas(raiz->dir);
    free(raiz->nome);
    if (raiz->pista) free(raiz->pista);
    free(raiz);
}

/* --------------------------
   Funções para BST de pistas
   -------------------------- */

/**
 * inserirPista(raiz, pista)
 * Insere 'pista' na BST ordenada por ordem alfabética.
 * Evita inserir duplicatas (comparação por strcmp).
 * Retorna a nova raiz (pode ser a mesma).
 */
PistaNode *inserirPista(PistaNode *raiz, const char *pista) {
    if (!pista || pista[0] == '\0') return raiz; /* nada a inserir */

    if (raiz == NULL) {
        PistaNode *n = malloc(sizeof(PistaNode));
        if (!n) {
            fprintf(stderr, "Erro: falha ao alocar PistaNode.\n");
            return NULL;
        }
        n->pista = my_strdup(pista);
        n->esq = n->dir = NULL;
        return n;
    }

    int cmp = strcmp(pista, raiz->pista);
    if (cmp == 0) {
        /* já existe: não inserimos duplicata */
        return raiz;
    } else if (cmp < 0) {
        raiz->esq = inserirPista(raiz->esq, pista);
    } else {
        raiz->dir = inserirPista(raiz->dir, pista);
    }
    return raiz;
}

/**
 * exibirPistas(raiz)
 * Percorre a BST em ordem crescente (in-order) e imprime as pistas.
 */
void exibirPistas(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistas(raiz->esq);
    printf(" - %s\n", raiz->pista);
    exibirPistas(raiz->dir);
}

/**
 * liberarPistas(raiz)
 * Libera recursivamente memória da BST de pistas.
 */
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz->pista);
    free(raiz);
}

/* --------------------------
   Exploração da mansão + coleta de pistas
   -------------------------- */

/**
 * explorarSalasComPistas(inicio, ponteiro_para_raiz_pistas)
 * Controle interativo da navegação:
 *  - 'e' para esquerda
 *  - 'd' para direita
 *  - 's' para sair (encerra exploração)
 *
 * Cada vez que entra em uma sala que possui pista não vazia, a pista é automaticamente
 * adicionada à árvore de pistas (BST). Pistas duplicadas não são inseridas duas vezes.
 *
 * Ao final, imprime histórico de salas visitadas.
 */
void explorarSalasComPistas(Sala *inicio, PistaNode **pistasRoot) {
    if (!inicio) {
        printf("Mapa vazio. Nada a explorar.\n");
        return;
    }

    /* vetor simples para registrar o histórico de visita (strings dos nomes) */
    const char **historico = NULL;
    size_t cap = 0, n = 0;

    Sala *atual = inicio;
    char entrada[80];

    while (1) {
        /* registrar visita */
        if (n + 1 > cap) {
            size_t nova = (cap == 0) ? 6 : cap * 2;
            const char **tmp = realloc((void *)historico, nova * sizeof(const char *));
            if (!tmp) {
                fprintf(stderr, "Erro: falha ao alocar histórico de visitas.\n");
                break;
            }
            historico = tmp;
            cap = nova;
        }
        historico[n++] = atual->nome;

        /* mostrar sala atual */
        printf("\nVocê está na sala: %s\n", atual->nome);

        /* se houver pista na sala, coletar automaticamente */
        if (atual->pista != NULL && atual->pista[0] != '\0') {
            /* tentar inserir na BST (inserirPista evita duplicatas) */
            size_t antes = 0; /* simples indicador: não vamos contar nós, só informar */
            *pistasRoot = inserirPista(*pistasRoot, atual->pista);
            printf("Pista encontrada e coletada: \"%s\"\n", atual->pista);
            /* opcional: podemos "marcar" a pista como coletada removendo-a da sala,
               mas mantemos na sala (não alteramos o mapa original). */
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        /* verificar caminhos disponíveis */
        int temEsq = (atual->esq != NULL);
        int temDir = (atual->dir != NULL);

        /* construir menu de opções */
        printf("Escolha uma opção:\n");
        if (temEsq) printf("  (e) Ir para a esquerda\n");
        if (temDir) printf("  (d) Ir para a direita\n");
        printf("  (s) Sair da exploração\n");
        printf("Opção: ");

        ler_linha(entrada, sizeof(entrada));

        /* pegar primeiro caractere não-espaço */
        char escolha = '\0';
        for (size_t i = 0; entrada[i] != '\0'; ++i) {
            if (!isspace((unsigned char)entrada[i])) {
                escolha = tolower((unsigned char)entrada[i]);
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
                printf("Caminho à esquerda não disponível. Tente outra opção.\n");
                continue;
            }
        } else if (escolha == 'd') {
            if (temDir) {
                atual = atual->dir;
                continue;
            } else {
                printf("Caminho à direita não disponível. Tente outra opção.\n");
                continue;
            }
        } else {
            printf("Opção inválida. Use 'e', 'd' ou 's'.\n");
            continue;
        }
    }

    /* mostrar histórico de salas visitadas */
    if (n > 0) {
        printf("\nHistórico de salas visitadas (ordem de chegada):\n");
        for (size_t i = 0; i < n; ++i) {
            printf("  %zu. %s\n", i + 1, historico[i]);
        }
    } else {
        printf("\nNenhuma sala visitada.\n");
    }

    free(historico);
}

/* --------------------------
   Função main: monta mapa e inicia exploração
   -------------------------- */

int main(void) {
    /* Montagem manual do mapa (árvore binária).
       Cada sala pode ter uma pista (string) ou não (NULL). */

    Sala *hall = criarSala("Hall de Entrada", "Pegadas de lama");
    Sala *salaEstar = criarSala("Sala de Estar", "Livro com página faltando");
    Sala *corredor = criarSala("Corredor", NULL);
    Sala *cozinha = criarSala("Cozinha", "Chave perdida");
    Sala *biblioteca = criarSala("Biblioteca", NULL);
    Sala *quarto = criarSala("Quarto", "Lençol manchado");
    Sala *jardim = criarSala("Jardim", "Gaveta perdida");

    /* conectar nós (fixo) */
    hall->esq = salaEstar;
    hall->dir = corredor;

    salaEstar->esq = cozinha;
    salaEstar->dir = biblioteca;

    corredor->esq = quarto;
    corredor->dir = jardim;

    /* A BST de pistas começa vazia */
    PistaNode *pistasRoot = NULL;

    printf("=== Detective Quest: Caça às Pistas (nível aventureiro) ===\n");
    printf("Iniciando no Hall de Entrada.\n");
    printf("Navegue com: 'e' = esquerda, 'd' = direita, 's' = sair.\n");

    explorarSalasComPistas(hall, &pistasRoot);

    /* ao final, exibir todas as pistas coletadas em ordem alfabética */
    printf("\n--- Pistas coletadas (em ordem alfabética) ---\n");
    if (pistasRoot == NULL) {
        printf("Nenhuma pista foi coletada.\n");
    } else {
        exibirPistas(pistasRoot);
    }

    /* liberar memória */
    liberarPistas(pistasRoot);
    liberarSalas(hall);

    printf("\nExploração finalizada. Obrigado por jogar (versão console).\n");
    return 0;
}
