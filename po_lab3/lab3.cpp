#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <condition_variable>
#include <random>

using namespace std;
mutex cout_mutex;

class Task 
{
    public:
        Task(int id, int duration) : id(id), duration(duration) {}
        Task() : id(0), duration(0) {} 
        
        void execute() const 
        {
            {
                lock_guard<mutex> lock(cout_mutex);
                cout << "[Task " << id << "] started, duration: " << duration << " sec\n";
            }
        
            this_thread::sleep_for(chrono::seconds(duration));
        
            {
                lock_guard<mutex> lock(cout_mutex);
                cout << "[Task " << id << "] finished\n";
            }
        }
        
    private:
        int id;
        int duration;
};


class TaskQueue 
{
public:
    
    bool empty() const 
    {
        unique_lock<mutex> lock(mtx);
        return queue.empty();
    }
    
    size_t size() const 
    {
        unique_lock<mutex> lock(mtx);
        return queue.size();
    }
    
    void clear() 
    {
        unique_lock<mutex> lock(mtx);
        while (!queue.empty()) 
            queue.pop();
    }
    
    bool push(const Task& task) 
    {
        unique_lock<mutex> lock(mtx);
        if (queue.size() >= 10) 
            return false; 

        queue.push(task); 
        cv.notify_one();  
        return true;  
    }
    
    bool pop(Task& task, atomic<bool>& paused, atomic<bool>& force_stop) 
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return !queue.empty() || terminated; });

        if (terminated || force_stop)
            return false;
        
        task = queue.front();
        queue.pop();
        return true;
    }
    
    void terminate() 
    {
        unique_lock<mutex> lock(mtx);
        terminated = true;
        cv.notify_all(); 
    }
    
    void notify() 
    {
        cv.notify_all();
    }

private:
    queue<Task> queue;
    mutable mutex mtx;
    condition_variable cv;
    bool terminated = false;
};

class ThreadPool 
{
    public:
        ThreadPool() 
        {
            queues.resize(2);
            for (int i = 0; i < 2; ++i) 
                queues[i] = new TaskQueue();
        }
        
        ~ThreadPool() 
        {
            for (auto queue : queues) 
                delete queue;
        } 
        
        
        void start() 
        {
            if (initialized || terminated) return;
            
            for (int i = 0; i < 4; ++i) 
                workers.emplace_back(&ThreadPool::workerRoutine, this, queues[i / 2]);
            
            initialized = true;
        }
        
        void addTask(const Task& task) 
        {
            if (!isWorking()) 
            return;
            
            size_t q1_size = queues[0]->size();
            size_t q2_size = queues[1]->size();
            
            bool added = false;
            if (q1_size <= q2_size) 
                added = queues[0]->push(task);
            else 
                added = queues[1]->push(task);
            
            if (!added) 
            {
                ++rejectedTasks;
                lock_guard<mutex> lock(cout_mutex);
                cout << "[Task] Rejected (queue is full)" << endl;
            }
        }
        
        void shutdown(bool force = false) 
        {
            terminated = true;
            force_stop = force;
            
            for (auto queue : queues) 
                queue->terminate();
            
            for (auto& worker : workers) 
            {
                if (worker.joinable()) 
                    worker.join();
            }
            
            workers.clear();
            initialized = false;
            terminated = false;
        }
        
        void pause() 
        {
            paused = true;
            lock_guard<mutex> lock(cout_mutex);
            cout << "ThreadPool paused.\n";
        }
    
        void resume() 
        {
            paused = false;
            for (auto& q : queues)
                q->notify();
    
            lock_guard<mutex> lock(cout_mutex);
            cout << "ThreadPool resumed.\n";
        }
    

        bool isWorking() const 
        {
            return initialized && !terminated;
        }
        
        int getRejectedTaskCount() const 
        {
            return rejectedTasks;
        }
        
    private:
        void workerRoutine(TaskQueue* queue) 
        {
            while (!terminated) 
            {         
                if (paused) 
                {
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }

                Task task;
                bool got_task = queue->pop(task, paused, force_stop);
                
                if (!got_task) 
                    break;
                
                task.execute();
            }
        }
        
        vector<TaskQueue*> queues;
        vector<thread> workers;
        bool initialized = false;
        bool terminated = false;
        atomic<bool> paused{false};
        atomic<bool> force_stop{false};
        atomic<int> rejectedTasks{0};
};
    

int random_duration(int min_sec = 4, int max_sec = 10) 
{
    return min_sec + rand() % (max_sec - min_sec + 1);
}

void generateTasks(ThreadPool& pool, atomic<int>& global_task_id, int thread_id) 
{
    for (int i = 0; i < 6; ++i) 
    {
        int task_id = global_task_id++;
        int duration = 4 + rand() % 7; 
        Task task(task_id, duration);
        pool.addTask(task);
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}


int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    ThreadPool pool;
    pool.start();
    atomic<int> global_task_id{1};
    
    thread t1(generateTasks, ref(pool), ref(global_task_id), 1);
    thread t2(generateTasks, ref(pool), ref(global_task_id), 2);
    thread t3(generateTasks, ref(pool), ref(global_task_id), 3);
    
    this_thread::sleep_for(chrono::seconds(3));
    pool.pause();
    this_thread::sleep_for(chrono::seconds(5));
    pool.resume();

    t1.join();
    t2.join();
    t3.join();
    
    this_thread::sleep_for(chrono::seconds(20));
    
    pool.shutdown(false);
    
    cout << "All tasks processed or rejected" << endl;
    cout << "Rejected tasks: " << pool.getRejectedTaskCount() << endl;


    return 0;
}