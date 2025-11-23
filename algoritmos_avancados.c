#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ===========================
   Tipos e estruturas
   =========================== */

/* Nó da árvore da mansão (cômodo) */
typedef struct Sala {
    char *nome;         /* nome do cômodo */
    char *pista;        /* pista associada (NULL se não houver) */
    struct Sala *esq;   /* filho à esquerda */
    struct Sala *dir;   /* filho à direita */
} Sala;

/* Nó da BST para armazenar pistas coletadas (ordenadas) */
typedef struct PistaNode {
    char *pista;
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* Entrada na tabela hash: chave = pista, valor = suspeito */
typedef struct HashEntry {
    char *chave;               /* a pista */
    char *suspeito;            /* suspeito associado */
    struct HashEntry *prox;    /* lista encadeada para colisões */
} HashEntry;

/* Tabela hash */
typedef struct HashTable {
    HashEntry **buckets;
    size_t tamanho; /* número de baldes (buckets) */
} HashTable;

/* Lista simples para contar pistas por suspeito durante verificação */
typedef struct SuspeitoConta {
    char *nome;
    int cont;
    struct SuspeitoConta *prox;
} SuspeitoConta;

/* ===========================
   Helpers para strings / memória
   =========================== */

/* duplicador de string (substitui strdup para portabilidade) */
static char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *c = malloc(n);
    if (!c) return NULL;
    memcpy(c, s, n);
    return c;
}

/* lê linha do stdin e remove newline */
static void ler_linha(char *buf, size_t tam) {
    if (!fgets(buf, (int)tam, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
}

/* transforma string para minúsculas (obriga comparar nomes sem diferenciar case) */
static void str_lower(char *s) {
    for (size_t i = 0; s[i]; ++i) s[i] = (char)tolower((unsigned char)s[i]);
}

/* ===========================
   Funções de Sala (mansão)
   =========================== */

/**
 * criarSala(nome, pista)
 * Cria dinamicamente um cômodo com nome e pista (pista pode ser NULL).
 * Retorna ponteiro para Sala ou NULL em caso de erro.
 */
Sala *criarSala(const char *nome, const char *pista) {
    Sala *s = malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro: falha alocação Sala.\n");
        return NULL;
    }
    s->nome = my_strdup(nome ? nome : "");
    s->pista = (pista && pista[0] != '\0') ? my_strdup(pista) : NULL;
    s->esq = s->dir = NULL;
    return s;
}

/* libera memória da árvore de salas */
void liberarSalas(Sala *raiz) {
    if (!raiz) return;
    liberarSalas(raiz->esq);
    liberarSalas(raiz->dir);
    free(raiz->nome);
    if (raiz->pista) free(raiz->pista);
    free(raiz);
}

/* ===========================
   Funções para BST de pistas
   =========================== */

/* inserirPista(raiz, pista)
 * Insere 'pista' na BST ordenada por strcmp. Evita duplicatas.
 * Retorna a raiz (possivelmente atualizada).
 */
PistaNode *inserirPista(PistaNode *raiz, const char *pista) {
    if (!pista || pista[0] == '\0') return raiz;
    if (raiz == NULL) {
        PistaNode *n = malloc(sizeof(PistaNode));
        if (!n) {
            fprintf(stderr, "Erro: alocação PistaNode.\n");
            return NULL;
        }
        n->pista = my_strdup(pista);
        n->esq = n->dir = NULL;
        return n;
    }
    int cmp = strcmp(pista, raiz->pista);
    if (cmp == 0) {
        /* já coletada — não duplicar */
        return raiz;
    } else if (cmp < 0) {
        raiz->esq = inserirPista(raiz->esq, pista);
    } else {
        raiz->dir = inserirPista(raiz->dir, pista);
    }
    return raiz;
}

/* exibirPistas: percorre em ordem e imprime */
void exibirPistas(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistas(raiz->esq);
    printf(" - %s\n", raiz->pista);
    exibirPistas(raiz->dir);
}

/* liberar BST de pistas */
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz->pista);
    free(raiz);
}

/* ===========================
   Tabela Hash (pista -> suspeito)
   =========================== */

/* função hash simples (djb2) */
static unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

/* criar tabela hash com 'tamanho' buckets */
HashTable *criarHash(size_t tamanho) {
    HashTable *h = malloc(sizeof(HashTable));
    if (!h) return NULL;
    h->buckets = calloc(tamanho, sizeof(HashEntry *));
    if (!h->buckets) {
        free(h);
        return NULL;
    }
    h->tamanho = tamanho;
    return h;
}

/**
 * inserirNaHash(ht, pista, suspeito)
 * Insere a associação pista -> suspeito na tabela hash.
 * Se a chave já existe, sobrescreve o suspeito.
 */
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!ht || !pista || !suspeito) return;
    unsigned long h = hash_djb2(pista) % ht->tamanho;
    /* procurar se já existe */
    HashEntry *ent = ht->buckets[h];
    while (ent) {
        if (strcmp(ent->chave, pista) == 0) {
            /* sobrescreve o suspeito existente */
            free(ent->suspeito);
            ent->suspeito = my_strdup(suspeito);
            return;
        }
        ent = ent->prox;
    }
    /* não existe — criar novo */
    HashEntry *novo = malloc(sizeof(HashEntry));
    if (!novo) {
        fprintf(stderr, "Erro: alocar HashEntry\n");
        return;
    }
    novo->chave = my_strdup(pista);
    novo->suspeito = my_strdup(suspeito);
    novo->prox = ht->buckets[h];
    ht->buckets[h] = novo;
}

/**
 * encontrarSuspeito(ht, pista)
 * Retorna o nome do suspeito associado à pista, ou NULL se não achar.
 * A string retornada é a que está armazenada na hash (não liberar externamente).
 */
char *encontrarSuspeito(HashTable *ht, const char *pista) {
    if (!ht || !pista) return NULL;
    unsigned long h = hash_djb2(pista) % ht->tamanho;
    HashEntry *ent = ht->buckets[h];
    while (ent) {
        if (strcmp(ent->chave, pista) == 0) return ent->suspeito;
        ent = ent->prox;
    }
    return NULL;
}

/* liberar tabela hash */
void liberarHash(HashTable *ht) {
    if (!ht) return;
    for (size_t i = 0; i < ht->tamanho; ++i) {
        HashEntry *e = ht->buckets[i];
        while (e) {
            HashEntry *next = e->prox;
            free(e->chave);
            free(e->suspeito);
            free(e);
            e = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

/* listar suspeitos únicos contidos na hash (para ajudar o jogador) */
void listarSuspeitosHash(HashTable *ht) {
    if (!ht) return;
    SuspeitoConta *lista = NULL;
    for (size_t i = 0; i < ht->tamanho; ++i) {
        HashEntry *e = ht->buckets[i];
        while (e) {
            /* checar se já está na lista */
            SuspeitoConta *p = lista;
            int achou = 0;
            while (p) {
                if (strcmp(p->nome, e->suspeito) == 0) { achou = 1; break; }
                p = p->prox;
            }
            if (!achou) {
                SuspeitoConta *novo = malloc(sizeof(SuspeitoConta));
                novo->nome = my_strdup(e->suspeito);
                novo->cont = 0;
                novo->prox = lista;
                lista = novo;
            }
            e = e->prox;
        }
    }
    printf("Suspeitos conhecidos:\n");
    SuspeitoConta *q = lista;
    if (!q) {
        printf("  (nenhum suspeito cadastrado)\n");
    }
    while (q) {
        printf("  - %s\n", q->nome);
        SuspeitoConta *tmp = q;
        q = q->prox;
        free(tmp->nome);
        free(tmp);
    }
}

/* ===========================
   Exploração + coleta de pistas
   =========================== */

/**
 * explorarSalas(inicio, ht, ponteiro_raiz_pistas)
 * Permite navegar pela mansão (arvore binaria). Ao entrar numa sala, exibe
 * a pista (se existir), insere na BST de pistas e mostra a quem a pista aponta
 * (consultando a hash).
 *
 * Comandos: 'e' esquerda, 'd' direita, 's' sair.
 */
void explorarSalas(Sala *inicio, HashTable *ht, PistaNode **pistasRoot) {
    if (!inicio) {
        printf("Mapa vazio.\n");
        return;
    }

    const char **historico = NULL;
    size_t cap = 0, n = 0;
    Sala *atual = inicio;
    char buf[128];

    while (1) {
        /* registrar visita */
        if (n + 1 > cap) {
            size_t nova = (cap == 0) ? 6 : cap * 2;
            const char **tmp = realloc((void *)historico, nova * sizeof(const char *));
            if (!tmp) { fprintf(stderr, "Erro: histórico\n"); break; }
            historico = tmp;
            cap = nova;
        }
        historico[n++] = atual->nome;

        /* mostrar onde está */
        printf("\nVocê está na sala: %s\n", atual->nome);
        if (atual->pista && atual->pista[0] != '\0') {
            printf("Pista encontrada: \"%s\"\n", atual->pista);
            /* coletar e inserir na BST (evita duplicatas) */
            *pistasRoot = inserirPista(*pistasRoot, atual->pista);
            /* mostrar suspeito relacionado (se existir na hash) */
            char *sus = encontrarSuspeito(ht, atual->pista);
            if (sus) {
                printf("  (Essa pista aponta para: %s)\n", sus);
            } else {
                printf("  (Nenhum suspeito conhecido para esta pista)\n");
            }
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        /* opções de movimento */
        int temEsq = (atual->esq != NULL);
        int temDir = (atual->dir != NULL);
        printf("Escolha uma opção:\n");
        if (temEsq) printf("  (e) Ir para a esquerda\n");
        if (temDir) printf("  (d) Ir para a direita\n");
        printf("  (s) Sair da exploração\n");
        printf("Opção: ");
        ler_linha(buf, sizeof(buf));
        char escolha = '\0';
        for (size_t i = 0; buf[i]; ++i) {
            if (!isspace((unsigned char)buf[i])) { escolha = (char)tolower((unsigned char)buf[i]); break; }
        }

        if (escolha == 's') {
            printf("Saindo da exploração.\n");
            break;
        } else if (escolha == 'e') {
            if (temEsq) { atual = atual->esq; continue; }
            else { printf("Caminho à esquerda não disponível.\n"); continue; }
        } else if (escolha == 'd') {
            if (temDir) { atual = atual->dir; continue; }
            else { printf("Caminho à direita não disponível.\n"); continue; }
        } else {
            printf("Opção inválida. Use 'e', 'd' ou 's'.\n");
            continue;
        }
    }

    /* mostrar histórico de visitas */
    if (n > 0) {
        printf("\nHistórico de salas visitadas:\n");
        for (size_t i = 0; i < n; ++i) printf("  %zu. %s\n", i+1, historico[i]);
    } else {
        printf("\nNenhuma sala visitada.\n");
    }
    free(historico);
}

/* ===========================
   Verificação final (julgamento)
   =========================== */

/* incrementa contador do suspeito na lista (ou adiciona se não existir) */
static void conta_suspeito_add(SuspeitoConta **lista, const char *nome) {
    if (!nome) return;
    SuspeitoConta *p = *lista;
    while (p) {
        if (strcmp(p->nome, nome) == 0) { p->cont += 1; return; }
        p = p->prox;
    }
    /* não achou -> cria novo nó */
    SuspeitoConta *novo = malloc(sizeof(SuspeitoConta));
    novo->nome = my_strdup(nome);
    novo->cont = 1;
    novo->prox = *lista;
    *lista = novo;
}

/* libera lista de contagem */
static void liberar_conta_lista(SuspeitoConta *l) {
    while (l) {
        SuspeitoConta *t = l->prox;
        free(l->nome);
        free(l);
        l = t;
    }
}

/* atravessa a BST de pistas e cria contagem por suspeito usando hash */
void contar_pistas_por_suspeito(PistaNode *raiz, HashTable *ht, SuspeitoConta **lista) {
    if (!raiz) return;
    /* in-order ou pre-order, não importa para contar */
    contar_pistas_por_suspeito(raiz->esq, ht, lista);
    char *sus = encontrarSuspeito(ht, raiz->pista);
    if (sus) conta_suspeito_add(lista, sus);
    contar_pistas_por_suspeito(raiz->dir, ht, lista);
}

/**
 * verificarSuspeitoFinal(pistasRoot, ht)
 * Pede ao jogador para acusar um suspeito; verifica se há pelo menos 2 pistas
 * que apontem para esse suspeito e imprime o resultado do julgamento.
 */
void verificarSuspeitoFinal(PistaNode *pistasRoot, HashTable *ht) {
    if (!pistasRoot) {
        printf("\nVocê não coletou pistas suficientes para acusar alguém.\n");
        return;
    }

    /* mostrar as pistas coletadas */
    printf("\nPistas coletadas (em ordem):\n");
    exibirPistas(pistasRoot);

    /* mostrar suspeitos conhecidos para ajudar o jogador */
    printf("\n");
    listarSuspeitosHash(ht);

    /* contar quantas pistas apontam para cada suspeito */
    SuspeitoConta *lista = NULL;
    contar_pistas_por_suspeito(pistasRoot, ht, &lista);

    /* solicitar acusação */
    char buf[128];
    printf("\nQuem você acusa? Digite o nome do suspeito: ");
    ler_linha(buf, sizeof(buf));
    if (buf[0] == '\0') {
        printf("Nenhum nome informado. Acusação cancelada.\n");
        liberar_conta_lista(lista);
        return;
    }

    /* simplificar: comparar sem diferenciar maiúsculas/minúsculas */
    char acusado_norm[128];
    strncpy(acusado_norm, buf, sizeof(acusado_norm)-1);
    acusado_norm[sizeof(acusado_norm)-1] = '\0';
    str_lower(acusado_norm);

    /* buscar na lista */
    SuspeitoConta *p = lista;
    int achou = 0;
    int cont = 0;
    while (p) {
        char nome_norm[128];
        strncpy(nome_norm, p->nome, sizeof(nome_norm)-1);
        nome_norm[sizeof(nome_norm)-1] = '\0';
        str_lower(nome_norm);
        if (strcmp(nome_norm, acusado_norm) == 0) {
            achou = 1;
            cont = p->cont;
            break;
        }
        p = p->prox;
    }

    if (!achou) {
        printf("\nO nome '%s' não corresponde a nenhum suspeito com pistas coletadas.\n", buf);
        printf("Resultado: Acusação improcedente.\n");
    } else {
        printf("\nVocê acusou: %s\n", buf);
        printf("Número de pistas que apontam para esse suspeito: %d\n", cont);
        if (cont >= 2) {
            printf("Veredito: Você reuniu evidências suficientes. Suspeito considerado CULPADO.\n");
        } else {
            printf("Veredito: Evidências insuficientes (são necessárias pelo menos 2 pistas). Suspeito inocentado.\n");
        }
    }

    liberar_conta_lista(lista);
}

/* ===========================
   main: monta mapa, hash, roda exploração e verificação
   =========================== */

int main(void) {
    /* Montagem manual do mapa (árvore binária de salas) */
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

    /* criar tabela hash e inserir associações pista -> suspeito */
    HashTable *ht = criarHash(31); /* 31 buckets é suficiente para este exemplo */

    /* Inserir associações (pré-definidas pela lógica do jogo) */
    inserirNaHash(ht, "Pegadas de lama", "Jardineiro");
    inserirNaHash(ht, "Gaveta perdida", "Jardineiro");
    inserirNaHash(ht, "Chave perdida", "Empregado");
    inserirNaHash(ht, "Lençol manchado", "Empregado");
    inserirNaHash(ht, "Livro com página faltando", "Bibliotecário");
    /* você pode adicionar mais pistas↔suspeitos aqui */

    /* BST de pistas coletadas começa vazia */
    PistaNode *pistasRoot = NULL;

    printf("=== Detective Quest: Modo Mestre ===\n");
    printf("Explore a mansão e colete pistas. No final, acuse um suspeito.\n");
    printf("Comandos: 'e' (esquerda), 'd' (direita), 's' (sair)\n");

    explorarSalas(hall, ht, &pistasRoot);

    verificarSuspeitoFinal(pistasRoot, ht);

    /* liberar toda memória usada */
    liberarPistas(pistasRoot);
    liberarHash(ht);
    liberarSalas(hall);

    printf("\nFim do jogo. Obrigado por jogar (console version).\n");
    return 0;
}
