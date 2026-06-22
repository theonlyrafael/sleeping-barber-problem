#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <atomic>

constexpr int CHAIRS = 2; // número de cadeiras na sala de espera

// semáforo contador implementado com mutex e condition_variable 
class Semaphore
{
public:
    explicit Semaphore(int initialCount = 0) : count(initialCount) {}

    void release()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    template <typename Rep, typename Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period> &timeout)
    {
        std::unique_lock<std::mutex> lock(mtx);
        bool acquired = cv.wait_for(lock, timeout, [this]
                                    { return count > 0; });
        if (acquired)
            count--;
        return acquired;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

Semaphore barberReady(0);        // semáforo que avisa o barbeiro que há um cliente
std::mutex accessWRSeats;        // mutex que protege as variáveis compartilhadas
int waiting = 0;                 // clientes esperando (não conta quem já está sendo atendido)
std::atomic<bool> working(true); // indica se o expediente ainda está aberto
bool barberSleeping = false;     // indica se o barbeiro está "dormindo"

void barber()
{
    while (true)
    {
        bool shouldExit = false;
        {
            std::lock_guard<std::mutex> lock(accessWRSeats);
            if (waiting == 0)
            {
                if (!working)
                {
                    // não há clientes e o expediente já encerrou: o barbeiro pode ir embora
                    shouldExit = true;
                }
                else
                {
                    std::cout << "Barbeiro esta dormindo...\n";
                    barberSleeping = true;
                }
            }
        }

        if (shouldExit)
            break;

        if (!barberReady.try_acquire_for(std::chrono::seconds(2)))
        {
            // se ninguém chegou nesses 2s, volta ao topo do loop e reavalia 'working'/'waiting'
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(accessWRSeats);
            waiting--;
            if (waiting < 0)
                waiting = 0;
            std::cout << "Barbeiro esta cortando o cabelo do cliente. "
                      << waiting << " cliente(s) esperando\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); // simula o tempo do corte
        std::cout << "Barbeiro terminou de cortar o cabelo do cliente.\n";
    }

    std::cout << "Barbeiro foi para casa.\n";
}

void customer()
{
    std::lock_guard<std::mutex> lock(accessWRSeats);
    if (waiting < CHAIRS)
    {
        waiting++;
        if (barberSleeping)
        {
            std::cout << "Cliente chegou e acordou o barbeiro. "
                      << waiting << " cliente(s) esperando.\n";
            barberSleeping = false;
        }
        else
        {
            std::cout << "Cliente chegou. " << waiting << " cliente(s) esperando.\n";
        }
        barberReady.release(); // avisa o barbeiro
    }
    else
    {
        std::cout << "Sala de espera cheia. Cliente foi embora.\n";
    }
}

int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::thread barberThread(barber);

    auto startTime = std::chrono::steady_clock::now();
    while (working)
    {
        if (std::rand() % 2 == 0)
        {
            std::thread customerThread(customer);
            customerThread.detach(); // a thread encerra sozinha
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        auto now = std::chrono::steady_clock::now();
        double secondsPassed = std::chrono::duration<double>(now - startTime).count();
        if (secondsPassed > 20.0)
        {
            working = false; // encerra o expediente (o barbeiro ainda atende quem já está esperando)
        }
    }

    barberThread.join(); // espera o barbeiro terminar de atender a fila e ir embora

    std::cout << "Expediente acabou.\n";

    return 0;
}