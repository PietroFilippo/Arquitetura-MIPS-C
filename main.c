#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

// Definições de constantes para o processador
#define TAMANHO_MEMORIA 1024    // Tamanho da memória
#define NUM_REGISTRADORES 32    // Número de registradores
#define TAMANHO_INSTRUCAO 4     // Bytes por instrução
#define MAX_INSTRUCAO_LEN 100   // Tamanho máximo de uma instrução

// Definição dos tipos de instruções
#define INSTRUCAO_R 0   // Instrução de registro
#define INSTRUCAO_I 1   // Instrução de valor imediato
#define INSTRUCAO_J 2   // Instrução de salto

// Definição do código de operação das instruções tipo R
#define OP_R    0x00 // Código para instruções de registro

// Definição dos códigos de operações do tipo I
#define OP_ADDI 0x08 // Código para a instrução addi
#define OP_LW   0x23 // Código para a instrução lw
#define OP_SW   0x2B // Código para a instrução sw
#define OP_BEQ  0x04 // Código para a instrução beq

// Definição dos códigos de operações do tipo J
#define OP_J    0x02 // Código para a instrução de salto
#define OP_JAL  0x03 // Código para a instrução de salto jal

// Definição dos códigos de função do tipo R
#define FUNC_ADD    0x20 // Código para a instrução add
#define FUNC_SUB    0x22 // Código para a instrução sub
#define FUNC_AND    0x24 // Código para a instrução and
#define FUNC_OR     0x25 // Código para a instrução or
#define FUNC_SLT    0x2A // Código para a instrução slt
#define FUNC_JR     0x08 // Código para a instrução jr

// Códigos de erro
#define SUCESSO 0
#define ERRO_PARAMETRO -1
#define ERRO_REGISTRADOR -2
#define ERRO_MEMORIA -3
#define ERRO_OVERFLOW -4

// Struct para representar uma instrução decodificada
typedef struct {
    int tipo;       // Tipo da instrução      
    int opcode;     // Código de operação da instrução
    int rs;         // Registrador fonte 1
    int rt;         // Registrador fonte 2 ou de destino
    int rd;         // Registrador de destino
    int shamt;      // Quantidade do shift
    int funcao;     // Código da função
    int imediato;   // Valor do imediato
    int endereco;   // Endereço do salto feito
} Instrucao;

// Struct do processador
typedef struct {
    int registradores[NUM_REGISTRADORES];  // Registradores
    int pc;                                // Contador de programa
    int memoria[TAMANHO_MEMORIA];          // Memória
} Processador;

// Protótipos das funções
void initProcessador(Processador *proc);
int decodeInstrucao(const char *assembly, Instrucao *inst);
int executeInstrucao(Processador *proc, Instrucao *inst);
void printRegistradores(const Processador *proc);
void printInstrucao(const Instrucao *inst);
int validaRegistrador(int reg);
int validaEndereco(int endereco);
void limparBuffer(void);
void printAjuda(void);
void printMemoria(const Processador *proc, int inicio, int fim);

//Inicializa o processador com valores padrão
void initProcessador(Processador *proc) {
    if (!proc) return;

    // Inicializa os registradores com 0
    memset(proc->registradores, 0, sizeof(proc->registradores));
    
    // Inicializa a memória com 0
    memset(proc->memoria, 0, sizeof(proc->memoria));
    
    // Inicializa o contador em 0
    proc->pc = 0;
}

//Valida se um número de registrador é válido
int validaRegistrador(int reg) {
    return (reg >= 0 && reg < NUM_REGISTRADORES) ? SUCESSO : ERRO_REGISTRADOR;
}

//Valida se um endereço de memória é válido
int validaEndereco(int endereco) {
    return (endereco >= 0 && endereco < TAMANHO_MEMORIA) ? SUCESSO : ERRO_MEMORIA;
}

// Funções auxiliares static para decodificação
static int decodeTipoR(const char *operandos, Instrucao *inst) {
    if (!operandos || !inst) return ERRO_PARAMETRO;

    char operandos_copia[MAX_INSTRUCAO_LEN];
    strncpy(operandos_copia, operandos, MAX_INSTRUCAO_LEN - 1);
    operandos_copia[MAX_INSTRUCAO_LEN - 1] = '\0';

    char *token = strtok(operandos_copia, ",");  // rd
    if(token && sscanf(token, " $%d", &inst->rd) == 1) {
        if (validaRegistrador(inst->rd) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    token = strtok(NULL, ",");  // rs
    if(token && sscanf(token, " $%d", &inst->rs) == 1) {
        if (validaRegistrador(inst->rs) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    token = strtok(NULL, ",");  // rt
    if(token && sscanf(token, " $%d", &inst->rt) == 1) {
        if (validaRegistrador(inst->rt) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    return SUCESSO;
}

static int decodeTipoI(const char *operandos, Instrucao *inst) {
    if (!operandos || !inst) return ERRO_PARAMETRO;

    char operandos_copia[MAX_INSTRUCAO_LEN];
    strncpy(operandos_copia, operandos, MAX_INSTRUCAO_LEN - 1);
    operandos_copia[MAX_INSTRUCAO_LEN - 1] = '\0';

    char *token = strtok(operandos_copia, ",");  // rt
    if(token && sscanf(token, " $%d", &inst->rt) == 1) {
        if (validaRegistrador(inst->rt) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    token = strtok(NULL, ",");  // rs
    if(token && sscanf(token, " $%d", &inst->rs) == 1) {
        if (validaRegistrador(inst->rs) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    token = strtok(NULL, ",");  // imediato
    if(token && sscanf(token, " %d", &inst->imediato) != 1) {
        return ERRO_PARAMETRO;
    }

    return SUCESSO;
}

static int decodeMemoria(const char *operandos, Instrucao *inst) {
    if (!operandos || !inst) return ERRO_PARAMETRO;

    char operandos_copia[MAX_INSTRUCAO_LEN];
    strncpy(operandos_copia, operandos, MAX_INSTRUCAO_LEN - 1);
    operandos_copia[MAX_INSTRUCAO_LEN - 1] = '\0';

    char *token = strtok(operandos_copia, ",");  // rt
    if(token && sscanf(token, " $%d", &inst->rt) == 1) {
        if (validaRegistrador(inst->rt) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    token = strtok(NULL, "(");  // imediato
    if(token && sscanf(token, " %d", &inst->imediato) != 1) {
        return ERRO_PARAMETRO;
    }

    token = strtok(NULL, ")");  // rs
    if(token && sscanf(token, "$%d", &inst->rs) == 1) {
        if (validaRegistrador(inst->rs) != SUCESSO) return ERRO_REGISTRADOR;
    } else return ERRO_PARAMETRO;

    return SUCESSO;
}

//Decodifica uma instrução em assembly MIPS
int decodeInstrucao(const char *assembly, Instrucao *inst) {
    if (!assembly || !inst) return ERRO_PARAMETRO;

    char operacao[10];
    char operandos[MAX_INSTRUCAO_LEN];
    int resultado;
    
    if (sscanf(assembly, "%s %[^\n]", operacao, operandos) != 2) {
        return ERRO_PARAMETRO;
    }

    memset(inst, 0, sizeof(Instrucao));

    if(strcmp(operacao, "add") == 0 || strcmp(operacao, "sub") == 0 || 
       strcmp(operacao, "and") == 0 || strcmp(operacao, "or") == 0 || 
       strcmp(operacao, "slt") == 0 || strcmp(operacao, "jr") == 0) {
        
        inst->tipo = INSTRUCAO_R;
        inst->opcode = OP_R;
        
        if(strcmp(operacao, "add") == 0) inst->funcao = FUNC_ADD;
        else if(strcmp(operacao, "sub") == 0) inst->funcao = FUNC_SUB;
        else if(strcmp(operacao, "and") == 0) inst->funcao = FUNC_AND;
        else if(strcmp(operacao, "or") == 0) inst->funcao = FUNC_OR;
        else if(strcmp(operacao, "slt") == 0) inst->funcao = FUNC_SLT;
        else if(strcmp(operacao, "jr") == 0) inst->funcao = FUNC_JR;


        // Para jr, precisamos de um tratamento especial pois ele só usa rs
        if(inst->funcao == FUNC_JR) {
            if(sscanf(operandos, " $%d", &inst->rs) != 1) {
                return ERRO_PARAMETRO;
            }
            if(validaRegistrador(inst->rs) != SUCESSO) {
                return ERRO_REGISTRADOR;
            }
            resultado = SUCESSO;
        } else {
            resultado = decodeTipoR(operandos, inst);
        }
    }   
    
    else if(strcmp(operacao, "addi") == 0 || strcmp(operacao, "beq") == 0) {
        inst->tipo = INSTRUCAO_I;
        inst->opcode = (strcmp(operacao, "addi") == 0) ? OP_ADDI : OP_BEQ;
        resultado = decodeTipoI(operandos, inst);
    }
    else if(strcmp(operacao, "lw") == 0 || strcmp(operacao, "sw") == 0) {
        inst->tipo = INSTRUCAO_I;
        inst->opcode = (strcmp(operacao, "lw") == 0) ? OP_LW : OP_SW;
        resultado = decodeMemoria(operandos, inst);
    }
    else if(strcmp(operacao, "j") == 0 || strcmp(operacao, "jal") == 0) {
        inst->tipo = INSTRUCAO_J;
        inst->opcode = (strcmp(operacao, "j") == 0) ? OP_J : OP_JAL;
        if(sscanf(operandos, " %d", &inst->endereco) != 1) {
            resultado = ERRO_PARAMETRO;
        } else {
            resultado = SUCESSO;
        }
    }
    else {
        resultado = ERRO_PARAMETRO;
    }

    return resultado;
}

//Executa uma instrução MIPS no processador
int executeInstrucao(Processador *proc, Instrucao *inst) {
    if (!proc || !inst) return ERRO_PARAMETRO;

    // Garantir que o registrador $0 sempre seja 0
    proc->registradores[0] = 0;

    switch(inst->tipo) {
        case INSTRUCAO_R:
            switch(inst->funcao) {
                case FUNC_ADD:
                    // Verificar overflow na adição
                    if ((proc->registradores[inst->rs] > 0 && proc->registradores[inst->rt] > INT_MAX - proc->registradores[inst->rs]) ||
                        (proc->registradores[inst->rs] < 0 && proc->registradores[inst->rt] < INT_MIN - proc->registradores[inst->rs])) {
                        printf("Aviso: overflow na operacao ADD\n");
                        return ERRO_OVERFLOW;
                    }
                    proc->registradores[inst->rd] = proc->registradores[inst->rs] + proc->registradores[inst->rt];
                    break;
                case FUNC_SUB:
                    // Verificar overflow na subtração
                    if ((proc->registradores[inst->rs] > 0 && proc->registradores[inst->rt] < INT_MIN + proc->registradores[inst->rs]) ||
                        (proc->registradores[inst->rs] < 0 && proc->registradores[inst->rt] > INT_MAX + proc->registradores[inst->rs])) {
                        printf("Aviso: overflow na operacao SUB\n");
                        return ERRO_OVERFLOW;
                    }
                    proc->registradores[inst->rd] = proc->registradores[inst->rs] - proc->registradores[inst->rt];
                    break;
                case FUNC_AND:
                    proc->registradores[inst->rd] = proc->registradores[inst->rs] & proc->registradores[inst->rt];
                    break;
                case FUNC_OR:
                    proc->registradores[inst->rd] = proc->registradores[inst->rs] | proc->registradores[inst->rt];
                    break;
                case FUNC_SLT:
                    proc->registradores[inst->rd] = (proc->registradores[inst->rs] < proc->registradores[inst->rt]) ? 1 : 0;
                    break;
                case FUNC_JR:
                    {
                        int novo_pc = proc->registradores[inst->rs];
                        if (novo_pc < 0 || novo_pc >= TAMANHO_MEMORIA * 4) {
                            printf("Erro: salto para endereco invalido\n");
                            return ERRO_MEMORIA;
                        }
                        proc->pc = novo_pc;
                        return SUCESSO;
                    }
                    break;
            }
            proc->pc += TAMANHO_INSTRUCAO;
            break;

        case INSTRUCAO_I:
            switch(inst->opcode) {
                case OP_ADDI:
                    // Verificar overflow na adição imediata
                    if ((proc->registradores[inst->rs] > 0 && inst->imediato > INT_MAX - proc->registradores[inst->rs]) ||
                        (proc->registradores[inst->rs] < 0 && inst->imediato < INT_MIN - proc->registradores[inst->rs])) {
                        printf("Aviso: overflow na operacao ADDI\n");
                        return ERRO_OVERFLOW;
                    }
                    proc->registradores[inst->rt] = proc->registradores[inst->rs] + inst->imediato;
                    proc->pc += TAMANHO_INSTRUCAO;
                    break;
                case OP_LW:
                    {
                        int endereco = proc->registradores[inst->rs] + inst->imediato;
                        if (validaEndereco(endereco) != SUCESSO) {
                            printf("Erro: acesso invalido a memoria\n");
                            return ERRO_MEMORIA;
                        }
                        proc->registradores[inst->rt] = proc->memoria[endereco];
                        proc->pc += TAMANHO_INSTRUCAO;
                    }
                    break;
                    
                case OP_SW:
                    {
                        int endereco = proc->registradores[inst->rs] + inst->imediato;
                        if (validaEndereco(endereco) != SUCESSO) {
                            printf("Erro: acesso invalido a memoria\n");
                            return ERRO_MEMORIA;
                        }
                        proc->memoria[endereco] = proc->registradores[inst->rt];
                        proc->pc += TAMANHO_INSTRUCAO;
                    }
                    break;
                case OP_BEQ:
                    if(proc->registradores[inst->rs] == proc->registradores[inst->rt]) {
                        // Verificar se o salto está dentro dos limites válidos
                        int novo_pc = proc->pc + (inst->imediato * TAMANHO_INSTRUCAO);
                        if (novo_pc < 0 || novo_pc >= TAMANHO_MEMORIA * 4) {
                            printf("Erro: salto para endereco invalido\n");
                            return ERRO_MEMORIA;
                        }
                        proc->pc = novo_pc;
                    } else {
                        proc->pc += TAMANHO_INSTRUCAO;
                    }
                    break;
            }
            break;

        case INSTRUCAO_J:
            {
                int novo_pc = inst->endereco * TAMANHO_INSTRUCAO;
                if (novo_pc < 0 || novo_pc >= TAMANHO_MEMORIA * 4) {
                    printf("Erro: salto para endereco inválido\n");
                    return ERRO_MEMORIA;
                }
                if(inst->opcode == OP_J) {
                    proc->pc = novo_pc;
                } else if(inst->opcode == OP_JAL) {
                    proc->registradores[31] = proc->pc + TAMANHO_INSTRUCAO;
                    proc->pc = novo_pc;
                }
            }
            break;
    }

    // Garantir que o registrador $0 permaneça zero após a execução
    proc->registradores[0] = 0;
    return SUCESSO;
}

//Imprime os valores dos registradores e PC
void printRegistradores(const Processador *proc) {
    if (!proc) return;

    printf("\nRegistradores:\n");
    for(int i = 0; i < NUM_REGISTRADORES; i++) {
        if(i % 4 == 0) printf("\n");
        printf("$%-2d: %-8d  ", i, proc->registradores[i]);
    }
    printf("\nPC: %d\n", proc->pc);
}

//Imprime os detalhes da instrução decodificada
void printInstrucao(const Instrucao *inst) {
    if (!inst) return;

    printf("\nInstrucao:\n");
    switch(inst->tipo) {
        case INSTRUCAO_R:
            printf("Tipo R:\n");
            printf("Opcode: %d (0x%02X)\n", inst->opcode, inst->opcode);
            printf("rs: $%d\n", inst->rs);
            printf("rt: $%d\n", inst->rt);
            printf("rd: $%d\n", inst->rd);
            printf("shamt: %d\n", inst->shamt);
            printf("Funcao: %d (0x%02X)\n", inst->funcao, inst->funcao);
            break;

        case INSTRUCAO_I:
            printf("Tipo I:\n");
            printf("Opcode: %d (0x%02X)\n", inst->opcode, inst->opcode);
            printf("rs: $%d\n", inst->rs);
            printf("rt: $%d\n", inst->rt);
            printf("Imediato: %d\n", inst->imediato);
            break;

        case INSTRUCAO_J:
            printf("Tipo J:\n");
            printf("Opcode: %d (0x%02X)\n", inst->opcode, inst->opcode);
            printf("Endereco: %d\n", inst->endereco);
            break;
    }
}

// Função de ajuda
void printAjuda() {
    printf("\nInstrucoes disponiveis:\n");
    printf("Tipo R: add, sub, and, or, slt\n");
    printf("Formato: <instrucao> $rd,$rs,$rt\n");
    printf("Exemplo: add $1,$2,$3\n\n");
    printf("Exemplo: sub $1,$2,$3\n\n");
    printf("Exemplo: jr $31\n\n");
    
    printf("Tipo I: addi, lw, sw, beq\n");
    printf("Formato addi: addi $rt,$rs,imediato\n");
    printf("Formato lw/sw: lw/sw $rt,offset($rs)\n");
    printf("Formato beq: beq $rs,$rt,offset\n");
    printf("Exemplo: addi $1,$2,100\n");
    printf("Exemplo: lw $1,0($2)\n");
    printf("Exemplo: beq $1,$2,10\n\n");
    
    printf("Tipo J: j, jal\n");
    printf("Formato: <instrucao> endereco\n");
    printf("Exemplo: j 100\n\n");
    
    printf("Comandos especiais:\n");
    printf("help - Mostra esta ajuda\n");
    printf("mem <inicio> <fim> - Mostra conteudo da memoria\n");
    printf("sair - Encerra o programa\n");
}

// Função para visualizar a memória
void printMemoria(const Processador *proc, int inicio, int fim) {
    if (!proc || inicio < 0 || fim >= TAMANHO_MEMORIA || inicio > fim) {
        printf("Erro: parametros invalidos para exibicao da memoria\n");
        return;
    }

    printf("\nMemoria [%d-%d]:\n", inicio, fim);
    for(int i = inicio; i <= fim; i++) {
        if(i % 4 == 0) printf("\n");
        printf("[%3d]: %-8d  ", i, proc->memoria[i]);
    }
    printf("\n");
}

// Função para limpar buffer de entrada
void limparBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    Processador proc;
    initProcessador(&proc);

    char buffer[MAX_INSTRUCAO_LEN];
    
    printf("Simulador MIPS\n");
    printf("Digite 'help' para ver as instrucoes disponiveis\n");
    printf("mem <inicio> <fim> - Mostra conteudo da memoria\n");
    printf("Digite 'sair' para encerrar o programa\n\n");

    while (1) {
        printf("Digite a instrucao MIPS: ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Erro na leitura da instrucao\n");
            continue;
        }
        
        // Remove nova linha do buffer
        buffer[strcspn(buffer, "\n")] = 0;

        // Verifica se o usuário deseja sair
        if (strcmp(buffer, "sair") == 0) {
            break;
        }

        // Verifica se o usuário pediu ajuda
        if (strcmp(buffer, "help") == 0) {
            printAjuda();
            continue;
        }

        // Verifica se o usuário quer ver a memória
        if (strncmp(buffer, "mem ", 4) == 0) {
            int inicio, fim;
            if (sscanf(buffer + 4, "%d %d", &inicio, &fim) == 2) {
                printMemoria(&proc, inicio, fim);
            } else {
                printf("Uso correto: mem <inicio> <fim>\n");
            }
            continue;
        }

        // Verifica o tamanho da instrução
        if (strlen(buffer) >= MAX_INSTRUCAO_LEN - 1) {
            printf("Erro: instrução muito longa\n");
            limparBuffer();
            continue;
        }

        Instrucao inst;
        int resultado = decodeInstrucao(buffer, &inst);
        
        if (resultado != SUCESSO) {
            printf("Erro ao decodificar a instrucao (codigo: %d)\n", resultado);
            continue;
        }

        resultado = executeInstrucao(&proc, &inst);
        
        if (resultado != SUCESSO && resultado != ERRO_OVERFLOW) {
            printf("Erro ao executar a instrucao (codigo: %d)\n", resultado);
            continue;
        }
        
        printInstrucao(&inst);
        printRegistradores(&proc);
    }

    return 0;
}