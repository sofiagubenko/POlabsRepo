#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <condition_variable>
#include <random>
#include <algorithm>


using namespace std;
mutex cout_mutex;

class Task 
{
    public:
        Task(int id, int duration) : id(id), duration(duration) {}
        Task() : id(0), duration(0) {} 

        void markEnqueued() 
        {
            enqueueTime = chrono::steady_clock::now();
        }
        
        long long getWaitTime() const 
        {
            auto now = chrono::steady_clock::now();
            return chrono::duration_cast<chrono::milliseconds>(now - enqueueTime).count();
        }
        
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
        chrono::steady_clock::time_point enqueueTime;
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
    
    bool push(Task task) 
    {
        unique_lock<mutex> lock(mtx);
        if (queue.size() >= 10) 
            return false; 
            
        task.markEnqueued();
        queue.push(task); 

        if (queue.size() == 10 && !full_flag) 
        {
            full_time_start = chrono::steady_clock::now();
            full_flag = true;
        }

        cv.notify_one();  
        return true;  
    }
    
    bool pop(Task& task, atomic<bool>& force_stop, const atomic<bool>& paused) 
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, &force_stop, &paused]() 
        { 
            return (!queue.empty() && !paused) || terminated || force_stop; 
        });

        if (queue.empty() && (terminated || force_stop))
            return false;
        
        task = queue.front();
        queue.pop();

        if (queue.size() < 10 && full_flag) 
        {
            auto full_time_end = chrono::steady_clock::now();
            full_durations.push_back(chrono::duration_cast<chrono::milliseconds>(full_time_end - full_time_start).count());
            full_flag = false;
        }

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

    vector<long long> get_full_times() const 
    {
        return full_durations;
    }


private:
    queue<Task> queue;
    mutable mutex mtx;
    condition_variable cv;
    bool terminated = false;

    chrono::steady_clock::time_point full_time_start;
    bool full_flag = false;
    vector<long long> full_durations;
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

        int getTasksExecuted() const 
        {
            return tasks_executed.load();
        }
        
        double getAverageWaitTime() const 
        {
            if (tasks_executed == 0) 
                return 0;
            return static_cast<double>(total_wait_time.load()) / tasks_executed;
        }

        double getAverageIdleTime() const 
        {
            if (idle_count == 0) 
                return 0;
            return static_cast<double>(total_worker_idle_time.load()) / idle_count;
        }
        
    
        TaskQueue* getQueue(int index) const 
        {
            return queues[index];
        }
        
    private:
        void workerRoutine(TaskQueue* queue) 
        {
            
            while (!terminated) 
            {     
                {
                    unique_lock<mutex> pause_lock(pause_mutex);    
                    if (paused) 
                    {
                        pause_cv.wait(pause_lock, [this]() { return !paused || terminated; });
                    }
                }

                if (terminated) 
                    break;

                Task task;

                auto wait_start = chrono::steady_clock::now();
                bool got_task = queue->pop(task, force_stop);
                auto wait_end = chrono::steady_clock::now();

                long long idle_time = chrono::duration_cast<chrono::milliseconds>(wait_end - wait_start).count();
                total_worker_idle_time += idle_time;
                idle_count++;
                
                if (!got_task) 
                    break;
                    
                total_wait_time += task.getWaitTime();
                tasks_executed++;
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
        atomic<long long> total_wait_time{0};
        atomic<int> tasks_executed{0};
        atomic<long long> total_worker_idle_time{0};  
        atomic<int> idle_count{0};    
        condition_variable pause_cv;
        mutex pause_mutex;
};
    

void generateTasks(ThreadPool& pool, atomic<int>& global_task_id, int task_limit) 
{
    while(true)
    {
        int task_id = global_task_id.fetch_add(1);
        if (task_id > task_limit)
            break;

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

    int task_limit = 50;        
    int num_generators = 2; 

    vector<thread> generators;
    for (int i = 0; i < num_generators; ++i)
    {
        generators.emplace_back(generateTasks, ref(pool), ref(global_task_id), task_limit);
    }
    
    this_thread::sleep_for(chrono::seconds(3));
    pool.pause();
    this_thread::sleep_for(chrono::seconds(5));
    pool.resume();

    for (auto& t : generators)
        t.join();
    
    this_thread::sleep_for(chrono::seconds(20));
    
    pool.shutdown(false);

    cout << "\n========== STATS ==========\n";
    cout << "Rejected tasks: " << pool.getRejectedTaskCount() << endl;
    cout << "Total executed tasks: " << pool.getTasksExecuted() << endl;
    cout << "Average task wait time (ms): " << pool.getAverageWaitTime() << endl;
    
    vector<long long> all_full;
    for (int i = 0; i < 2; ++i) 
    {
        vector<long long> times = pool.getQueue(i)->get_full_times();
        all_full.insert(all_full.end(), times.begin(), times.end());
    }

    if (!all_full.empty()) 
    {
        auto [min_it, max_it] = minmax_element(all_full.begin(), all_full.end());
        cout << "Max full queue time (ms): " << *max_it << endl;
        cout << "Min full queue time (ms): " << *min_it << endl;
    } else 
    {
        cout << "Queue was never fully filled.\n";
    }

    cout << "Average worker idle time (ms): " << pool.getAverageIdleTime() << endl;

    return 0;
}