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
#include <ctype.h>

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
    char nome[100];
    int enviado;
    int processoID;
} Saida;

typedef struct {
    Saida mensagens[100];
    int tamanho;
} ListaSaida;

ListaSaida listaSaida;
pthread_mutex_t listaSaidaMutex;

typedef struct {
    int rg;
    int exibido;
    char operacao;  // 'C' para cadastrar, 'Q' para consultar
    char nome[100];
} Mensagem;

typedef struct {
    Mensagem mensagens[100];
    int tamanho;
} ListaMensagens;

ListaMensagens listaMensagens;
pthread_mutex_t listaMensagensMutex;

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

    return chave;
}

int idProcessoRecebedor() {
    return processoID == 1 ? 2 : 1;
}

// Função para buscar próxima posição livre na tabelaHash
int buscarProximaPosicaoLivre(int posicaoAtual) {
    int posicao = posicaoAtual;
    int inicio = tabelaRoteamento[processoID - 1].inicio - 1;
    int fim = tabelaRoteamento[processoID - 1].fim;

    for (int i = inicio; i <= fim; i++) {
        if (tabelaHash[posicao].registro.rg == -1) {
            return posicao;
        }
    }

    // Caso não encontre uma posição livre
    return -1;
}

// Função adiciona a lista de saida para envio na thread cliente
void adicionarSaidaLista(int rg, char operacao, char* nome, int processoID) {
    pthread_mutex_lock(&listaSaidaMutex);

    if (listaSaida.tamanho < 100) {
        Saida saida;
        saida.rg = rg;
        saida.operacao = operacao;
        strcpy(saida.nome, nome);
        saida.enviado = 0;
        saida.processoID = processoID;

        listaSaida.mensagens[listaSaida.tamanho++] = saida;
    } else {
        printf("Lista de saída está cheia. Não é possível adicionar mais mensagens.\n");
    }

    pthread_mutex_unlock(&listaSaidaMutex);
}

// Função para cadastrar um registro na tabelaHash
void cadastrarRegistro(int rg, char* nome) {
    int chave = gerarChaveHash(rg);
    int inicio = tabelaRoteamento[processoID - 1].inicio;

    pthread_mutex_lock(&tabelaHashMutex);
    
    if (tabelaHash[chave - 1].registro.rg == -1 && chave > inicio) {
        tabelaHash[chave - 1].registro.rg = rg;
        strcpy(tabelaHash[chave - 1].registro.nome, nome);
        // printf("\n*Registro cadastrado com sucesso.\n");
    } else {
        int posicaoLivre = buscarProximaPosicaoLivre(chave);
        if (posicaoLivre != -1) {
            tabelaHash[posicaoLivre].registro.rg = rg;
            strcpy(tabelaHash[posicaoLivre].registro.nome, nome);
            // printf("\nRegistro cadastrado com sucesso.\n");
        } else {
            // printf("\nNão foi possível cadastrar o registro. Tabela cheia.\n");
            adicionarSaidaLista(rg, 'C', nome, idProcessoRecebedor());
        }
    }

    pthread_mutex_unlock(&tabelaHashMutex);
}

// Função para consultar um registro na tabelaHash
void consultarRegistro(int rg) {
    int chave = gerarChaveHash(rg);

    pthread_mutex_lock(&tabelaHashMutex);

    if (tabelaHash[chave - 1].registro.rg == rg) {
        printf("\n*Registro encontrado:\n");
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
    pthread_mutex_init(&listaMensagensMutex, NULL);
}

void inicializarTabelaHash() {
    for (int i = 0; i < 20; i++) {
        tabelaHash[i].registro.rg = -1;
    }
}

void inicializarListaSaida() {
    listaSaida.tamanho = 0;
    pthread_mutex_init(&listaSaidaMutex, NULL);

    for (int i = 0; i < 100; i++) {
        listaSaida.mensagens[i].enviado = -1;
    }
}

void adicionarMensagemLista(int rg, char operacao, char* nome) {
    pthread_mutex_lock(&listaMensagensMutex);

    if (listaMensagens.tamanho < 100) {
        Mensagem mensagem;
        mensagem.rg = rg;
        mensagem.operacao = operacao;
        mensagem.exibido = 0;
        strcpy(mensagem.nome, nome);

        listaMensagens.mensagens[listaMensagens.tamanho++] = mensagem;
    } else {
        printf("Lista de mensagens está cheia. Não é possível adicionar mais mensagens.\n");
    }

    pthread_mutex_unlock(&listaMensagensMutex);
}

void exibirMensagensLista() {
    pthread_mutex_lock(&listaMensagensMutex);

    printf("\nMensagens Pendentes:\n");
    for (int i = 0; i < listaMensagens.tamanho; i++) {
        if (listaMensagens.mensagens[i].exibido == 0) {
            printf("RG: %d - Nome: %s\n", listaMensagens.mensagens[i].rg, listaMensagens.mensagens[i].nome);
            listaMensagens.mensagens[i].exibido = 1;
        }
    }

    pthread_mutex_unlock(&listaMensagensMutex);
}

void limparListaMensagens() {
    pthread_mutex_lock(&listaMensagensMutex);

    listaMensagens.tamanho = 0;

    pthread_mutex_unlock(&listaMensagensMutex);
}

// Função para consultar um registro na tabelaHash
void encontrarRegistroParaSaida(int rg, int processoIdAux) {
    int chave = gerarChaveHash(rg);

    pthread_mutex_lock(&tabelaHashMutex);

    if (tabelaHash[chave - 1].registro.rg == rg) {
        adicionarSaidaLista(rg, 'Q', tabelaHash[chave - 1].registro.nome, processoIdAux);
    } else {
        int fim = tabelaRoteamento[processoID - 1].fim;
        int posicao = chave;
        for (int i = chave; i <= fim; i++) {
            posicao = i;
            if (tabelaHash[posicao].registro.rg == rg) break;
        }

        if (posicao != -1 && tabelaHash[posicao].registro.rg == rg) {
            adicionarSaidaLista(rg, 'Q', tabelaHash[posicao].registro.nome, processoIdAux);
        } else {
            printf("Registro não encontrado.\n");
        }
    }

    pthread_mutex_unlock(&tabelaHashMutex);
}

// Função para servidor receber mensagens
void* servidor(void* arg) {
    int socketID;
    int porta = processos[processoID - 1].port;

    struct sockaddr_in endereco;
    endereco.sin_family = AF_INET;
    endereco.sin_port = htons(porta);
    endereco.sin_addr.s_addr = INADDR_ANY;

    socketID = socket(AF_INET, SOCK_DGRAM, 0);

    if (bind(socketID, (struct sockaddr*)&endereco, sizeof(endereco)) == -1) {
        printf("Erro ao vincular o socket à porta.\n");
        exit(1);
    }

    struct sockaddr_in enderecoRemoto;
    int tamanhoEnderecoRemoto = sizeof(enderecoRemoto);
    char buffer[256];

    while (true) {
        int bytesRecebidos = recvfrom(socketID, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&enderecoRemoto, &tamanhoEnderecoRemoto);
        if (bytesRecebidos == -1) {
            printf("Erro ao receber a mensagem.\n");
            continue;
        }

        buffer[bytesRecebidos] = '\0';

        // Verificar se a mensagem é uma consulta
        if (buffer[0] == 'Q' && bytesRecebidos > 3) {
            int rg;
            char nome[100];
            char operacao;

            // Extrair as informações da mensagem
            sscanf(buffer, "%c|%d|%99[^\n]", &operacao, &rg, nome);

            if (isdigit(nome[0])) {
                int processoIDAux = atoi(nome);
                //adicionarSaidaLista(rg, 'Q', nome, processoIDAux);
                encontrarRegistroParaSaida(rg, processoIDAux);
            } else {
                // Adicionar a mensagem na lista de mensagens
                adicionarMensagemLista(rg, 'Q', nome);
            }
        }
        else {
            int rg;
            char nome[100];
            char operacao;

            // Extrair as informações da mensagem
            sscanf(buffer, "%c|%d|%99[^\n]", &operacao, &rg, nome);

            cadastrarRegistro(rg, nome);
        }
    }

    close(socketID);

    return NULL;
}

// Função para cliente enviar mensagens
void* cliente(void* arg) {
    int socketID;
    int porta = processos[processoID - 1].port;

    while (true) {
        pthread_mutex_lock(&listaSaidaMutex);

        // Verificar se há mensagens pendentes na lista de saída
        for (int i = 0; i < 100; i++) {
            if (listaSaida.mensagens[i].enviado == 0) {
                // Configuração
                char* ipDestino = processos[listaSaida.mensagens[i].processoID - 1].ip;
                int portaDestino = processos[listaSaida.mensagens[i].processoID - 1].port;

                struct sockaddr_in enderecoDestino;
                enderecoDestino.sin_family = AF_INET;
                enderecoDestino.sin_port = htons(portaDestino);
                inet_aton(ipDestino, &enderecoDestino.sin_addr);

                socketID = socket(AF_INET, SOCK_DGRAM, 0);
                // Fim configuração

                Mensagem mensagemEnviar;
                mensagemEnviar.rg = listaSaida.mensagens[i].rg;
                mensagemEnviar.operacao = listaSaida.mensagens[i].operacao;
                strcpy(mensagemEnviar.nome, listaSaida.mensagens[i].nome);

                char buffer[256];
                sprintf(buffer, "%c|%d|%s", mensagemEnviar.operacao, mensagemEnviar.rg, mensagemEnviar.nome);

                int bytesEnviados = sendto(socketID, buffer, strlen(buffer), 0, (struct sockaddr*)&enderecoDestino, sizeof(enderecoDestino));
                if (bytesEnviados == -1) {
                    printf("Erro ao enviar a mensagem.\n");
                } else {
                    listaSaida.mensagens[i].enviado = 1;
                }
            }
        }

        pthread_mutex_unlock(&listaSaidaMutex);

        sleep(5);
    }

    close(socketID);

    return NULL;
}

// Função para terminal exibir e solicitar dados de entrada do usuário
void* terminal(void* arg) {
    while (true) {
        printf("\n---------- MENU ----------\n");
        printf("1. Consultar registro por RG\n");
        printf("2. Cadastrar novo registro\n");
        printf("3. Exibir pendentes consulta\n");
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
                int processo = idProcessoRecebedor();
                char nome[100];
                sprintf(nome, "%d", processoID);
                //adicionarMensagemLista(rg, 'Q');
                // ***Adicionar na fila de saida para ser enviada pela threado cliente
                adicionarSaidaLista(rg, 'Q', nome, processo);
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
                int processo = idProcessoRecebedor();
                //adicionarMensagemLista(rg, 'C');
                // ***Adicionar na fila de saida para ser enviada pela threado cliente
                adicionarSaidaLista(rg, 'C', nome, processo);
            }
        } else if (opcao == 3) {
            exibirMensagensLista();
        } else if (opcao == 4) {
            int inicio = tabelaRoteamento[processoID -1].inicio - 1;
            int fim = tabelaRoteamento[processoID -1].fim;
            printf("\nTodos os registros locais:\n");
            for (int i = inicio; i < fim; i++) {
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

    inicializarListaSaida();
    inicializarListaMensagens();
    inicializarTabelaHash();

    pthread_t servidorThread, clienteThread, terminalThread;

    pthread_mutex_init(&tabelaHashMutex, NULL);

    if (pthread_create(&servidorThread, NULL, servidor, NULL) != 0) {
        fprintf(stderr, "Erro ao criar a thread do servidor.\n");
        return 1;
    }

    if (pthread_create(&clienteThread, NULL, cliente, NULL) != 0) {
        fprintf(stderr, "Erro ao criar a thread do cliente.\n");
        return 1;
    }

    if (pthread_create(&terminalThread, NULL, terminal, NULL) != 0) {
        fprintf(stderr, "Erro ao criar a thread do terminal.\n");
        return 1;
    }

    pthread_join(servidorThread, NULL);
    pthread_join(clienteThread, NULL);
    pthread_join(terminalThread, NULL);

    pthread_mutex_destroy(&tabelaHashMutex);

    return 0;
}
