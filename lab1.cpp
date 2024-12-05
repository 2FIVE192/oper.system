#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
// g++ -std=c++17 lab1.cpp -o lab1  ./lab1
using namespace std;
// Глобальные переменные
mutex mtx;                  // Мьютекс для синхронизации доступа
condition_variable cv;      // Условная переменная для ожидания события
bool ready = false;              // Флаг готовности события
int count = 0;                   // Счётчик итераций

// Функция потока-поставщика
void provider()
{
    while (count < 10)  // Ограничиваем выполнение 10 циклами
    {
        this_thread::sleep_for(chrono::seconds(1)); // Задержка 1 секунда
        {
            lock_guard<mutex> lock(mtx); // Защищаем доступ к разделяемым данным
            if (ready)
                return;  // Если событие уже передано, выходим из цикла
            ready = true;
            count++;  // Увеличиваем счётчик
            cout << "Event " << count << " provided" << endl;
        }
        cv.notify_one();  // Уведомление потока-потребителя о событии
    }
}

// Функция потока-потребителя
void consumer()
{
    while (count < 10)  // Ограничиваем выполнение 10 циклами
    {
        unique_lock<mutex> lock(mtx); // Блокируем мьютекс
        cv.wait(lock, []{ return ready; });  // Ожидаем, пока событие не будет готово
        cout << "Event " << count << " consumed" << std::endl;
        ready = false; // Сброс флага события
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
