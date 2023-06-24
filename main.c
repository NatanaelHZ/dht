#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdbool.h>

// compilar: clear && gcc *.c -o exe -pthread && ./exe N
// Onde N ID , ID = 1 coordenador

typedef struct {
    int id;
    int port;
    char* ip;
} Processo;

typedef struct {
    int id;
    int inicio;
    int fim;
} TabelaRoteamento;

typedef struct {
    int rg;
    char nome[100];
} Registro;

typedef struct {
    int endereco;
    Registro registro;
} TabelaHash;

typedef struct {
    int rg;
    char operacao;  // 'C' para cadastrar, 'Q' para consultar
} Mensagem;

typedef struct {
    Mensagem mensagens[100];
    int tamanho;
} ListaMensagens;

ListaMensagens listaMensagens;

Processo processos[2] = {
    {1, 25001, "127.0.0.1"},
    {2, 25002, "127.0.0.1"}
};

TabelaRoteamento tabelaRoteamento[2] = {
                 // nodo    |  inicial       | final
    {1,  1, 10}, // N1      |    1           |     10
    {2, 11, 20}  // N2      |    11          |     20
};

TabelaHash tabelaHash[20]; // Tabela Hash distribuída
pthread_mutex_t tabelaHashMutex; // Mutex para proteger acesso à tabelaHash

int processoID;

// Função para gerar a chave hash
int gerarChaveHash(int rg) {
    int chave = rg % 20 + 1;

    printf("\n - chave: %i\n", chave);
    return chave;
}

// Função para buscar próxima posição livre na tabelaHash
int buscarProximaPosicaoLivre(int posicaoAtual) {
    int posicao = posicaoAtual;
    int inicio = tabelaRoteamento[processoID - 1].inicio - 1;
    int fim = tabelaRoteamento[processoID - 1].fim;

    do {
        posicao++;
        if (posicao > fim) {
            posicao = inicio;
        }
        if (tabelaHash[posicao - 1].registro.rg == -1) {
            return posicao-1;
        }
    } while (posicao != posicaoAtual);

    // Caso não encontre uma posição livre, retorne -1 ou faça outro tratamento adequado
    return -1;
}

// Função para cadastrar um registro na tabelaHash
void cadastrarRegistro(int rg, char* nome) {
    int chave = gerarChaveHash(rg);

    pthread_mutex_lock(&tabelaHashMutex);
    
    if (tabelaHash[chave - 1].registro.rg == -1) {
        tabelaHash[chave - 1].registro.rg = rg;
        strcpy(tabelaHash[chave - 1].registro.nome, nome);
        printf("\nRegistro cadastrado com sucesso.\n");
    } else {
        int posicaoLivre = buscarProximaPosicaoLivre(chave);
        if (posicaoLivre != -1) {
            tabelaHash[posicaoLivre].registro.rg = rg;
            strcpy(tabelaHash[posicaoLivre].registro.nome, nome);
            printf("\nRegistro cadastrado com sucesso.\n");
        } else {
            printf("\nNão foi possível cadastrar o registro. Tabela cheia.\n");
        }
    }

    pthread_mutex_unlock(&tabelaHashMutex);
}

// Função para consultar um registro na tabelaHash
void consultarRegistro(int rg) {
    int chave = gerarChaveHash(rg);

    pthread_mutex_lock(&tabelaHashMutex);

    if (tabelaHash[chave - 1].registro.rg == rg) {
        printf("\nRegistro encontrado:\n");
        printf("RG: %d\n", tabelaHash[chave - 1].registro.rg);
        printf("Nome: %s\n", tabelaHash[chave - 1].registro.nome);
        printf("\n--------------------------\n");
    } else {
        int fim = tabelaRoteamento[processoID - 1].fim;
        int posicao = chave;
        for (int i = chave; i <= fim; i++) {
            posicao = i;
            if (tabelaHash[posicao].registro.rg == rg) break;
        }

        if (posicao != -1 && tabelaHash[posicao].registro.rg == rg) {
            printf("\nRegistro encontrado:\n");
            printf("RG: %d\n", tabelaHash[posicao].registro.rg);
            printf("Nome: %s\n", tabelaHash[posicao].registro.nome);
            printf("\n--------------------------\n");
        } else {
            printf("Registro não encontrado.\n");
        }
    }

    pthread_mutex_unlock(&tabelaHashMutex);
}

void inicializarListaMensagens() {
    listaMensagens.tamanho = 0;
}

void inicializarTabelaHash() {
    for (int i = 0; i < 20; i++) {
        tabelaHash[i].registro.rg = -1;
    }
}

void adicionarMensagemLista(int rg, char operacao) {
    if (listaMensagens.tamanho < 100) {
        Mensagem mensagem;
        mensagem.rg = rg;
        mensagem.operacao = operacao;

        listaMensagens.mensagens[listaMensagens.tamanho++] = mensagem;

        printf("Registro adicionado à lista de mensagens.\n");
    } else {
        printf("Lista de mensagens está cheia. Não é possível adicionar mais registros.\n");
    }
}

void exibirMensagensLista() {
    printf("\nMensagens Pendentes:\n");
    for (int i = 0; i < listaMensagens.tamanho; i++) {
        printf("RG: %d, Operação: %c\n", listaMensagens.mensagens[i].rg, listaMensagens.mensagens[i].operacao);
    }
}

void limparListaMensagens() {
    listaMensagens.tamanho = 0;
}

// Função para processamento de mensagens recebidas
void* processamento(void* arg) {
    // Implemente o código para processar as mensagens recebidas
}

// Função para servidor receber mensagens
void* servidor(void* arg) {
    // Implemente o código para o servidor receber mensagens
}

// Função para cliente enviar mensagens
void* cliente(void* arg) {
    // Implemente o código para o cliente enviar mensagens
}

// Função para terminal exibir e solicitar dados de entrada do usuário
void* terminal(void* arg) {
    while (true) {
        printf("\n---------- MENU ----------\n");
        printf("1. Consultar registro por RG\n");
        printf("2. Cadastrar novo registro\n");
        printf("3. Exibir mensagens pendentes\n");
        printf("4. Registros locais\n");
        printf("5. Sair\n");
        printf("\n--------------------------\n");
        printf("Escolha uma opção: ");

        int opcao;
        scanf("%d", &opcao);

        if (opcao == 1) {
            int rg;
            printf("Digite o RG: ");
            scanf("%d", &rg);

            // Verificar se o RG pertence ao range de gerenciamento do processo atual
            int chave = gerarChaveHash(rg);
            bool rgLocal = false;
            if (chave >= tabelaRoteamento[processoID - 1].inicio && chave <= tabelaRoteamento[processoID - 1].fim) {
                rgLocal = true;
            }

            if (rgLocal) {
                consultarRegistro(rg);
            } else {
                adicionarMensagemLista(rg, 'Q');
            }
        } else if (opcao == 2) {
            int rg;
            char nome[100];

            printf("Digite o RG: ");
            scanf("%d", &rg);
            printf("Digite o nome: ");
            scanf("%s", nome);

            // Verificar se o RG pertence ao range de gerenciamento do processo atual
            bool rgLocal = false;
            int chave = gerarChaveHash(rg);
            if (chave >= tabelaRoteamento[processoID - 1].inicio && chave <= tabelaRoteamento[processoID - 1].fim) {
                rgLocal = true;
            }

            if (rgLocal) {
                cadastrarRegistro(rg, nome);
            } else {
                adicionarMensagemLista(rg, 'C');
            }
        } else if (opcao == 3) {
            exibirMensagensLista();
        } else if (opcao == 4) {
            int inicio = tabelaRoteamento[processoID -1].inicio - 1;
            int fim = tabelaRoteamento[processoID -1].fim;
            printf("\nTodos os registros locais:\n");
            for (int i = inicio; i <= fim; i++) {
                if (tabelaHash[i].registro.rg != -1)
                    printf("%i - Rg: %i Nome: %s\n", i, tabelaHash[i].registro.rg, tabelaHash[i].registro.nome);
            }
            printf("\n--------------------------\n");
        } else if (opcao == 5) {
            break;
        } else {
            printf("Opção inválida. Tente novamente.\n");
        }
    }

    limparListaMensagens();

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Use: %s <processoID>\n", argv[0]);
        return 1;
    }

    processoID = atoi(argv[1]);

    if (processoID < 1) {
        printf("ID do processo invalido...\n");
        return 1;
    }

    printf("Processo iniciado com ID: %d\n", processoID);

    // Inicialize outras variáveis e estruturas de dados necessárias
    inicializarListaMensagens();
    inicializarTabelaHash();

    // Crie as threads necessárias para o servidor, cliente, terminal e processamento
    pthread_t servidorThread, clienteThread, terminalThread;

    pthread_mutex_init(&tabelaHashMutex, NULL);

    // Crie a thread para o servidor
    if (pthread_create(&servidorThread, NULL, servidor, NULL) != 0) {
        fprintf(stderr, "Erro ao criar a thread do servidor.\n");
        return 1;
    }

    // Crie a thread para o cliente
    if (pthread_create(&clienteThread, NULL, cliente, NULL) != 0) {
        fprintf(stderr, "Erro ao criar a thread do cliente.\n");
        return 1;
    }

    // Crie a thread para o terminal
    if (pthread_create(&terminalThread, NULL, terminal, NULL) != 0) {
        fprintf(stderr, "Erro ao criar a thread do terminal.\n");
        return 1;
    }

    // Aguarde a finalização das threads
    pthread_join(servidorThread, NULL);
    pthread_join(clienteThread, NULL);
    pthread_join(terminalThread, NULL);

    pthread_mutex_destroy(&tabelaHashMutex);

    return 0;
}
