#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;

// Глобальные переменные
mutex mtx;                  // Мьютекс для синхронизации доступа
condition_variable cv;      // Условная переменная для ожидания события
bool ready = false;         // Флаг готовности события
int count = 0;              // Счётчик итераций
const int max_count = 10;   // Максимальное количество итераций

// Функция потока-поставщика
void provider()
{
    while (true)
    {
        {
            unique_lock<mutex> lock(mtx);
            if (count >= max_count) // Прекращаем выполнение при достижении лимита
                break;

            if (ready) // Если событие еще не обработано, пропускаем
                continue;

            ready = true;        // Устанавливаем флаг события
            count++;             // Увеличиваем счётчик
            cout << "Event " << count << " provided" << endl;
        }
        cv.notify_one();          // Уведомляем поток-потребителя
        this_thread::sleep_for(chrono::seconds(1)); // Задержка
    }
}

// Функция потока-потребителя
void consumer()
{
    while (true)
    {
        unique_lock<mutex> lock(mtx);
        if (count >= max_count && !ready) // Проверяем условие завершения
            break;

        if (!ready) 
        {
            cv.wait(lock);
            continue;
        }

        // Обрабатываем событие
        cout << "Event " << count << " consumed" << endl;
        ready = false; // Сбрасываем флаг
    }
}

int main()
{
    // Создаем и запускаем потоки
    thread t1(provider);
    thread t2(consumer);

    // Ожидаем завершения потоков
    t1.join();
    t2.join();

    return 0;
}
