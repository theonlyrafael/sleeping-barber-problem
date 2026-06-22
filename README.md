# Problema do Barbeiro Dorminhoco

> Projeto inicialmente desenvolvido e finalizado em Março de 2024 para a matéria de Sistemas Operacionais.

## Definição do Problema

O Problema do Barbeiro Dorminhoco é um clássico problema de sincronização em programação concorrente, proposto por Edsger Dijkstra. A situação é a seguinte: uma barbearia possui um barbeiro, uma cadeira de atendimento e um número limitado de cadeiras na sala de espera. Quando não há clientes, o barbeiro dorme. Ao chegar um cliente, ele acorda o barbeiro caso ele esteja dormindo, ou aguarda em uma das cadeiras disponíveis. Se todas as cadeiras estiverem ocupadas, o cliente vai embora.

O desafio está em coordenar o acesso a esses recursos compartilhados sem gerar condições de corrida, deadlocks ou starvation.

---

## Conceitos

- **Thread**: unidade básica de execução concorrente dentro de um processo.
- **Multithreading**: técnica em que múltiplas threads executam de forma concorrente, compartilhando os recursos do mesmo processo.
- **Semáforo**: mecanismo de sincronização que controla o acesso a recursos compartilhados por meio de operações atômicas de incremento e decremento.
- **Mutex**: semáforo binário que garante exclusão mútua, ou seja, apenas uma thread por vez pode estar na seção crítica.

---

## Abordagem

A implementação simula um expediente de 20 segundos com 2 cadeiras na sala de espera (`CHAIRS = 2`). A cada ciclo, há 50% de chance de um novo cliente chegar.

Dois mecanismos de sincronização estruturam a lógica:

- **`barberReady`**: sinaliza ao barbeiro que um cliente está esperando. O barbeiro aguarda por no máximo 2 segundos antes de reavaliar o estado do expediente, evitando que ele trave indefinidamente quando não há movimento.
- **`accessWRSeats`**: mutex que protege as variáveis compartilhadas `waiting` e `barberSleeping`, prevenindo condições de corrida entre as threads.

O barbeiro encerra apenas após atender todos os clientes que já estão na fila. Novos clientes não são aceitos após os 20 segundos, mas os que aguardam nas cadeiras são atendidos até o fim.

---

## Os Arquivos

Este repositório contém duas implementações do mesmo problema, cada uma com uma abordagem diferente de concorrência.

### `barbeiro_dorminhoco.c` — POSIX

Utiliza as APIs do sistema operacional POSIX diretamente: `pthread.h` para threads e `semaphore.h` para semáforos. Por depender de chamadas do SO, sua portabilidade está atrelada ao ambiente de execução. Roda nativamente no Linux e em ambientes que emulam POSIX no Windows, como o MinGW-w64 presente no Code::Blocks.

### `barbeiro_dorminhoco.cpp` — Biblioteca Padrão do C++

Utiliza exclusivamente a biblioteca padrão do C++ (`<thread>`, `<mutex>`, `<condition_variable>`), disponível desde o C++11. O semáforo foi implementado como uma classe própria sobre essas primitivas, eliminando qualquer dependência de API do sistema operacional. O compilador de cada plataforma é responsável por traduzir essas chamadas para o que aquele SO entende: no Linux usa pthreads por baixo dos panos; no Windows usa as APIs nativas. O mesmo código roda em ambos sem nenhuma adaptação.

---

## Compatibilidade e Execução

| Arquivo | Linux | Windows |
|---|---|---|
| `barbeiro_dorminhoco.c` | ✅ Nativo | ⚠️ Requer MinGW-w64 (ex: Code::Blocks) |
| `barbeiro_dorminhoco.cpp` | ✅ | ✅ |

### C — Linux / ambiente POSIX

```bash
gcc -std=c11 -pthread barbeiro_dorminhoco.c -o barbeiro_dorminhoco
./barbeiro_dorminhoco
```

### C++ — Linux (GCC) ou Windows (MinGW-w64)

```bash
g++ -std=c++11 -pthread barbeiro_dorminhoco.cpp -o barbeiro_dorminhoco
./barbeiro_dorminhoco
```

### C++ — Windows (MSVC / VSCode)

```bash
cl /std:c++14 /EHsc barbeiro_dorminhoco.cpp
barbeiro_dorminhoco.exe
```

---

## Conclusão

Desenvolver este projeto me fez entender na prática o que antes eu tinha dificuldade de entender em aulas de laboratório. Desse modo, implementar o Problema do Barbeiro Dorminhoco exigiu que eu pensasse em como múltiplas threads competem por recursos compartilhados e o que acontece quando esse acesso não é coordenado corretamente.

A partir disso, aprendi o papel de cada primitiva de sincronização: o semáforo como forma de sinalização entre threads (o cliente avisando o barbeiro), e o mutex sendo responsável por variáveis compartilhadas, garantindo que apenas uma thread por vez possa lê-las ou modificá-las. Entendi também que erros de concorrência não são sempre óbvios, a primeira versão do código compilava e executava, mas o barbeiro travava na primeira janela sem clientes e nunca mais atendia ninguém, um bug silencioso que só aparece quando se acompanha o comportamento ao longo do tempo.

Por último, portando o projeto para C++, aprendi uma distinção importante: a diferença entre APIs do sistema operacional e APIs da linguagem. O código em C depende do POSIX, que é uma especificação do SO, enquanto o C++ me permitiu usar a biblioteca padrão da linguagem, que é portável por definição. Isso me mostrou que a escolha das ferramentas impacta diretamente onde o programa pode ou não rodar, e que nem sempre o problema está no código em si, mas na camada que ele acessa.